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

#include <any>
#include <ct_lvtqtc_graphicsscene.h>

#include <ct_lvtmdl_circular_relationships_model.h>

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
#include <QGraphicsRectItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include <QLoggingCategory>
#include <QMenu>
#include <QScreen>
#include <QTextBrowser>
#include <QTimer>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <fstream>
#include <random>

using namespace Codethink::lvtldr;

namespace {

// https://doc.qt.io/qt-5/qcoreapplication.html#processEvents-1
// QCoreApplication::processEvents will keep processing events until there are
// no more events to process or until a timeout occurs (measured in milliseconds).
constexpr int PROCESS_EVENTS_TIMEOUT_MS = 100;

} // namespace

namespace Codethink::lvtqtc {

// TODO: Move this struct outside of GraphicsScene.
// It would simplify the GraphicsScene code and make things
// easier on the long run.
struct GraphConfigurationData {
    // Those are the settings for the graph that's being currently drawn.

    lvtshr::ClassView classView = lvtshr::ClassView::ClassHierarchyGraph;
    // Tells how should we display classes

    lvtshr::ClassScope classScope = lvtshr::ClassScope::All;
    // Tells how the classes should be scoped.

    int relationTypeFlags = 0;
    // receives flags based on lvtshr::LakosRelationType

    int traversalLimit = 0;
    // number of times the graph should traverse a node

    int relationLimit = 0;
    // maximum number of relationships.

    bool showExternalDepEdges = false;
    // control if we show cross-subgraph edges between dependencies in the
    // package graph

    lvtshr::DiagramType diagramType = lvtshr::DiagramType::NoneType;
    // Package or class diagram

    QString fullyQualifiedName;
    // Fully Qualified name of the displayed class

    GraphConfigurationData() = default;

    GraphConfigurationData(const GraphConfigurationData& other) = default;
    // defaulted copy constructor

    bool operator==(const GraphConfigurationData& other) const
    {
        return classView == other.classView && classScope == other.classScope
            && relationTypeFlags == other.relationTypeFlags && traversalLimit == other.traversalLimit
            && relationLimit == other.relationLimit && showExternalDepEdges == other.showExternalDepEdges
            && diagramType == other.diagramType && fullyQualifiedName == other.fullyQualifiedName;
    }
    // non defaulted operator overload for equality
};

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

    bool strict = false;
    // Controls how forgiving we are of errors
    // should mostly be used on unit tests where an error should fail
    // the test, but on desktop we can ignore some of the errors.

    GraphConfigurationData graphData;
    // Data of the graph that's being displayed on the scene.
    // Those are the elements we can change via interaction with the UI.

    bool blockNodeResizeOnHover = false;
    // blocks mouseHoverEvent resizing the nodes with this flag on.

    LoadFlags flags = LoadFlags::Idle;
    // flags to indicate success of parts of the load / layout algorithm.

    Codethink::lvtmdl::CircularRelationshipsModel *circleModel = nullptr;
    // model to display the circle relationships.

    LakosEntity *mainEntity = nullptr;
    // The mainEntity is the entity that was first loaded on the view.

    LakosEntity *selectedEntity = nullptr;
    // The selected entity is chosen by the user by selecting it on the view.

    lvtldr::NodeStorage& nodeStorage;

    bool skipPannelCollapse = false;

    bool showTransitive = false;
    // Show all transitive edges on the top level elements.
    // up to the children. when this flag changes, all children will
    // also set their showTransitive status. but modifying a child
    // won't change this.

    bool isEditMode = false;
    // if in edit mode, we are not allowing navigation.
    // if in not edit mode, we dont allow edit.

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

    using Codethink::lvtmdl::CircularRelationshipsModel;
    d->circleModel = new CircularRelationshipsModel(this);
    connect(d->circleModel, &QAbstractItemModel::dataChanged, this, &GraphicsScene::highlightCyclesOnCicleModelChanged);
    connect(d->circleModel,
            &CircularRelationshipsModel::relationshipsUpdated,
            this,
            &GraphicsScene::resetHighlightedCycles);

    d->transitiveReductionAlg = new AlgorithmTransitiveReduction();

    setLoadFlags(Idle);
    d->physicalLoader.setGraph(this);
    d->physicalLoader.setExtDeps(d->graphData.showExternalDepEdges);

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

            if (parent->isCovered()) {
                parent->toggleCover(PackageEntity::ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
            }
        }

        if (!hasMainNodeSelected()) {
            setMainNode(QString::fromStdString(entity->qualifiedName()), entity->instanceType());
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
                     [this](LakosianNode *source, LakosianNode *target, PhysicalDependencyType type) {
                         auto *fromEntity = findLakosEntityFromUid(source->uid());
                         auto *toEntity = findLakosEntityFromUid(target->uid());
                         if (!fromEntity || !toEntity) {
                             // This Graphics Scene doesn't have such entities to update
                             return;
                         }
                         switch (type) {
                         case PhysicalDependencyType::ConcreteDependency: {
                             addEdgeBetween(fromEntity, toEntity, lvtshr::LakosRelationType::PackageDependency);
                             break;
                         }
                         case PhysicalDependencyType::AllowedDependency: {
                             addEdgeBetween(fromEntity, toEntity, lvtshr::LakosRelationType::AllowedDependency);
                             break;
                         }
                         }

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
                             removeItem(entity);
                         } else if (newParentEntity) {
                             // Setting the new parent will automatically update the old parent if it is in this scene.
                             entity->setParentItem(newParentEntity);
                         }
                     });
}

GraphicsScene::~GraphicsScene() noexcept = default;

lvtshr::DiagramType GraphicsScene::diagramType() const
{
    return d->graphData.diagramType;
}

void GraphicsScene::visualizationModeTriggered()
{
    d->isEditMode = false;
}

void GraphicsScene::editModeTriggered()
{
    d->isEditMode = true;
}

bool GraphicsScene::isEditMode()
{
    return d->isEditMode;
}

QString GraphicsScene::qualifiedName() const
{
    return d->graphData.fullyQualifiedName;
}

Codethink::lvtmdl::CircularRelationshipsModel *GraphicsScene::circularRelationshipsModel() const
{
    return d->circleModel;
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

void GraphicsScene::setClassView(lvtshr::ClassView classView)
{
    if (d->graphData.classView == classView) {
        return;
    }
    d->graphData.classView = classView;
    updateGraph();
}

void GraphicsScene::setScope(lvtshr::ClassScope classScope)
{
    if (d->graphData.classScope == classScope) {
        return;
    }
    d->graphData.classScope = classScope;
    updateGraph();
}

void GraphicsScene::setTraversalLimit(int limit)
{
    if (d->graphData.traversalLimit == limit) {
        return;
    }
    d->graphData.traversalLimit = limit;
    updateGraph();
}

void GraphicsScene::setRelationLimit(int limit)
{
    if (d->graphData.relationLimit == limit) {
        return;
    }
    d->graphData.relationLimit = limit;
    updateGraph();
}

void GraphicsScene::setShowExternalDepEdges(bool showExternalDepEdges)
{
    if (d->graphData.showExternalDepEdges == showExternalDepEdges) {
        return;
    }
    d->graphData.showExternalDepEdges = showExternalDepEdges;
    d->physicalLoader.setExtDeps(showExternalDepEdges);
    updateGraph();
}

bool GraphicsScene::hasMainNodeSelected() const
{
    return d->graphData.diagramType != lvtshr::DiagramType::NoneType;
}

void GraphicsScene::setMainNode(const QString& fullyQualifiedName, lvtshr::DiagramType type)
{
    // we just requested to load a new graph.
    // perhaps this graph that we are looking right now
    // is modified, we need to save it before loading another graph.
    if (d->graphData.fullyQualifiedName == fullyQualifiedName && d->graphData.diagramType == type
        && !d->vertices.empty()) {
        return;
    }

    d->graphData.fullyQualifiedName = fullyQualifiedName;
    d->graphData.diagramType = type;

    auto *node = d->nodeStorage.findByQualifiedName(type, fullyQualifiedName.toStdString());
    d->physicalLoader.setMainNode(node);

    updateGraph();

    d->circleModel->setCircularRelationships({});
    Q_EMIT mainNodeChanged(d->mainEntity);
}

void GraphicsScene::setStrictMode(bool strict)
{
    d->strict = strict;
}

bool GraphicsScene::isReady()
{
    return !d->graphData.fullyQualifiedName.isEmpty();
}

void GraphicsScene::collapseSecondaryEntities()
{
    for (LakosEntity *entity : d->verticesVec) {
        if (entity->parentItem() == nullptr && entity != d->mainEntity) {
            entity->shrink(QtcUtil::CreateUndoAction::e_No);
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
    d->mainEntity = nullptr;
    d->showTransitive = Preferences::showRedundantEdgesDefault();
    d->vertices.clear();
    d->verticesVec.clear();
    d->relationVec.clear();
    d->entityLoadFlags.clear();
    d->transitiveReductionAlg->reset();

    clear();
    if (Preferences::enableDebugOutput()) {
        qDebug() << "Graph Cleared!";
    }
}

void GraphicsScene::updateGraph()
{
    if (Preferences::enableDebugOutput()) {
        qDebug() << "Reloading the Graph";
    }

    if (d->flags == Running) {
        Q_EMIT refuseToLoad(d->graphData.fullyQualifiedName);
        return;
    }
    clearGraph();
    setLoadFlags(Running);
    Q_EMIT graphLoadStarted();
    Q_EMIT d->circleModel->initialState();

    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Start);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);

    if (!isReady()) {
        setLoadFlags(NotReady);
        Q_EMIT graphLoadFinished();
        return;
    }

    if (requestDataFromDatabase() == CodeDbLoadStatus::Error) {
        // stop graph load
        return;
    }

    layoutVertices();
    finalizeLayout();
}

void GraphicsScene::relayout()
{
    if (d->flags == Running) {
        Q_EMIT refuseToLoad(d->graphData.fullyQualifiedName);
        return;
    }

    setLoadFlags(Running);
    Q_EMIT graphLoadStarted();

    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Start);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);

    if (!isReady()) {
        setLoadFlags(NotReady);
        Q_EMIT graphLoadFinished();
        return;
    }

    runLayoutAlgorithm();
    for (LakosEntity *entity : d->verticesVec) {
        entity->enableLayoutUpdates();
    }
    layoutDone();
}

void GraphicsScene::setMainEntity(LakosEntity *entity)
{
    d->mainEntity = entity;
    entity->setMainEntity();
}

LakosEntity *GraphicsScene::mainEntity() const
{
    return d->mainEntity;
}

namespace {

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
        if (d->selectedEntity) {
            d->selectedEntity->setSelected(false);
            d->selectedEntity->updateZLevel();
        }

        auto *oldSelectedEntity = d->selectedEntity;
        d->selectedEntity = nullptr;
        if (entity != oldSelectedEntity) {
            d->selectedEntity = entity;
            d->selectedEntity->setSelected(true);
            d->selectedEntity->updateZLevel();
        }
        Q_EMIT scene->selectedEntityChanged(d->selectedEntity);
    });

    QObject::connect(entity, &LakosEntity::requestRemoval, scene, [scene, &nodeStorage, node, entity] {
        auto *view = qobject_cast<GraphicsView *>(scene->views().constFirst());
        auto name = node->name();
        auto qualifiedName = node->qualifiedName();
        auto parentQualifiedName = node->parent() ? node->parent()->qualifiedName() : "";

        if (node->type() == lvtshr::DiagramType::PackageType) {
            auto err = nodeStorage.removePackage(node);
            if (err.has_error()) {
                switch (err.error().kind) {
                case lvtldr::ErrorRemovePackage::Kind::CannotRemovePackageWithProviders: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove packages with connected with other packages, break the connections "
                        "first."));
                    return;
                }
                case lvtldr::ErrorRemovePackage::Kind::CannotRemovePackageWithClients: {
                    Q_EMIT scene->errorMessage(
                        QObject::tr("Currently we can't remove packages with clients, break the connections first."));
                    return;
                }
                case lvtldr::ErrorRemovePackage::Kind::CannotRemovePackageWithChildren: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove packages that contains children, remove the childs first."));
                    return;
                }
                }
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
                switch (err.error().kind) {
                case lvtldr::ErrorRemoveComponent::Kind::CannotRemoveComponentWithProviders: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove components with connected with other components, break the "
                        "connections first."));
                    return;
                }
                case lvtldr::ErrorRemoveComponent::Kind::CannotRemoveComponentWithClients: {
                    Q_EMIT scene->errorMessage(
                        QObject::tr("Currently we can't remove components with clients, break the "
                                    "connections first."));
                    return;
                }
                case lvtldr::ErrorRemoveComponent::Kind::CannotRemoveComponentWithChildren: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove components that contains children, remove the childs first."));
                    return;
                }
                }
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
                switch (err.error().kind) {
                case lvtldr::ErrorRemoveLogicalEntity::Kind::CannotRemoveUDTWithProviders: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove logical entities connected with other entities, break the "
                        "connections first."));
                    return;
                }
                case lvtldr::ErrorRemoveLogicalEntity::Kind::CannotRemoveUDTWithClients: {
                    Q_EMIT scene->errorMessage(
                        QObject::tr("Currently we can't remove logical entities with clients, break the connections "
                                    "first."));
                    return;
                }
                case lvtldr::ErrorRemoveLogicalEntity::Kind::CannotRemoveUDTWithChildren: {
                    Q_EMIT scene->errorMessage(QObject::tr(
                        "Currently we can't remove logical entities that contains children, remove the inner childs "
                        "first."));
                    return;
                }
                }
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

    if (d->graphData.fullyQualifiedName == QString::fromStdString(node->qualifiedName())) {
        scene->setMainEntity(entity);
    }

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
        auto ec = source->getRelationshipWith(target);
        for (auto *r : ec->relations()) {
            auto *pkgRelation = dynamic_cast<PackageDependency *>(r);
            assert(pkgRelation);
            pkgRelation->updateFlavor();
        }
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

LakosRelation *GraphicsScene::addAllowedPackageDependencyRelation(LakosEntity *source, LakosEntity *target)
{
    assert(source && source->instanceType() != lvtshr::DiagramType::ClassType);
    assert(target && target->instanceType() != lvtshr::DiagramType::ClassType);

    if (source->hasRelationshipWith(target)) {
        auto ec = source->getRelationshipWith(target);
        for (auto *r : ec->relations()) {
            auto *pkgRelation = dynamic_cast<PackageDependency *>(r);
            assert(pkgRelation);
            pkgRelation->updateFlavor();
        }
        return nullptr;
    }

    auto *r = addRelation(new PackageDependency(source, target));
    if (r) {
        addItem(r);
        r->show();
    }
    return r;
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

void GraphicsScene::layoutVertices()
{
    if (d->flags == UserAborted) {
        setLoadFlags(Idle);
        return;
    }

    runLayoutAlgorithm();
}

void GraphicsScene::runLayoutAlgorithm()
{
    if (d->flags == UserAborted) {
        setLoadFlags(Idle);
        return;
    }

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
    runLevelizationLayout(entityToLevel, LakosEntity::LevelizationLayoutType::Vertical, direction);

    setLoadFlags(Success);
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

    // Tells the system tha the graph updated.
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

void GraphicsScene::setLoadFlags(GraphicsScene::LoadFlags flags)
{
    d->flags = flags;
}

// This method is only called for the "main" node. The subsequent database
// calls are done via the "load children", "load clients" and "load providers".
GraphicsScene::CodeDbLoadStatus GraphicsScene::requestDataFromDatabase()
{
    if (d->flags == UserAborted) {
        setLoadFlags(Idle);
        finalizeLayout();
        return CodeDbLoadStatus::Error;
    }

    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::CdbLoad);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);

    bool success = false;
    switch (d->graphData.diagramType) {
    case lvtshr::DiagramType::NoneType:
        if (Preferences::enableDebugOutput()) {
            qWarning() << "Database corrupted";
        }
        success = false;
        break;
    case lvtshr::DiagramType::RepositoryType:
        [[fallthrough]];
    case lvtshr::DiagramType::ClassType:
        [[fallthrough]];
    case lvtshr::DiagramType::PackageType:
        [[fallthrough]];
    case lvtshr::DiagramType::ComponentType:
        auto *node =
            d->nodeStorage.findByQualifiedName(d->graphData.diagramType, d->graphData.fullyQualifiedName.toStdString());

        if (!node) {
            success = false;
            break;
        }

        lvtldr::NodeLoadFlags flags;
        if (d->graphData.fullyQualifiedName.toStdString() == node->qualifiedName()) {
            flags.traverseClients = Preferences::showClients();
            flags.traverseProviders = Preferences::showProviders();
            flags.loadChildren = true;
        }

        d->physicalLoader.clear();
        success = d->physicalLoader.loadV2(node, flags).has_value();
        break;
    }

    if (!success) {
        if (Preferences::enableDebugOutput()) {
            qWarning() << d->graphData.fullyQualifiedName << "not found in package database";
        }
        setLoadFlags(LoadFromDbError);
        finalizeLayout();
        return CodeDbLoadStatus::Error;
    }

    return CodeDbLoadStatus::Success;
}

lvtldr::NodeLoadFlags GraphicsScene::loadFlagsFor(lvtldr::LakosianNode *node) const
{
    const auto search = d->entityLoadFlags.find(node);
    if (search != d->entityLoadFlags.end()) {
        return search->second;
    }

    return NodeLoadFlags{};
}

void GraphicsScene::dumpScene(const QList<QGraphicsItem *>& items)
{
    for (auto *item : items) {
        dumpScene(item, 0);
    }
}

void GraphicsScene::dumpScene(QGraphicsItem *item, int indent)
{
    if (!Preferences::enableDebugOutput()) {
        return;
    }

    if (!item) {
        // qAsConst<QList<QGraphicsItem*>> has been explicitly deleted
        for (const auto& i : items()) { // clazy:exclude=range-loop,range-loop-detach
            if (i->parentItem() == nullptr) {
                dumpScene(i, indent);
            }
        }
        return;
    }

    auto output_data = [this, item, indent](LakosEntity *entity) {
        auto relations = entity->edgesCollection();
        if (!relations.empty()) {
            qDebug() << std::string(indent, ' ') << " Relations";
            for (const auto& edge : entity->edgesCollection()) {
                qDebug() << std::string(indent, ' ') << "From:" << edge->from()->qualifiedName();
                qDebug() << std::string(indent, ' ') << "To: " << edge->to()->qualifiedName();
            }
        }
        // qAsConst<QList<QGraphicsItem*>> has been explicitly deleted
        for (QGraphicsItem *c : item->childItems()) { // clazy:exclude=range-loop,range-loop-detach
            if (auto *childentity = qgraphicsitem_cast<LakosEntity *>(c)) {
                dumpScene(childentity, indent + 4);
            }
        }
    };

    if (auto *package = dynamic_cast<PackageEntity *>(item)) {
        qDebug() << std::string(indent, ' ') << " Package: " << package->qualifiedName();
        output_data(package);
        return;
    }

    if (auto *component = dynamic_cast<ComponentEntity *>(item)) {
        qDebug() << std::string(indent, ' ') << " Component: " << component->qualifiedName();
        output_data(component);
        return;
    }

    if (auto *classPtr = qgraphicsitem_cast<LakosEntity *>(item)) {
        qDebug() << std::string(indent, ' ') << " Class: " << classPtr->qualifiedName();
        output_data(classPtr);
        return;
    }
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

void GraphicsScene::finalizeLayout()
{
    const QString ourErrorMessage = fetchErrorMessage();
    if (!ourErrorMessage.isEmpty()) {
        if (Preferences::enableDebugOutput()) {
            qDebug() << "Finalized with" << ourErrorMessage;
        }
    }

    if (d->flags != Success) {
        clearGraph();
        Q_EMIT errorMessage(ourErrorMessage);
    } else {
        // we only need to add the toplevel items to the scene,
        // as the inner items are added automatically.
        for (auto *item : d->verticesVec) {
            if (item->parentItem()) {
                continue;
            }

            addItem(item);
        }

        // some vertices are child items, and we can't add them
        // to the scene yet, but we don't know the order.
        for (auto *item : d->relationVec) {
            if (item->scene()) {
                continue;
            }

            addItem(item);
        }
    }

    fixRelations();
    edgesContainersLayout();
    if (!d->showTransitive) {
        transitiveReduction();
    }
    layoutDone();
}

void GraphicsScene::fixRelations()
{
    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::FixRelations);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);

    fixRelationsParentRelationship();
}

void GraphicsScene::edgesContainersLayout()
{
    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::EdgesContainersLayout);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);

    // tell all entities that we are done loading the scene and they should run
    // their layout stuff
    for (LakosEntity *entity : d->verticesVec) {
        entity->enableLayoutUpdates();
    }
}

void GraphicsScene::transitiveReduction()
{
    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::TransitiveReduction);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);
    searchTransitiveRelations();
}

void GraphicsScene::layoutDone()
{
    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Done);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_TIMEOUT_MS);
    Q_EMIT graphLoadFinished();
    reLayout();
}

GraphicsScene::LoadFlags GraphicsScene::loadFlags() const
{
    return d->flags;
}

QString GraphicsScene::fetchErrorMessage() const
{
    switch (d->flags) {
    case Idle:
        return tr("Graph Layout finished but the status is Idle instead of success.");
    case Running:
        return tr("Graph Layout finished but the status is Running instead of success.");
    case NotReady:
        return tr("Graph Layout was executed with the status being Not Ready.");
    case LoadFromCacheDbError:
        return tr("There was an error loading from the Cache Database");
    case LoadFromDbError:
        return tr("There was an error loading from the LLVM Database");
    case LayoutNodesError:
        return tr("There was an error laying out the nodes");
    case LayoutEdgesError:
        return tr("There was an error laying out the edges");
    case LayoutGvzError:
        return tr("There was an error running the internal cache database, contact the developers.");
    case LayoutGvzDimensionsError:
        return tr("There was an error with the Dimensions on the cache database, contact the developers.");
    case LayoutGvzPositionError:
        return tr("There was an error with the positions on the cache database, contact the developers.");
    case LayoutGvzLPosError:
        return tr("There was an error with the field LPos on the cache database, contact the developers.");
    case LayoutEnhancedDotError:
        return tr("There was an error running graphviz, contact the developers");
    case SaveGraphError:
        return tr("There was an error saving the cache database, verify your filesystem permissions");
    case UserAborted:
        return tr("User aborted the generation of the graph");
    case Success:
        break;
    }
    return {};
}

} // namespace Codethink::lvtqtc

namespace Codethink::lvtqtc {

void GraphicsScene::toggleIdentifyCycles()
{
    if (!d->mainEntity) {
        return;
    }

    // TODO: identify cycles
}

std::vector<LakosEntity *>& GraphicsScene::allEntities() const
{
    return d->verticesVec;
}

void GraphicsScene::connectEntitySignals(LakosEntity *entity)
{
    assert(entity);

    const std::string qualifiedName = entity->qualifiedName();
    const lvtshr::DiagramType type = entity->instanceType();

    connect(entity, &LogicalEntity::navigateRequested, this, [this, qualifiedName, type] {
        QString qname = QString::fromStdString(qualifiedName);
        switch (type) {
        case lvtshr::DiagramType::ClassType:
            Q_EMIT classNavigateRequested(qname);
            break;
        case lvtshr::DiagramType::ComponentType:
            Q_EMIT componentNavigateRequested(qname);
            break;
        case lvtshr::DiagramType::PackageType:
            Q_EMIT packageNavigateRequested(qname);
            break;
        default:
            break;
        }
    });

    connect(entity, &LakosEntity::undoCommandCreated, this, [this](QUndoCommand *command) {
        auto *view = qobject_cast<GraphicsView *>(views().constFirst());
        assert(view);
        view->undoCommandReceived(command);
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

    connect(entity, &LakosEntity::loadClients, this, [entity, this] {
        auto *node = entity->internalNode();

        auto& flags = d->entityLoadFlags[node];
        flags.traverseClients = true;

        finalizeEntityPartialLoad(entity);
    });

    connect(entity, &LakosEntity::loadProviders, this, [entity, this] {
        auto *node = entity->internalNode();

        auto& flags = d->entityLoadFlags[node];
        flags.traverseProviders = true;

        finalizeEntityPartialLoad(entity);
    });

    connect(entity, &LakosEntity::coverChanged, this, [this, entity]() {
        if (entity->isCovered()) {
            return;
        }

        d->transitiveReductionAlg->reset();
        searchTransitiveRelations();
        transitiveRelationSearchFinished();
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
        Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::VertexLayout);
        reLayout();
        Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Done);
    }
}

void GraphicsScene::unloadEntity(lvtshr::UniqueId uuid, UnloadDepth depth)
{
    LakosEntity *entity = findLakosEntityFromUid(uuid);
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

    d->transitiveReductionAlg->reset();
    searchTransitiveRelations();
    reLayout();
    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Done);
}

void GraphicsScene::unloadEntity(LakosEntity *entity)
{
    // if this item is the mainEntity, do not remove it or it's children.
    // removing children still works if the "Remove Children" action
    // is selected, this just blocks automatic removal of the main node
    if (entity == d->mainEntity) {
        return;
    }

    // This for *must* be run between the mainEntity check, and the parent of
    // the mainEntity check.
    const auto entities = entity->lakosEntities();
    for (auto *child : entities) {
        unloadEntity(child);
    }

    // if this item is the parent of the mainEntity, do not remove it.
    if (d->mainEntity) {
        const QList<LakosEntity *> parentItems = d->mainEntity->parentHierarchy();
        auto findIt = std::find(std::begin(parentItems), std::end(parentItems), entity);
        if (findIt != std::end(parentItems)) {
            return;
        }
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

    if (d->mainEntity == entity) {
        d->mainEntity = nullptr;
        d->physicalLoader.setMainNode(nullptr);
    }

    qDebug() << "Unloading entity" << intptr_t(entity) << QString::fromStdString(entity->name());
    delete entity;
}

void GraphicsScene::finalizeEntityPartialLoad(LakosEntity *entity)
{
    auto *node = entity->internalNode();
    auto flags = d->entityLoadFlags[node];

    bool success = d->physicalLoader.loadV2(node, flags).has_value();
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

    fixRelations();

    entity->calculateEdgeVisibility();
    reLayout();

    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Done);
}

void GraphicsScene::populateMenu(QMenu& menu, QMenu *debugMenu)
{
    using namespace Codethink::lvtplg;

    if (d->pluginManager) {
        auto getAllEntitiesInCurrentView = [this]() {
            std::vector<Entity> entitiesInView{};
            for (auto *e : allEntities()) {
                entitiesInView.emplace_back(createWrappedEntityFromLakosEntity(e));
            }
            return entitiesInView;
        };
        auto getEntityByQualifiedName = [this](std::string const& qualifiedName) -> std::optional<Entity> {
            auto *e = entityByQualifiedName(qualifiedName);
            if (!e) {
                return std::nullopt;
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
        using ctxMenuAction_f = PluginContextMenuHandler::ctxMenuAction_f;
        auto registerContextMenu = [=, this, &menu](std::string const& title, ctxMenuAction_f const& userAction) {
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
                auto runQueryOnDatabase = [this](std::string const& dbQuery) -> std::vector<std::vector<RawDBData>> {
                    return lvtmdb::SociHelper::runSingleQuery(d->nodeStorage.getSession(), dbQuery);
                };
                auto handler = PluginContextMenuActionHandler{getPluginData,
                                                              getAllEntitiesInCurrentView,
                                                              getEntityByQualifiedName,
                                                              getTree,
                                                              getEdgeByQualifiedName,
                                                              loadEntityByQualifiedName,
                                                              addEdgeByQualifiedName,
                                                              removeEdgeByQualifiedName,
                                                              hasEdgeByQualifiedName,
                                                              runQueryOnDatabase};
                userAction(&handler);
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
        auto *action = menu.addAction(tr("Collapse Entities"));
        connect(action, &QAction::triggered, this, [this] {
            collapseSecondaryEntities();
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

void GraphicsScene::updateBoundingRect()
{
    setSceneRect(itemsBoundingRect().adjusted(-20, -20, 20, 20));
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
    for (auto *relation : d->relationVec) {
        relation->updateDebugInformation();
    }
}

void GraphicsScene::toggleEdgeShapes()
{
    LakosRelation::toggleShape();
    for (auto *relation : d->relationVec) {
        relation->updateDebugInformation();
    }
}

void GraphicsScene::toggleEdgeTextualInformation()
{
    LakosRelation::toggleTextualInformation();
    for (auto *relation : d->relationVec) {
        relation->updateDebugInformation();
    }
}

void GraphicsScene::toggleEdgeIntersectionPaths()
{
    LakosRelation::toggleIntersectionPaths();
    for (auto *relation : d->relationVec) {
        relation->updateDebugInformation();
    }
}

void GraphicsScene::toggleEdgeOriginalLine()
{
    LakosRelation::toggleOriginalLine();
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
        Q_EMIT errorMessage(tr("Element not found"));
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
}

void GraphicsScene::addEdgeBetween(LakosEntity *fromEntity, LakosEntity *toEntity, lvtshr::LakosRelationType type)
{
    LakosRelation *relation = nullptr;
    switch (type) {
    // Package groups, packages and components
    case lvtshr::PackageDependency:
        relation = addPackageDependencyRelation(fromEntity, toEntity);
        break;
    case lvtshr::AllowedDependency:
        relation = addAllowedPackageDependencyRelation(fromEntity, toEntity);
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

    return {{"elements", array}};
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
    clearGraph();

    const auto elements = doc["elements"].toArray();

    for (const auto& element : elements) {
        recursiveJsonToLakosEntity(this, element);
    }

    // TODO: Do we need to trigger a relayout or just update the edges?
    // All the elements have pos set already.
    edgesContainersLayout();
    // This method seems not to do what it says that it does. should we remove it?

    reLayout();
    // Probably we don't need this. I'm just being safe.

    Q_EMIT graphLoadProgressUpdate(GraphLoadProgress::Done);
}

void GraphicsScene::highlightCyclesOnCicleModelChanged(const QModelIndex& _, const QModelIndex& index)
{
    using DataModel = Codethink::lvtmdl::CircularRelationshipsModel;

    if (!index.isValid() || index.parent() != QModelIndex()) {
        return;
    }

    auto cyclePathAsList = index.data(DataModel::CyclePathAsListRole).value<QList<QString>>();
    if (cyclePathAsList.size() < 2) {
        return;
    }
    // "Complete" the cycle by adding the first element as the last. This is just to make the
    // algorithm below work without having to add special case to the last element.
    cyclePathAsList.push_back(cyclePathAsList[0]);

    auto& entities = allEntities();
    auto findEntityByNameOnCurrentScene = [&](auto&& name) -> LakosEntity * {
        auto it = std::find_if(entities.begin(), entities.end(), [&](auto&& e) {
            return e->name() == name;
        });
        if (it == entities.end()) {
            return nullptr;
        }
        return *it;
    };

    auto isEdgeValid = [&](auto const& e) {
        auto relations = e.relations();
        return std::all_of(relations.begin(), relations.end(), [this](auto&& r) {
            if (!r->isVisible() || r->shouldBeHidden()) {
                Q_EMIT errorMessage(QObject::tr(
                    "One or more entities in the selected cycle is not visible in the current scene, so the path "
                    "cannot be highlighted - Consider using 'show redundant edges' option."));
                return false;
            }
            return true;
        });
    };
    auto isEntityValid = [&](auto *e) {
        if (!e) {
            Q_EMIT errorMessage(QObject::tr(
                "One or more entities in the selected cycle is not present in the current scene, so the path "
                "cannot be highlighted."));
            return false;
        }
        return true;
    };

    auto edgesToHighlight = std::vector<std::reference_wrapper<EdgeCollection>>{};
    auto nodesToHighlight = std::vector<std::reference_wrapper<LakosEntity>>{};
    auto *currentEntity = findEntityByNameOnCurrentScene(cyclePathAsList[0].toStdString());
    if (!isEntityValid(currentEntity)) {
        return;
    }
    nodesToHighlight.emplace_back(*currentEntity);
    for (int i = 1; i < cyclePathAsList.size(); ++i) {
        auto edges = currentEntity->edgesCollection();
        for (auto&& edge : edges) {
            if (!isEdgeValid(*edge)) {
                return;
            }
            if (edge->to()->name() == cyclePathAsList[i].toStdString()) {
                edgesToHighlight.emplace_back(*edge);
                break;
            }
        }
        currentEntity = findEntityByNameOnCurrentScene(cyclePathAsList[i].toStdString());
        if (!isEntityValid(currentEntity)) {
            return;
        }
        nodesToHighlight.emplace_back(*currentEntity);
    }

    auto toggle = index.data(Qt::CheckStateRole).toBool();
    for (auto&& edge : edgesToHighlight) {
        edge.get().toggleRelationFlags(EdgeCollection::RelationFlags::RelationIsHighlighted, toggle);
    }
    for (auto&& node : nodesToHighlight) {
        node.get().setPresentationFlags(LakosEntity::PresentationFlags::Highlighted, toggle);
    }
}

void GraphicsScene::resetHighlightedCycles() const
{
    auto& entities = allEntities();
    for (auto&& entity : entities) {
        auto edges = entity->edgesCollection();
        for (auto&& edge : edges) {
            edge->toggleRelationFlags(EdgeCollection::RelationFlags::RelationIsHighlighted, false);
        }
    }
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

    fromEntity.getTopLevelParent()->calculateEdgeVisibility();
    fromEntity.recursiveEdgeRelayout();
}

} // end namespace Codethink::lvtqtc
