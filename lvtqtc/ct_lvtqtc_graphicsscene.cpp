// ct_lvtqtc_graphicsscene.cpp                                       -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "ct_lvtshr_graphstorage.h"
#include <any>
#include <ct_lvtqtc_graphicsscene.h>

#include <ct_lvtqtc_alg_level_layout.h>
#include <ct_lvtqtc_alg_transitive_reduction.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtqtc_componententity.h>
#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_isa.h>
#include <ct_lvtqtc_lakosentitypluginutils.h>
#include <ct_lvtqtc_logicalentity.h>
#include <ct_lvtqtc_packagedependency.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtqtc_repositoryentity.h>
#include <ct_lvtqtc_undo_add_component.h>
#include <ct_lvtqtc_undo_add_edge.h>
#include <ct_lvtqtc_undo_add_logicalentity.h>
#include <ct_lvtqtc_undo_add_package.h>
#include <ct_lvtqtc_undo_load_entity.h>
#include <ct_lvtqtc_undo_rename_entity.h>
#include <ct_lvtqtc_usesintheimplementation.h>
#include <ct_lvtqtc_usesintheinterface.h>

#include <ct_lvtldr_componentnode.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtldr_physicalloader.h>
#include <ct_lvtldr_typenode.h>
#include <ct_lvtmdb_soci_helper.h>

#include <ct_lvtshr_functional.h>
#include <ct_lvtshr_stringhelpers.h>

#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_pluginmanagerutils.h>

#include <preferences.h>

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QGraphicsRectItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QTextBrowser>
#include <QTimer>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <fstream>
#include <random>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct GraphicsScene::Private {
    std::unordered_map<std::string, LakosEntity *> vertices;
    // A map of Entity unique id strings against vertex descriptors.
    //
    // The values for a particular row in a
    // database table, such as the 'class_declaration' table,
    // should only appear once in the map.

    std::unordered_map<lvtldr::LakosianNode *, lvtldr::NodeLoadFlags> entityLoadFlags;
    // How each loaded element handles loading data.

    std::vector<LakosEntity *> verticesVec;
    // Stores the same vertices as above, but with a stable order.
    // The order is important because positions of childrens are
    // processed by Qt relative to the positions of parents: therefore
    // parents have to be positioned before children.

    std::vector<LakosRelation *> relationVec;
    // stores all relations.

    std::shared_ptr<lvtclr::ColorManagement> colorManagement;
    // Manages the Color for the nodes

    lvtldr::PhysicalLoader physicalLoader;

    bool blockNodeResizeOnHover = false;
    // blocks mouseHoverEvent resizing the nodes with this flag on.

    std::vector<LakosEntity *> selectedEntities;
    // The selected entities is chosen by the user by selecting it on the view.

    QGraphicsSimpleTextItem *bgMessage = nullptr;

    lvtldr::NodeStorage& nodeStorage;

    bool showTransitive = false;
    // Show all transitive edges on the top level elements.
    // up to the children. when this flag changes, all children will
    // also set their showTransitive status. but modifying a child
    // won't change this.

    AlgorithmTransitiveReduction *transitiveReductionAlg = nullptr;

    lvtprj::ProjectFile const& projectFile;

    std::optional<std::reference_wrapper<lvtplg::PluginManager>> pluginManager = std::nullopt;

    explicit Private(NodeStorage& nodeStorage, lvtprj::ProjectFile const& projectFile):
        physicalLoader(nodeStorage), nodeStorage(nodeStorage), projectFile(projectFile)
    {
        showTransitive = Preferences::showRedundantEdgesDefault();
    }
};

// Spacing between the relation arrows

// --------------------------------------------
// class GraphicsScene
// --------------------------------------------

GraphicsScene::GraphicsScene(NodeStorage& nodeStorage, lvtprj::ProjectFile const& projectFile, QObject *parent):
    QGraphicsScene(parent), d(std::make_unique<GraphicsScene::Private>(nodeStorage, projectFile))
{
    static int last_id = 0;
    this->setObjectName(QString::fromStdString("gs_" + std::to_string(last_id)));
    last_id++;

    d->transitiveReductionAlg = new AlgorithmTransitiveReduction();

    d->physicalLoader.setGraph(this);
    d->physicalLoader.setExtDeps(true);

    QObject::connect(&d->nodeStorage, &NodeStorage::storageCleared, this, &GraphicsScene::clearGraph);

    QObject::connect(&d->nodeStorage, &NodeStorage::nodeNameChanged, this, [this](LakosianNode *node) {
        auto *entity = findLakosEntityFromUid(node->uid());
        if (!entity) {
            // This Graphics Scene doesn't have such entity to update
            return;
        }

        // Update the node data
        entity->setQualifiedName(node->qualifiedName());
        entity->setName(node->name());
        entity->updateTooltip();

        // Update all relations to that node
        for (auto const& ec : entity->edgesCollection()) {
            for (auto *relation : ec->relations()) {
                relation->updateTooltip();
            }
        }
        for (auto const& ec : entity->targetCollection()) {
            for (auto *relation : ec->relations()) {
                relation->updateTooltip();
            }
        }
    });

    QObject::connect(&d->nodeStorage, &NodeStorage::nodeAdded, this, [this](LakosianNode *node, std::any userdata) {
        auto *parentPackage = node->parent();
        auto *parent = parentPackage ? findLakosEntityFromUid(parentPackage->uid()) : nullptr;

        try {
            auto *anyScene = std::any_cast<GraphicsScene *>(userdata);
            if (!parent && anyScene != this) {
                return;
            }
        } catch (const std::bad_any_cast&) {
            // noop.
        }

        lvtshr::LoaderInfo info;
        info.setHasParent(parent != nullptr);

        // The parameter after newPackageId has the name `selected`, but that actually serves to tell if this
        // entity will be a "graph" or a "leaf".
        auto *entity = ([&]() -> LakosEntity * {
            switch (node->type()) {
            case lvtshr::DiagramType::ClassType:
                return addUdtVertex(node, true, parent, info);
            case lvtshr::DiagramType::ComponentType:
                return addCompVertex(node, true, parent, info);
            case lvtshr::DiagramType::PackageType:
                return addPkgVertex(node, true, parent, info);
            case lvtshr::DiagramType::FreeFunctionType:
                // Not implemented (We do not support creating free functions in CAD mode)
                break;
            case lvtshr::DiagramType::RepositoryType:
                break;
            case lvtshr::DiagramType::NoneType:
                break;
            }
            return nullptr;
        })();
        assert(entity);
        if (!parent) {
            addItem(entity);
        }

        entity->enableLayoutUpdates();
        entity->show();

        if (parent) {
            if (!parent->isExpanded()) {
                parent->toggleExpansion(QtcUtil::CreateUndoAction::e_No);
            }
        }
    });

    QObject::connect(&d->nodeStorage, &NodeStorage::nodeRemoved, this, [this](LakosianNode *node) {
        auto *entity = findLakosEntityFromUid(node->uid());
        if (!entity) {
            // This Graphics Scene doesn't have such entity to update
            return;
        }

        unloadEntity(entity);
    });

    QObject::connect(&d->nodeStorage,
                     &NodeStorage::physicalDependencyAdded,
                     this,
                     [this](LakosianNode *source, LakosianNode *target) {
                         auto *fromEntity = findLakosEntityFromUid(source->uid());
                         auto *toEntity = findLakosEntityFromUid(target->uid());
                         if (!fromEntity || !toEntity) {
                             // This Graphics Scene doesn't have such entities to update
                             return;
                         }
                         addEdgeBetween(fromEntity, toEntity, lvtshr::LakosRelationType::PackageDependency);
                         fromEntity->getTopLevelParent()->calculateEdgeVisibility();
                         fromEntity->recursiveEdgeRelayout();
                     });

    QObject::connect(&d->nodeStorage,
                     &NodeStorage::physicalDependencyRemoved,
                     this,
                     [this](LakosianNode *source, LakosianNode *target) {
                         auto *fromEntity = findLakosEntityFromUid(source->uid());
                         auto *toEntity = findLakosEntityFromUid(target->uid());
                         if (!fromEntity || !toEntity) {
                             // This Graphics Scene doesn't have such entities to update
                             return;
                         }

                         // explicit copy, so we don't mess with internal iterators.
                         auto allCollections = fromEntity->edgesCollection();

                         std::vector<LakosRelation *> toDelete;
                         for (const auto& collection : allCollections) {
                             if (collection->to() == toEntity) {
                                 for (auto *relation : collection->relations()) {
                                     fromEntity->removeEdge(relation);
                                     toEntity->removeEdge(relation);
                                     toDelete.push_back(relation);
                                 }
                             }
                         }

                         for (LakosRelation *rel : toDelete) {
                             rel->setParent(nullptr);
                             removeItem(rel);
                             d->relationVec.erase(
                                 std::remove(std::begin(d->relationVec), std::end(d->relationVec), rel),
                                 std::end(d->relationVec));

                             delete rel;
                         }

                         fromEntity->getTopLevelParent()->calculateEdgeVisibility();
                         fromEntity->recursiveEdgeRelayout();
                     });

    QObject::connect(&d->nodeStorage,
                     &NodeStorage::logicalRelationAdded,
                     this,
                     [this](LakosianNode *source, LakosianNode *target, lvtshr::LakosRelationType type) {
                         auto *fromEntity = findLakosEntityFromUid(source->uid());
                         auto *toEntity = findLakosEntityFromUid(target->uid());
                         if (!fromEntity || !toEntity) {
                             // This Graphics Scene doesn't have such entities to update
                             return;
                         }
                         addEdgeBetween(fromEntity, toEntity, type);

                         fromEntity->getTopLevelParent()->calculateEdgeVisibility();
                         fromEntity->recursiveEdgeRelayout();
                     });

    QObject::connect(
        &d->nodeStorage,
        &NodeStorage::logicalRelationRemoved,
        this,
        [this](LakosianNode *source, LakosianNode *target, lvtshr::LakosRelationType type) {
            auto *fromEntity = findLakosEntityFromUid(source->uid());
            auto *toEntity = findLakosEntityFromUid(target->uid());

            // explicit copy, so we don't mess with internal iterators.
            auto allCollections = fromEntity->edgesCollection();

            std::vector<LakosRelation *> toDelete;
            for (const auto& collection : allCollections) {
                if (collection->to() == toEntity) {
                    for (auto *relation : collection->relations()) {
                        fromEntity->removeEdge(relation);
                        toEntity->removeEdge(relation);
                        toDelete.push_back(relation);
                    }
                }
            }

            for (LakosRelation *rel : toDelete) {
                rel->setParent(nullptr);
                removeItem(rel);
                d->relationVec.erase(std::remove(std::begin(d->relationVec), std::end(d->relationVec), rel),
                                     std::end(d->relationVec));

                delete rel;
            }

            fromEntity->getTopLevelParent()->calculateEdgeVisibility();
            fromEntity->recursiveEdgeRelayout();
        });

    QObject::connect(&d->nodeStorage,
                     &NodeStorage::entityReparent,
                     this,
                     [this](LakosianNode *lakosianNode, LakosianNode *oldParent, LakosianNode *newParent) {
                         auto *entity = findLakosEntityFromUid(lakosianNode->uid());
                         if (!entity) {
                             // This Graphics Scene doesn't have such entity to update
                             return;
                         }

                         auto *oldParentEntity = findLakosEntityFromUid(oldParent->uid());
                         auto *newParentEntity = findLakosEntityFromUid(newParent->uid());
                         if (oldParentEntity && !newParentEntity) {
                             // Entity must vanish from this scene, as it doesn't have the other parent to go to.
                             unloadEntity(entity);
                         } else if (newParentEntity) {
                             // Setting the new parent will automatically update the old parent if it is in this scene.
                             entity->setParentItem(newParentEntity);
                             entity->recursiveEdgeRelayout();
                         }
                     });

    d->bgMessage = new QGraphicsSimpleTextItem();
    d->bgMessage->setText(tr("Drag And Drop Elements\nTo Visualize Them"));
    d->bgMessage->setVisible(true);
    addItem(d->bgMessage);
}

GraphicsScene::~GraphicsScene() noexcept = default;

std::vector<LakosRelation *> GraphicsScene::edges() const
{
    return d->relationVec;
}

std::vector<LakosEntity *> GraphicsScene::vertices() const
{
    return d->verticesVec;
}

LakosEntity *GraphicsScene::findLakosEntityFromUid(lvtshr::UniqueId uid) const
{
    const auto it = std::find_if(d->verticesVec.cbegin(), d->verticesVec.cend(), [uid](LakosEntity *entity) {
        return entity->uniqueId() == uid;
    });
    if (it == d->verticesVec.cend()) {
        return nullptr;
    }
    return *it;
}

void GraphicsScene::setColorManagement(const std::shared_ptr<lvtclr::ColorManagement>& colorManagement)
{
    assert(colorManagement);
    d->colorManagement = colorManagement;
}

std::vector<LakosEntity *> GraphicsScene::selectedEntities() const
{
    return d->selectedEntities;
}

// TODO: Pass the entity that we don't want to collapse here.'
void GraphicsScene::collapseToplevelEntities()
{
    for (LakosEntity *entity : d->verticesVec) {
        if (entity->parentItem() == nullptr) {
            entity->collapse(QtcUtil::CreateUndoAction::e_No);
        }
    }
    reLayout();
}

void GraphicsScene::expandToplevelEntities()
{
    for (LakosEntity *entity : d->verticesVec) {
        if (entity->parentItem() == nullptr) {
            entity->expand(QtcUtil::CreateUndoAction::e_No);
        }
    }
    reLayout();
}

void GraphicsScene::reLayout()
{
    runLayoutAlgorithm();

    for (auto *entity : d->verticesVec) {
        if (entity->parentItem() == nullptr) {
            entity->calculateEdgeVisibility();
            entity->recursiveEdgeRelayout();
        }
    }
    updateBoundingRect();
}

// This class defines what we need to implement on classes that load graphs visually
void GraphicsScene::clearGraph()
{
    d->showTransitive = Preferences::showRedundantEdgesDefault();
    d->vertices.clear();
    d->verticesVec.clear();
    d->relationVec.clear();
    d->entityLoadFlags.clear();
    d->transitiveReductionAlg->reset();
    d->physicalLoader.clear();

    removeItem(d->bgMessage);
    clear();

    addItem(d->bgMessage);
    d->bgMessage->setPos(sceneRect().center());
    d->bgMessage->setVisible(true);
}

namespace {

QString errorKindToStr(ErrorRemoveEntity::Kind kind, const QString& type)
{
    switch (kind) {
    case lvtldr::ErrorRemoveEntity::Kind::CannotRemoveWithProviders: {
        return QObject::tr(
                   "Currently we can't remove %1 with connected with other packages, break the connections "
                   "first.")
            .arg(type);
    }
    case lvtldr::ErrorRemoveEntity::Kind::CannotRemoveWithClients: {
        return QObject::tr("Currently we can't remove %1 with clients, break the connections first.").arg(type);
    }
    case lvtldr::ErrorRemoveEntity::Kind::CannotRemoveWithChildren: {
        return QObject::tr("Currently we can't remove %1 that contains children, remove the childs first.").arg(type);
    }
    }

    // Unreachable.
    return QString();
}

template<typename EntityType>
LakosEntity *addVertex(GraphicsScene *scene,
                       GraphicsScene::Private *d,
                       lvtldr::LakosianNode *node,
                       bool selected,
                       LakosEntity *parent,
                       lvtshr::LoaderInfo info,
                       lvtldr::NodeStorage& nodeStorage,
                       std::optional<std::reference_wrapper<lvtplg::PluginManager>> pm)
{
    std::string uid = EntityType::getUniqueId(node->id());
    auto search = d->vertices.find(uid);
    if (search != d->vertices.end()) {
        if (selected) {
            search->second->setHighlighted(selected);
        }
        return search->second;
    }

    // freed by either the parent or as a top level item in the GraphicsScene
    LakosEntity *entity = new EntityType(node, info);
    entity->setColorManagement(d->colorManagement.get());
    entity->setHighlighted(selected);
    entity->setZValue(QtcUtil::e_NODE_LAYER);
    if (pm) {
        entity->setPluginManager(*pm);
    }

    QObject::connect(entity, &LakosEntity::toggleSelection, scene, [scene, d, entity] {
        if (std::find(d->selectedEntities.begin(), d->selectedEntities.end(), entity) == d->selectedEntities.end()) {
            d->selectedEntities.push_back(entity);
            entity->setSelected(true);
        } else {
            std::erase(d->selectedEntities, entity);
            entity->setSelected(false);
        }
        entity->updateZLevel();

        Q_EMIT scene->selectedEntityChanged(entity);
    });

    QObject::connect(entity, &LakosEntity::requestRemoval, scene, [scene, &nodeStorage, node, entity] {
        auto *view = qobject_cast<GraphicsView *>(scene->views().constFirst());
        auto name = node->name();
        auto qualifiedName = node->qualifiedName();
        auto parentQualifiedName = node->parent() ? node->parent()->qualifiedName() : "";

        if (node->type() == lvtshr::DiagramType::PackageType) {
            auto err = nodeStorage.removePackage(node);
            if (err.has_error()) {
                Q_EMIT scene->errorMessage(errorKindToStr(err.error().kind, QStringLiteral("packages")));
                return;
            }
            view->undoCommandReceived(new UndoAddPackage(scene,
                                                         entity->pos(),
                                                         name,
                                                         qualifiedName,
                                                         parentQualifiedName,
                                                         QtcUtil::UndoActionType::e_Remove,
                                                         nodeStorage));
        } else if (node->type() == lvtshr::DiagramType::ComponentType) {
            auto err = nodeStorage.removeComponent(node);
            if (err.has_error()) {
                Q_EMIT scene->errorMessage(errorKindToStr(err.error().kind, "components"));
                return;
            }
            view->undoCommandReceived(new UndoAddComponent(scene,
                                                           entity->pos(),
                                                           name,
                                                           qualifiedName,
                                                           parentQualifiedName,
                                                           QtcUtil::UndoActionType::e_Remove,
                                                           nodeStorage));
        } else if (node->type() == lvtshr::DiagramType::ClassType) {
            auto err = nodeStorage.removeLogicalEntity(node);
            if (err.has_error()) {
                Q_EMIT scene->errorMessage(errorKindToStr(err.error().kind, "user defined type"));
                return;
            }
            view->undoCommandReceived(new UndoAddLogicalEntity(scene,
                                                               entity->pos(),
                                                               name,
                                                               qualifiedName,
                                                               parentQualifiedName,
                                                               QtcUtil::UndoActionType::e_Remove,
                                                               nodeStorage));
        } else {
            Q_EMIT scene->errorMessage("Invalid entity type for removal.");
        }
    });

    scene->connectEntitySignals(entity);

    if (parent) {
        entity->setParentItem(parent);
    }

    QObject::connect(entity, &LakosEntity::createReportActionClicked, scene, &GraphicsScene::createReportActionClicked);

    d->vertices.insert({uid, entity});
    d->verticesVec.push_back(entity);
    d->entityLoadFlags.insert({entity->internalNode(), lvtldr::NodeLoadFlags{}});
    if (Preferences::enableDebugOutput()) {
        qDebug() << "Setting empty flags for" << QString::fromStdString(entity->qualifiedName());
    }
    d->bgMessage->setVisible(false);
    return entity;
}
} // namespace

LakosEntity *
GraphicsScene::addUdtVertex(lvtldr::LakosianNode *node, bool selected, LakosEntity *parent, lvtshr::LoaderInfo info)
{
    return addVertex<LogicalEntity>(this, d.get(), node, selected, parent, info, d->nodeStorage, d->pluginManager);
}

LakosEntity *
GraphicsScene::addPkgVertex(lvtldr::LakosianNode *node, bool selected, LakosEntity *parent, lvtshr::LoaderInfo info)
{
    return addVertex<PackageEntity>(this, d.get(), node, selected, parent, info, d->nodeStorage, d->pluginManager);
}

LakosEntity *GraphicsScene::addRepositoryVertex(lvtldr::LakosianNode *node,
                                                bool selected,
                                                LakosEntity *parent,
                                                lvtshr::LoaderInfo info)
{
    return addVertex<RepositoryEntity>(this, d.get(), node, selected, parent, info, d->nodeStorage, d->pluginManager);
}

LakosEntity *
GraphicsScene::addCompVertex(lvtldr::LakosianNode *node, bool selected, LakosEntity *parent, lvtshr::LoaderInfo info)
{
    return addVertex<ComponentEntity>(this, d.get(), node, selected, parent, info, d->nodeStorage, d->pluginManager);
}

namespace {

template<typename RelationType>
LakosRelation *addClassBasedRelation(GraphicsScene *scn, LakosEntity *source, LakosEntity *target)
{
    assert(source && source->instanceType() == lvtshr::DiagramType::ClassType);
    assert(target && target->instanceType() == lvtshr::DiagramType::ClassType);
    if (source->hasRelationshipWith(target)) {
        return nullptr;
    }

    // add an extra edge between the top level containers
    LakosEntity *sourceParent = source->getTopLevelParent();
    LakosEntity *targetParent = target->getTopLevelParent();
    if (sourceParent != targetParent) {
        if (sourceParent != source || targetParent != target) {
            // if both parents are logical, add this kind of logical relation
            if (sourceParent->instanceType() == lvtshr::DiagramType::ClassType
                && targetParent->instanceType() == lvtshr::DiagramType::ClassType
                && !sourceParent->hasRelationshipWith(targetParent)) {
                (void) scn->addRelation(new RelationType(sourceParent, targetParent));
            }
            // if both parents are physical, add a physical dependency
            if (sourceParent->instanceType() != lvtshr::DiagramType::ClassType
                && targetParent->instanceType() != lvtshr::DiagramType::ClassType
                && !sourceParent->hasRelationshipWith(targetParent)) {
                (void) scn->addRelation(new PackageDependency(sourceParent, targetParent));
            }
        }
    }

    return scn->addRelation(new RelationType(source, target));
}

} // namespace

LakosRelation *GraphicsScene::addIsARelation(LakosEntity *source, LakosEntity *target)
{
    return addClassBasedRelation<IsA>(this, source, target);
}

LakosRelation *GraphicsScene::addUsesInTheInterfaceRelation(LakosEntity *source, LakosEntity *target)
{
    return addClassBasedRelation<UsesInTheInterface>(this, source, target);
}

LakosRelation *GraphicsScene::addUsesInTheImplementationRelation(LakosEntity *source, LakosEntity *target)
{
    return addClassBasedRelation<UsesInTheImplementation>(this, source, target);
}

LakosRelation *GraphicsScene::addPackageDependencyRelation(LakosEntity *source, LakosEntity *target)
{
    assert(source && source->instanceType() != lvtshr::DiagramType::ClassType);
    assert(target && target->instanceType() != lvtshr::DiagramType::ClassType);

    if (source->hasRelationshipWith(target)) {
        return nullptr;
    }

    LakosEntity *sourceParent = source->getTopLevelParent();
    LakosEntity *targetParent = target->getTopLevelParent();

    if (sourceParent != targetParent) {
        if ((sourceParent != source || targetParent != target) && !sourceParent->hasRelationshipWith(targetParent)) {
            assert(sourceParent->instanceType() != lvtshr::DiagramType::ClassType);
            assert(targetParent->instanceType() != lvtshr::DiagramType::ClassType);
            (void) addRelation(new PackageDependency(sourceParent, targetParent));
        }
    }

    return addRelation(new PackageDependency(source, target));
}

LakosEntity *GraphicsScene::outermostParent(LakosEntity *a, LakosEntity *b)
{
    // If an edge has from() and to() on different parents, we still
    // could hit a possibility that there's a common parent. such as
    // |--------------------------
    // |          parent
    // | |=====|       |---------|
    // | |from |-------|-->|to|  |
    // | |=====|       |   ----  |
    // |               |---------|
    ///---------------------------

    const QList<LakosEntity *> fromParents = a->parentHierarchy();
    const QList<LakosEntity *> toParents = b->parentHierarchy();

    for (auto *fromParent : fromParents) {
        for (auto *toParent : toParents) {
            if (fromParent == toParent) {
                return fromParent;
            }
        }
    }
    return nullptr;
}

// TODO: Move this logic to the LakosEntity code.
LakosRelation *GraphicsScene::addRelation(LakosRelation *relation, bool isVisible)
{
    LakosEntity *from = relation->from();
    LakosEntity *to = relation->to();

    if (from->isAncestorOf(to) || to->isAncestorOf(from)) {
        if (!relation->scene()) {
            delete relation;
        }
        return nullptr;
    }

    relation->setShouldBeHidden(!isVisible);

    // From Here -----------------
    std::vector<std::shared_ptr<EdgeCollection>>& edges = from->edgesCollection();
    auto it = std::find_if(std::begin(edges), std::end(edges), [to](const std::shared_ptr<EdgeCollection>& edge) {
        return edge->to() == to;
    });

    if (it == std::end(edges)) {
        // TODO: Move he initialization to constructor.
        auto edgeCollection = std::make_shared<EdgeCollection>();
        edgeCollection->setFrom(from);
        edgeCollection->setTo(to);
        edges.push_back(edgeCollection);
        to->addTargetCollection(edgeCollection);
        it = std::prev(std::end(edges));
    }

    relation = (*it)->addRelation(relation);

    auto *commonParent = from->commonAncestorItem(to);

    if (commonParent) {
        relation->setParentItem(commonParent);
    }
    relation->setZValue(QtcUtil::e_EDGE_LAYER);
    d->relationVec.push_back(relation);
    (*it)->layoutRelations();

    connect(relation, &LakosRelation::undoCommandCreated, this, [this](QUndoCommand *command) {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        assert(view);
        view->undoCommandReceived(command);
    });

    connect(relation, &LakosRelation::requestRemoval, this, [this, relation] {
        using lvtshr::LakosRelationType;

        auto *from = relation->from();
        auto *to = relation->to();

        auto relationType = relation->relationType();

        auto isLogicalRelation =
            (relationType == LakosRelationType::IsA || relationType == LakosRelationType::UsesInTheImplementation
             || relationType == LakosRelationType::UsesInTheInterface);
        if (isLogicalRelation) {
            auto *fromTypeNode = dynamic_cast<TypeNode *>(from->internalNode());
            auto *toTypeNode = dynamic_cast<TypeNode *>(to->internalNode());
            assert(fromTypeNode && toTypeNode);

            auto result = d->nodeStorage.removeLogicalRelation(fromTypeNode, toTypeNode, relationType);
            if (result.has_error()) {
                using ErrorKind = ErrorRemoveLogicalRelation::Kind;
                switch (result.error().kind) {
                case (ErrorKind::InexistentRelation): {
                    assert(false && "GraphicsScene has a relation not present in the model");
                }
                case (ErrorKind::InvalidLakosRelationType): {
                    assert(false && "Trying to remove a LakosRelation with unexpected model type");
                }
                }
            }
        }

        auto isPhysicalRelation = relationType == LakosRelationType::PackageDependency;
        if (isPhysicalRelation) {
            auto *fromPhysicalNode = from->internalNode();
            auto *toPhysicalNode = to->internalNode();
            assert(fromPhysicalNode && toPhysicalNode);

            auto result = d->nodeStorage.removePhysicalDependency(fromPhysicalNode, toPhysicalNode);
            if (result.has_error()) {
                using ErrorKind = ErrorRemovePhysicalDependency::Kind;
                switch (result.error().kind) {
                case (ErrorKind::InexistentRelation): {
                    assert(false && "GraphicsScene has a relation not present in the model");
                }
                }
            }
        }

        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        assert(view);
        view->undoCommandReceived(new UndoAddEdge(from->qualifiedName(),
                                                  to->qualifiedName(),
                                                  relationType,
                                                  QtcUtil::UndoActionType::e_Remove,
                                                  d->nodeStorage));
    });

    return relation;
}

void GraphicsScene::runLayoutAlgorithm()
{
    auto topLevelEntities = std::vector<LakosEntity *>();
    for (auto *e : d->verticesVec) {
        if (!e->parentItem()) {
            topLevelEntities.push_back(e);
        }
    }

    auto direction = Preferences::invertVerticalLevelizationLayout() ? +1 : -1;
    std::function<void(LakosEntity *)> recursiveLevelLayout = [&](LakosEntity *e) -> void {
        auto childs = e->lakosEntities();
        for (auto *c : childs) {
            recursiveLevelLayout(c);
        }
        e->levelizationLayout(LakosEntity::LevelizationLayoutType::Vertical, direction);
    };
    for (auto *e : topLevelEntities) {
        recursiveLevelLayout(e);
    }
    auto entityToLevel = computeLevelForEntities(topLevelEntities);
    runLevelizationLayout(entityToLevel,
                          {LakosEntity::LevelizationLayoutType::Vertical,
                           direction,
                           Preferences::spaceBetweenLevels(),
                           Preferences::spaceBetweenSublevels(),
                           Preferences::spaceBetweenEntities(),
                           Preferences::maxEntitiesPerLevel()});
}

bool GraphicsScene::blockNodeResizeOnHover() const
{
    return d->blockNodeResizeOnHover;
}

void GraphicsScene::setEntityPos(const lvtshr::UniqueId& uid, QPointF pos) const
{
    auto *entity = findLakosEntityFromUid(uid);
    entity->setPos(pos);

    // triggers a recalculation of the parent's boundaries.
    Q_EMIT entity->moving();

    // Tells the system that the graph updated.
    Q_EMIT entity->graphUpdate();
}

void GraphicsScene::setBlockNodeResizeOnHover(bool block)
{
    d->blockNodeResizeOnHover = block;

    if (block) {
        // View "0" is the main view, view "1" is the minimap.
        QGraphicsView *view = views().at(0);
        const QPoint viewCoords = view->mapFromGlobal(QCursor::pos());
        const QPointF sceneCoords = view->mapToScene(viewCoords);
        const auto itemList = items(sceneCoords);
        for (QGraphicsItem *item : itemList) {
            if (auto *lakosEntity = qgraphicsitem_cast<LakosEntity *>(item)) {
                lakosEntity->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
            }
        }
    }
    update();
}

// ---------- Our State Machine starts Here. ------------------
lvtldr::NodeLoadFlags GraphicsScene::loadFlagsFor(lvtldr::LakosianNode *node) const
{
    const auto search = d->entityLoadFlags.find(node);
    if (search != d->entityLoadFlags.end()) {
        return search->second;
    }

    return NodeLoadFlags{};
}

// HACK: This should really not exist.
void GraphicsScene::fixRelationsParentRelationship()
{
    auto for_each_relation = [this](const std::function<void(LakosRelation *)>& func) {
        for (const auto *vertex : d->verticesVec) {
            for (const auto& edges : vertex->edgesCollection()) {
                for (auto *edge : edges->relations()) {
                    func(edge);
                }
            }
        }
    };

    // TODO: Move this somewhere else. to LakosEntity perhaps.
    auto is_connected = [](LakosEntity *a, LakosEntity *b) -> bool {
        const auto& collection = a->edgesCollection();
        return std::any_of(collection.begin(), collection.end(), [b](const auto& edges) {
            return edges->to() == b;
        });
    };

    for_each_relation([this, is_connected](LakosRelation *edge) {
        auto *parent = qgraphicsitem_cast<LakosEntity *>(edge->from()->commonAncestorItem(edge->to()));
        if (parent) {
            edge->setParentItem(parent);
            return;
        }

        // We don't have a common parent, resort to heuristics.
        auto *pFrom = qgraphicsitem_cast<LakosEntity *>(edge->from()->parentItem());
        auto *pTo = qgraphicsitem_cast<LakosEntity *>(edge->to()->parentItem());
        if (pFrom && pFrom != pTo) {
            // on this case we need to add a edge between the two different parents if there's none.
            edge->setParentItem(nullptr);

            // A component that has a parent *can* point to a component that
            // has no parent, such as Qt classes, STD entities and so on.
            // we need to add those to a subcomponent based on the include path,
            // but that won't happen now.
            if (pFrom && pTo) {
                if (!is_connected(pFrom, pTo)) {
                    if (pFrom->instanceType() != lvtshr::DiagramType::ClassType
                        && pTo->instanceType() != lvtshr::DiagramType::ClassType) {
                        addPackageDependencyRelation(pFrom, pTo);
                    }
                }
            }
        } else {
            edge->setParentItem(nullptr);
        }
    });
}

void GraphicsScene::enableLayoutUpdates()
{
    for (LakosEntity *entity : d->verticesVec) {
        entity->enableLayoutUpdates();
    }
}

void GraphicsScene::layoutDone()
{
    reLayout();
}

} // namespace Codethink::lvtqtc

namespace Codethink::lvtqtc {

std::vector<LakosEntity *>& GraphicsScene::allEntities() const
{
    return d->verticesVec;
}

void GraphicsScene::connectEntitySignals(LakosEntity *entity)
{
    assert(entity);

    const std::string qualifiedName = entity->qualifiedName();

    connect(entity, &LogicalEntity::navigateRequested, this, [qualifiedName] {
        // TODO: Navigate.
        (void) qualifiedName;
    });

    connect(entity, &LakosEntity::undoCommandCreated, this, [this](QUndoCommand *command) {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        assert(view);
        view->undoCommandReceived(command);
    });

    connect(entity, &LakosEntity::requestMultiSelectActivation, this, [this](const QPoint& positionInScene) {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        view->activateMultiSelect(view->mapFromScene(positionInScene));
    });

    connect(entity, &LogicalEntity::graphUpdate, this, [this] {
        updateBoundingRect();
    });

    connect(entity, &LakosEntity::entityRenameRequest, this, [this](lvtshr::UniqueId uid, const std::string& newName) {
        auto *node = d->nodeStorage.findById(uid);
        auto oldName = node->name();
        auto oldQualifiedName = node->qualifiedName();
        node->setName(newName);

        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        view->undoCommandReceived(new UndoRenameEntity(node->qualifiedName(),
                                                       oldQualifiedName,
                                                       node->type(),
                                                       oldName,
                                                       newName,
                                                       d->nodeStorage));
    });

    connect(entity, &LakosEntity::requestRelayout, this, [this, entity] {
        // TODO: Review this
        entity->recursiveEdgeRelayout();
        updateBoundingRect();
    });

    connect(entity, &LakosEntity::loadChildren, this, [entity, this] {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        view->undoCommandReceived(new UndoLoadEntity(this,
                                                     entity->internalNode()->uid(),
                                                     UnloadDepth::Children,
                                                     QtcUtil::UndoActionType::e_Add));
        view->fitAllInView();
    });

    connect(entity, &LakosEntity::loadClients, this, [entity, this](bool onlyLocal) {
        auto *node = entity->internalNode();

        auto& flags = d->entityLoadFlags[node];
        flags.traverseClients = true;
        flags.traverseClientsOnlyLocal = onlyLocal;

        finalizeEntityPartialLoad(entity);
    });

    connect(entity, &LakosEntity::loadProviders, this, [entity, this](bool onlyLocal) {
        auto *node = entity->internalNode();

        auto& flags = d->entityLoadFlags[node];
        flags.traverseProviders = true;
        flags.traverseProvidersOnlyLocal = onlyLocal;

        finalizeEntityPartialLoad(entity);
    });

    // Perhaps this should be a toggle?
    connect(entity, &LakosEntity::requestGraphRelayout, this, [this] {
        if (Preferences::enableDebugOutput()) {
            qDebug() << "Running graph relayout";
        }
        reLayout();
    });

    connect(entity, &LakosEntity::unloadThis, this, [this, entity] {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        view->undoCommandReceived(new UndoLoadEntity(this,
                                                     entity->internalNode()->uid(),
                                                     UnloadDepth::Entity,
                                                     QtcUtil::UndoActionType::e_Remove));
    });

    connect(entity, &LakosEntity::unloadChildren, this, [this, entity] {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        view->undoCommandReceived(new UndoLoadEntity(this,
                                                     entity->internalNode()->uid(),
                                                     UnloadDepth::Children,
                                                     QtcUtil::UndoActionType::e_Remove));
    });

    connect(entity, &LakosEntity::requestNewTab, this, &GraphicsScene::requestNewTab);
}

void GraphicsScene::loadEntity(lvtshr::UniqueId uuid, UnloadDepth depth)
{
    // TODO: This needs to be improved with the other possible load flags.
    if (depth == UnloadDepth::Children) {
        LakosEntity *entity = findLakosEntityFromUid(uuid);
        auto *node = entity->internalNode();
        d->entityLoadFlags[node].loadChildren = true;
        finalizeEntityPartialLoad(entity);
    } else {
        auto *node = d->nodeStorage.findById(uuid);
        const QString qualName = QString::fromStdString(node->qualifiedName());

        loadEntityByQualifiedName(qualName, QPoint(0, 0));

        d->transitiveReductionAlg->reset();
        searchTransitiveRelations();
        reLayout();
    }
}

void GraphicsScene::unloadEntity(lvtshr::UniqueId uuid, UnloadDepth depth)
{
    LakosEntity *entity = findLakosEntityFromUid(uuid);
    if (!entity) {
        return;
    }
    if (depth == UnloadDepth::Children) {
        const auto entities = entity->lakosEntities();
        for (auto *child : entities) {
            unloadEntity(child);
        }

        auto& flags = d->entityLoadFlags[entity->internalNode()];
        flags.loadChildren = false;
    } else {
        unloadEntity(entity);
    }

    if (std::find(d->selectedEntities.begin(), d->selectedEntities.end(), entity) != d->selectedEntities.end()) {
        std::erase(d->selectedEntities, entity);
    }

    d->transitiveReductionAlg->reset();
    searchTransitiveRelations();
    reLayout();
}

void GraphicsScene::unloadEntity(LakosEntity *entity)
{
    const auto entities = entity->lakosEntities();
    for (auto *child : entities) {
        unloadEntity(child);
    }

    // lambda to clean collections:
    auto cleanCollections = [this](const std::shared_ptr<EdgeCollection>& ec) {
        while (!ec->relations().empty()) {
            auto *edge = ec->relations().front();
            auto pos = std::find(std::begin(d->relationVec), std::end(d->relationVec), edge);
            if (pos != std::end(d->relationVec)) {
                d->relationVec.erase(pos);
            }

            // this can't be in the edge destructor because
            // we need to make sure we are deleting all edges *before* nodes
            // and in some places that causes a crash. Leave at here for now.
            edge->from()->removeEdge(edge);
            edge->to()->removeEdge(edge);
            delete edge;
        }
    };

    while (!entity->edgesCollection().empty()) {
        auto ec = entity->edgesCollection().front();
        cleanCollections(ec);
    }

    while (!entity->targetCollection().empty()) {
        auto ec = entity->targetCollection().front();
        cleanCollections(ec);
    }

    auto id = entity->internalNode()->id();
    const std::string uniqueId = entity->instanceType() == lvtshr::DiagramType::PackageType
        ? PackageEntity::getUniqueId(id)
        : entity->instanceType() == lvtshr::DiagramType::ComponentType ? ComponentEntity::getUniqueId(id)
        : entity->instanceType() == lvtshr::DiagramType::ClassType     ? LogicalEntity::getUniqueId(id)
                                                                       : std::string{};

    if (uniqueId.empty()) {
        Q_EMIT errorMessage("Tried to remove an invalid element.");
        return;
    }

    d->physicalLoader.unvisitVertex(entity->internalNode());
    d->vertices.erase(uniqueId);
    d->verticesVec.erase(std::remove(std::begin(d->verticesVec), std::end(d->verticesVec), entity),
                         std::end(d->verticesVec));

    if (Preferences::enableDebugOutput()) {
        qDebug() << "Unloading entity" << intptr_t(entity) << QString::fromStdString(entity->name());
    }

    if (d->vertices.empty()) {
        d->bgMessage->setPos(sceneRect().center());
        d->bgMessage->show();
    }

    delete entity;
}

void GraphicsScene::finalizeEntityPartialLoad(LakosEntity *entity)
{
    auto *node = entity->internalNode();
    auto flags = d->entityLoadFlags[node];

    bool success = d->physicalLoader.load(node, flags).has_value();
    if (!success) {
        return;
    }

    // some relationships could have been added, but not on the scene.
    for (auto *vertice : d->verticesVec) {
        if (!vertice->scene()) {
            addItem(vertice);
        }
    }

    for (auto *relation : d->relationVec) {
        if (!relation->scene()) {
            addItem(relation);
        }
    }

    d->transitiveReductionAlg->reset();
    searchTransitiveRelations();
    transitiveRelationSearchFinished();

    for (auto *vertice : d->verticesVec) {
        vertice->enableLayoutUpdates();
    }

    fixRelationsParentRelationship();

    entity->calculateEdgeVisibility();
    reLayout();
}

void GraphicsScene::populateMenu(QMenu& menu, QMenu *debugMenu)
{
    using namespace Codethink::lvtplg;

    if (d->pluginManager) {
        auto getAllEntitiesInCurrentView = [this]() {
            std::vector<std::shared_ptr<Entity>> entitiesInView{};
            for (auto *e : allEntities()) {
                entitiesInView.emplace_back(createWrappedEntityFromLakosEntity(e));
            }
            return entitiesInView;
        };
        auto getEntityByQualifiedName = [this](std::string const& qualifiedName) -> std::shared_ptr<Entity> {
            auto *e = entityByQualifiedName(qualifiedName);
            if (!e) {
                return {};
            }
            return createWrappedEntityFromLakosEntity(e);
        };
        auto getEdgeByQualifiedName = [this](std::string const& fromQualifiedName,
                                             std::string const& toQualifiedName) -> std::optional<Edge> {
            auto *fromEntity = entityByQualifiedName(fromQualifiedName);
            if (!fromEntity) {
                return std::nullopt;
            }
            auto *toEntity = entityByQualifiedName(toQualifiedName);
            if (!toEntity) {
                return std::nullopt;
            }
            return createWrappedEdgeFromLakosEntity(fromEntity, toEntity);
        };
        auto loadEntityByQualifiedName = [this](std::string const& qualifiedName) {
            this->loadEntityByQualifiedName(QString::fromStdString(qualifiedName), {0, 0});
            this->reLayout();
        };
        auto addEdgeByQualifiedName = [this](std::string const& fromQualifiedName,
                                             std::string const& toQualifiedName) -> std::optional<Edge> {
            auto *fromEntity = entityByQualifiedName(fromQualifiedName);
            auto *toEntity = entityByQualifiedName(toQualifiedName);
            if (!fromEntity || !toEntity || fromEntity == toEntity || fromEntity->hasRelationshipWith(toEntity)) {
                return std::nullopt;
            }
            this->addEdgeBetween(fromEntity, toEntity, lvtshr::LakosRelationType::PackageDependency);
            return createWrappedEdgeFromLakosEntity(fromEntity, toEntity);
        };
        auto removeEdgeByQualifiedName = [this](std::string const& fromQualifiedName,
                                                std::string const& toQualifiedName) {
            auto *fromEntity = entityByQualifiedName(fromQualifiedName);
            auto *toEntity = entityByQualifiedName(toQualifiedName);
            if (!fromEntity || !toEntity || fromEntity == toEntity || !fromEntity->hasRelationshipWith(toEntity)) {
                return;
            }
            this->removeEdge(*fromEntity, *toEntity);
        };
        auto hasEdgeByQualifiedName = [this](std::string const& fromQualifiedName, std::string const& toQualifiedName) {
            auto *fromEntity = entityByQualifiedName(fromQualifiedName);
            auto *toEntity = entityByQualifiedName(toQualifiedName);
            if (!fromEntity || !toEntity || fromEntity == toEntity) {
                return false;
            }
            return fromEntity->hasRelationshipWith(toEntity);
        };
        using ctxMenuAction_f = std::function<void(PluginContextMenuActionHandler *)>;
        auto registerContextMenu = [=, this, &menu](std::string const& title, ctxMenuAction_f const& userAction) {
            // make a copy of all the actions we currently have, so we can
            // iterate through it without having problems.
            const auto currentActions = menu.actions();

            // Remove pre-existing actions from scripts.
            for (QAction *act : currentActions) {
                if (act->text() == QString::fromStdString(title)) {
                    errorMessage(
                        "Two or more of your plugins declares\n"
                        "the same context menu, This is not supported.");
                    return;
                }
            }

            auto *action = menu.addAction(QString::fromStdString(title));
            connect(action, &QAction::triggered, this, [=, this]() {
                auto getPluginData = [this](auto&& id) {
                    auto& pm = d->pluginManager.value().get();
                    return pm.getPluginData(id);
                };
                auto getTree = [this](std::string const& id) {
                    auto *pm = &d->pluginManager->get();
                    return PluginManagerQtUtils::createPluginTreeWidgetHandler(pm, id, this);
                };
                auto getDock = [this](std::string const& id) {
                    auto *pm = &d->pluginManager->get();
                    return PluginManagerQtUtils::createPluginDockWidgetHandler(pm, id);
                };
                auto runQueryOnDatabase = [this](std::string const& dbQuery) -> std::vector<std::vector<RawDBData>> {
                    return lvtmdb::SociHelper::runSingleQuery(d->nodeStorage.getSession(), dbQuery);
                };
                auto handler = PluginContextMenuActionHandler{getPluginData,
                                                              getAllEntitiesInCurrentView,
                                                              getEntityByQualifiedName,
                                                              getTree,
                                                              getDock,
                                                              getEdgeByQualifiedName,
                                                              loadEntityByQualifiedName,
                                                              addEdgeByQualifiedName,
                                                              removeEdgeByQualifiedName,
                                                              hasEdgeByQualifiedName,
                                                              runQueryOnDatabase};

                try {
                    userAction(&handler);
                } catch (std::exception& e) {
                    Q_EMIT errorMessage(QString::fromStdString(e.what()));
                }
            });
        };

        auto& pm = d->pluginManager.value().get();
        pm.callHooksContextMenu(getAllEntitiesInCurrentView,
                                getEntityByQualifiedName,
                                getEdgeByQualifiedName,
                                registerContextMenu);
    }

    if (d->showTransitive) {
        auto *action = menu.addAction(tr("Hide redundant edges"));
        connect(action, &QAction::triggered, this, [this] {
            toggleTransitiveRelationVisibility(false);
        });
    } else {
        auto *action = menu.addAction(tr("Show redundant edges"));
        connect(action, &QAction::triggered, this, [this] {
            searchTransitiveRelations();
            toggleTransitiveRelationVisibility(true);
        });
    }
    {
        auto *action = menu.addAction(tr("Collapse Toplevel"));
        connect(action, &QAction::triggered, this, [this] {
            collapseToplevelEntities();
        });

        action = menu.addAction(tr("Expand Toplevel"));
        connect(action, &QAction::triggered, this, [this] {
            expandToplevelEntities();
        });
    }
    if (debugMenu) {
        QAction *action = debugMenu->addAction(tr("Show Edge Bounding Rects"));
        action->setCheckable(true);
        action->setChecked(LakosRelation::showBoundingRect());
        connect(action, &QAction::triggered, this, &GraphicsScene::toggleEdgeBoundingRects);

        action = debugMenu->addAction(tr("Show Edge Shapes"));
        action->setCheckable(true);
        action->setChecked(LakosRelation::showShape());

        connect(action, &QAction::triggered, this, &GraphicsScene::toggleEdgeShapes);

        action = debugMenu->addAction(tr("Show Edge Textual Information"));
        action->setCheckable(true);
        action->setChecked(LakosRelation::showTextualInformation());
        connect(action, &QAction::triggered, this, &GraphicsScene::toggleEdgeTextualInformation);

        action = debugMenu->addAction(tr("Show Edge Intersection Paths"));
        action->setCheckable(true);
        action->setChecked(LakosRelation::showIntersectionPaths());

        connect(action, &QAction::triggered, this, &GraphicsScene::toggleEdgeIntersectionPaths);

        action = debugMenu->addAction(tr("Show Edge original line"));
        action->setCheckable(true);
        action->setChecked(LakosRelation::showOriginalLine());
        connect(action, &QAction::triggered, this, &GraphicsScene::toggleEdgeOriginalLine);
    }
}

void GraphicsScene::searchTransitiveRelations()
{
    if (d->transitiveReductionAlg->hasRun()) {
        toggleTransitiveRelationVisibility(d->showTransitive);
        return;
    }

    for (auto *entity : d->verticesVec) {
        entity->resetRedundantRelations();
    }

    auto visibleEntities = std::vector<LakosEntity *>();
    for (auto *e : d->verticesVec) {
        visibleEntities.push_back(e);
    }
    d->transitiveReductionAlg->setVertices(visibleEntities);
    d->transitiveReductionAlg->run();
    transitiveRelationSearchFinished();
}

void GraphicsScene::handleViewportChanged()
{
    updateBoundingRect();
}
void GraphicsScene::updateBoundingRect()
{
    // View "0" is the main view, view "1" is the minimap.
    QGraphicsView *graphicsView = views().at(0);

    // The viewport
    QRectF viewportRect = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();

    // The width and height of viewport
    qreal viewportWidth = viewportRect.width();
    qreal viewportHeight = viewportRect.height();

    // The width and height of items bounding rectangle
    qreal itemsBoundingRectWidth = itemsBoundingRect().width();
    qreal itemsBoundingRectHeight = itemsBoundingRect().height();

    // Take whichever is smaller (the viewport or the bounding rectangle), and adjust the scene rect accordingly:
    auto const ADJUST_PCT = 0.5;
    qreal widthAdjust = qMin(viewportWidth, itemsBoundingRectWidth) * ADJUST_PCT;
    qreal heightAdjust = qMin(viewportHeight, itemsBoundingRectHeight) * ADJUST_PCT;

    setSceneRect(itemsBoundingRect().adjusted(-widthAdjust, -heightAdjust, widthAdjust, heightAdjust));
}

void GraphicsScene::transitiveRelationSearchFinished()
{
    if (d->transitiveReductionAlg->hasError()) {
        Q_EMIT errorMessage(d->transitiveReductionAlg->errorMessage());
    }

    // mark all of the redundant edges
    for (const auto& [node, edgeVector] : d->transitiveReductionAlg->redundantEdgesByNode()) {
        for (const auto& collection : edgeVector) {
            node->setRelationRedundant(collection);
        }
    }

    toggleTransitiveRelationVisibility(d->showTransitive);
}

void GraphicsScene::toggleTransitiveRelationVisibility(bool show)
{
    d->showTransitive = show;
    fixTransitiveEdgeVisibility();
}

void GraphicsScene::fixTransitiveEdgeVisibility()
{
    for (LakosEntity *node : d->verticesVec) {
        node->showRedundantRelations(d->showTransitive);
    }

    if (d->showTransitive) {
        for (LakosEntity *node : d->verticesVec) {
            node->recursiveEdgeRelayout();
        }
    }
}

void GraphicsScene::toggleEdgeBoundingRects()
{
    LakosRelation::toggleBoundingRect();
    updateEdgeDebugInfo();
}

void GraphicsScene::toggleEdgeShapes()
{
    LakosRelation::toggleShape();
    updateEdgeDebugInfo();
}

void GraphicsScene::toggleEdgeTextualInformation()
{
    LakosRelation::toggleTextualInformation();
    updateEdgeDebugInfo();
}

void GraphicsScene::toggleEdgeIntersectionPaths()
{
    LakosRelation::toggleIntersectionPaths();
    updateEdgeDebugInfo();
}

void GraphicsScene::toggleEdgeOriginalLine()
{
    LakosRelation::toggleOriginalLine();
    updateEdgeDebugInfo();
}

void GraphicsScene::updateEdgeDebugInfo()
{
    for (auto *relation : d->relationVec) {
        relation->updateDebugInformation();
    }
}

LakosEntity *GraphicsScene::entityById(const std::string& uniqueId) const
{
    try {
        return d->vertices.at(uniqueId);
    } catch (...) {
        return nullptr;
    }
}

LakosEntity *GraphicsScene::entityByQualifiedName(const std::string& qualName) const
{
    const bool showDebug = Preferences::enableDebugOutput();

    if (d->verticesVec.empty()) {
        if (showDebug) {
            qDebug() << "There are no entities on the vector";
        }
        return nullptr;
    }

    const auto findIt = std::find_if(std::cbegin(d->verticesVec), std::cend(d->verticesVec), [&qualName](auto *entity) {
        return entity->qualifiedName() == qualName;
    });
    if (findIt == std::cend(d->verticesVec)) {
        if (showDebug) {
            qDebug() << "Could not find " << QString::fromStdString(qualName);
            qDebug() << "Available entities:";
            for (auto *entity : d->verticesVec) {
                qDebug() << "> " << QString::fromStdString(entity->qualifiedName());
            }
        }
        return nullptr;
    }

    return *findIt;
}
void GraphicsScene::loadEntitiesByQualifiedNameList(const QStringList& qualifiedNameList, const QPointF& pos)
{
    if (qualifiedNameList.isEmpty()) {
        return;
    }
    for (const auto& qualName : qualifiedNameList) {
        loadEntityByQualifiedName(qualName, pos);
    }
    d->transitiveReductionAlg->reset();
    searchTransitiveRelations();
}

void GraphicsScene::loadEntityByQualifiedName(const QString& qualifiedName, const QPointF& pos)
{
    const std::string qualName = qualifiedName.toStdString();
    qDebug() << "Loading" << qualName;
    if (entityByQualifiedName(qualName)) {
        Q_EMIT errorMessage(tr("The element is already loaded"));
        return;
    }

    auto *lakosianNode = d->nodeStorage.findByQualifiedName(qualName);
    if (!lakosianNode) {
        Q_EMIT errorMessage(tr("Element %1 not found").arg(qualifiedName));
        return;
    }

    assert(lakosianNode);

    LakosEntity *lastAddedEntity = nullptr;
    size_t parentIdx = -1;
    const auto hierarchy = lakosianNode->parentHierarchy();

    // Traverse the hierarchy to find the bottom-most of the items already on the view.
    // If there are already elements of the hierarchy of the items dropped, those
    // elements should be used as the drop target.
    for (size_t i = 0; i < hierarchy.size(); i++) {
        lvtldr::LakosianNode *thisNode = hierarchy[i];
        LakosEntity *entity = entityByQualifiedName(thisNode->qualifiedName());
        if (entity == nullptr) {
            break;
        }
        lastAddedEntity = entity;
        parentIdx = i;
    }

    // this will iterate first from the parents, then to the children. the last element is the one we
    // dragged to the view, but first, we need to create all of the parents on the view, with
    // the exception of the already created elements, calculated on the for above.
    // since parentIdx starts with -1, when there's no parents, we start the for below on zero
    // and everything should be good.
    std::vector<LakosEntity *> newEntities;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 100.0);
    QPointF scenePos = lastAddedEntity == nullptr ? pos : QPointF(dist(mt), dist(mt));

    for (size_t i = parentIdx + 1; i < hierarchy.size(); i++) {
        lvtldr::LakosianNode *node = hierarchy[i];

        // TODO: Remove the boolean traps.
        lvtshr::LoaderInfo info(false, lastAddedEntity != nullptr, false);
        info.setHasParent(lastAddedEntity != nullptr);

        lvtshr::DiagramType nodeType = node->type();
        auto methodPtr = [&]() -> decltype(&GraphicsScene::addUdtVertex) {
            switch (nodeType) {
            case lvtshr::DiagramType::ClassType:
                return &GraphicsScene::addUdtVertex;
            case lvtshr::DiagramType::ComponentType:
                return &GraphicsScene::addCompVertex;
            case lvtshr::DiagramType::PackageType:
                return &GraphicsScene::addPkgVertex;
            case lvtshr::DiagramType::RepositoryType:
                return &GraphicsScene::addRepositoryVertex;
            case lvtshr::DiagramType::FreeFunctionType:
                return &GraphicsScene::addUdtVertex;
            case lvtshr::DiagramType::NoneType:
                break;
            }
            return nullptr;
        }();
        assert(methodPtr);

        // the parameter after newPackageId has the name `selected`, but that actually serves to tell if this
        // entity will be a "graph" or a "leaf".
        LakosEntity *pkgEntity = (this->*methodPtr)(node, true, lastAddedEntity, info);

        pkgEntity->enableLayoutUpdates();
        pkgEntity->setPos(scenePos);
        pkgEntity->show();
        // When an item has no parent, it needs to be added to the view manually.
        if (!lastAddedEntity) {
            addItem(pkgEntity);
        } else {
            if (!lastAddedEntity->isExpanded()) {
                lastAddedEntity->toggleExpansion(QtcUtil::CreateUndoAction::e_No);
            } else {
                // HACK: Toggle expansion twice so it shrinks / expands to recalculate the rectangle.
                // the recalculateRectangle() method exists but triggering it just after adding the child
                // is not correctly setting up the boundaries.
                lastAddedEntity->toggleExpansion(QtcUtil::CreateUndoAction::e_No);
                lastAddedEntity->toggleExpansion(QtcUtil::CreateUndoAction::e_No);
            }
        }

        lastAddedEntity = pkgEntity;
        newEntities.push_back(pkgEntity);

        // Position the item inside of the previous element with a
        // bit of randomness so they don't stack vertically
        scenePos = QPointF(dist(mt), dist(mt));
    }

    // Add the edges between the entities that are currently loaded.
    std::unordered_map<LakosianNode *, LakosEntity *> entity_to_node;
    for (auto const& node : d->verticesVec) {
        entity_to_node[node->internalNode()] = node;
    }
    auto sceneContainsNode = [&entity_to_node](LakosianNode *node) {
        return entity_to_node.count(node) > 0;
    };
    for (auto *newEntity : newEntities) {
        auto *newEntityNode = newEntity->internalNode();
        for (const auto& edge : newEntityNode->providers()) {
            if (sceneContainsNode(edge.other())) {
                addEdgeBetween(newEntity, entity_to_node[edge.other()], edge.type());
            }
        }
        for (const auto& edge : newEntityNode->clients()) {
            if (sceneContainsNode(edge.other())) {
                addEdgeBetween(entity_to_node[edge.other()], newEntity, edge.type());
            }
        }
    }

    for (auto *newEntity : newEntities) {
        newEntity->recursiveEdgeRelayout();
    }

    Q_EMIT graphLoadFinished();
}

void GraphicsScene::addEdgeBetween(LakosEntity *fromEntity, LakosEntity *toEntity, lvtshr::LakosRelationType type)
{
    LakosRelation *relation = nullptr;
    switch (type) {
    // Package groups, packages and components
    case lvtshr::PackageDependency:
        relation = addPackageDependencyRelation(fromEntity, toEntity);
        break;
    // Logical entities
    case lvtshr::IsA:
        relation = addIsARelation(fromEntity, toEntity);
        break;
    case lvtshr::UsesInNameOnly:
        assert(false && "Not implemented");
        break;
    case lvtshr::UsesInTheImplementation:
        relation = addUsesInTheImplementationRelation(fromEntity, toEntity);
        break;
    case lvtshr::UsesInTheInterface:
        relation = addUsesInTheInterfaceRelation(fromEntity, toEntity);
        break;

    case lvtshr::None:
        assert(false && "Unexpected unknown relation type");
        break;
    }

    if (relation && !relation->parentItem()) {
        addItem(relation);
    }
}

lvtprj::ProjectFile const& GraphicsScene::projectFile() const
{
    return d->projectFile;
}

QJsonObject GraphicsScene::toJson() const
{
    // filter all toplevel items:
    QJsonArray array;

    for (LakosEntity *e : d->verticesVec) {
        if (!e->parentItem()) {
            array.append(e->toJson());
        }
    }
    return {
        {"x", sceneRect().x()},
        {"y", sceneRect().y()},
        {"width", sceneRect().width()},
        {"height", sceneRect().height()},
        {"elements", array},
        {"transitive_visibility", d->showTransitive},
    };
}

void recursiveJsonToLakosEntity(GraphicsScene *scene, const QJsonValue& entity)
{
    const QJsonObject obj = entity.toObject();
    const QString qualName = obj["qualifiedName"].toString();
    const QJsonObject posObj = obj["pos"].toObject();
    const QPointF pos(posObj["x"].toDouble(), posObj["y"].toDouble());

    scene->loadEntityByQualifiedName(qualName, pos);

    // TODO: return the entity directly by the above method.
    LakosEntity *thisObj = scene->entityByQualifiedName(qualName.toStdString());
    if (!thisObj) {
        return;
    }
    thisObj->fromJson(obj);
    thisObj->enableLayoutUpdates();

    const auto children = obj["children"].toArray();
    for (const auto& child : children) {
        recursiveJsonToLakosEntity(scene, child);
    }

    thisObj->setPos(pos);
}

void GraphicsScene::fromJson(const QJsonObject& doc)
{
    // Hack. if we add elements to the GraphiscScene, it will
    // trigger a repaint, and we might be adding *thousands* of edges
    // here. because edges have a big boundingRect, this will trigger
    // a repaint and cache invalidation of the view *thousands* of times.
    // making loading huge graphs quite slow.
    // so we hide the viewport, it will not try to paint anything untill
    // we set it to visible again.
    // before this change: Load From Jason Took: 1778ms
    // after this change: Load from Json Took: 230ms
    views().at(0)->viewport()->setVisible(false);

    Q_EMIT graphLoadStarted();
    clearGraph();

    const auto x = doc["x"].toInt();
    const auto y = doc["y"].toInt();
    const auto w = doc["width"].toInt();
    const auto h = doc["height"].toInt();

    setSceneRect(x, y, w, h);

    const auto elements = doc["elements"].toArray();

    for (const auto& element : elements) {
        recursiveJsonToLakosEntity(this, element);
    }

    // Invalidate transitive reduction caches
    d->transitiveReductionAlg->reset();
    searchTransitiveRelations();

    // Calculate Default Visibility of edges
    for (auto entity : d->verticesVec) {
        if (!entity->parentItem()) {
            entity->calculateEdgeVisibility();
        }
    }

    const auto show_transitive = doc["transitive_visibility"].toBool();
    toggleTransitiveRelationVisibility(show_transitive);
    Q_EMIT graphLoadFinished();

    views().at(0)->viewport()->setVisible(true);

    // Do not remove this, this is a hack to fix the edges pointing to nowhere.
    // Or Better, Remove this when this hack is not needed anymore.
    // it does not really matter that the strings are slightly incorrect,
    // but they allow us to not show garbage to the user, and that matters more.
    // The error appears to be that the initial setup contains bogus information
    // about the viewport because we might position things outside of the view
    // area.
    // the Event Loop is not helping because it calculates things while the view
    // is not visible yet, so I need to halt this method using something that
    // awaits for user interaction. when the user *sees* this, it means that the
    // view is ok, and I can continue with the collapse / expand.
    // the relayout() call below is currently needed but might be easier to remove
    // than the start of this hack.
    // not really proud of this code, but, hey, it works.
    QTimer::singleShot(std::chrono::seconds{1}, [this] {
        collapseToplevelEntities();
        expandToplevelEntities();

        QTimer::singleShot(std::chrono::seconds{1}, [this] {
            reLayout();
        });
    });
}

void GraphicsScene::setPluginManager(Codethink::lvtplg::PluginManager& pm)
{
    d->pluginManager = pm;
    for (auto *e : allEntities()) {
        e->setPluginManager(pm);
    }
}

void GraphicsScene::removeEdge(LakosEntity& fromEntity, LakosEntity& toEntity)
{
    auto edgeCollection = fromEntity.getRelationshipWith(&toEntity);
    if (!edgeCollection || edgeCollection->relations().empty()) {
        return;
    }

    // explicit copy, so we don't mess with internal iterators.
    auto allCollections = fromEntity.edgesCollection();

    std::vector<LakosRelation *> toDelete;
    for (const auto& collection : allCollections) {
        if (collection->to() == &toEntity) {
            for (auto *relation : collection->relations()) {
                fromEntity.removeEdge(relation);
                toEntity.removeEdge(relation);
                toDelete.push_back(relation);
            }
        }
    }

    for (LakosRelation *rel : toDelete) {
        rel->setParent(nullptr);
        removeItem(rel);
        d->relationVec.erase(std::remove(std::begin(d->relationVec), std::end(d->relationVec), rel),
                             std::end(d->relationVec));

        rel->deleteLater();
    }

    // Invalidate transitive reduction caches
    d->transitiveReductionAlg->reset();

    fromEntity.getTopLevelParent()->calculateEdgeVisibility();
    fromEntity.recursiveEdgeRelayout();
}

} // end namespace Codethink::lvtqtc
