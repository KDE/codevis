// ct_lvtldr_nodestorage.h                                         -*-C++-*-

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

#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtldr_componentnode.h>
#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtldr_repositorynode.h>
#include <ct_lvtldr_sociutils.h>
#include <ct_lvtldr_typenode.h>

#include <ct_lvtshr_stringhelpers.h>

#include <iostream>
#include <optional>
#include <unordered_map>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtshr;
using RecordNumberType = Codethink::lvtshr::UniqueId::RecordNumberType;

struct NodeStorage::Private {
    std::unique_ptr<DatabaseHandler> dbHandler = nullptr;

    std::unordered_map<lvtshr::UniqueId, std::unique_ptr<LakosianNode>, lvtshr::UniqueId::Hash> nodes;

// NOLINTBEGIN
#define BUILD_CALLBACK_PRIVATE(callback_type, name) boost::signals2::signal<callback_type> name

    BUILD_CALLBACK_PRIVATE(StorageClearedCallback, storageClearedSignal);
    BUILD_CALLBACK_PRIVATE(StorageChangedCallback, storageChangedSignal);
    BUILD_CALLBACK_PRIVATE(NodeAddedCallback, nodeAddedSignal);
    BUILD_CALLBACK_PRIVATE(NodeRemovedCallback, nodeRemovedSignal);
    BUILD_CALLBACK_PRIVATE(NodeNameChangedCallback, nodeNameChangedSignal);
    BUILD_CALLBACK_PRIVATE(PhysicalDependencyAddedCallback, physicalDependencyAddedSignal);
    BUILD_CALLBACK_PRIVATE(PhysicalDependencyRemovedCallback, physicalDependencyRemovedSignal);
    BUILD_CALLBACK_PRIVATE(LogicalRelationAddedCallback, logicalRelationAddedSignal);
    BUILD_CALLBACK_PRIVATE(LogicalRelationRemovedCallback, logicalRelationRemovedSignal);
    BUILD_CALLBACK_PRIVATE(EntityReparentCallback, entityReparentSignal);

#undef BUILD_CALLBACK_PRIVATE
    // NOLINTEND

    std::unordered_map<void *, std::vector<boost::signals2::connection>> connections;
};

NodeStorage::NodeStorage(): d(std::make_unique<Private>())
{
}

NodeStorage::NodeStorage(NodeStorage&& other) noexcept: d(std::move(other.d))
{
}

NodeStorage::~NodeStorage() noexcept = default;

void NodeStorage::setDatabaseSourcePath(std::string const& path)
{
    d->dbHandler = std::make_unique<SociDatabaseHandler>(path);
    clear();
    preloadHighLevelComponents();
}

void NodeStorage::closeDatabase()
{
    if (d->dbHandler) {
        d->dbHandler->close();
        clear();
    }
}

// NOLINTBEGIN
#define BUILD_CALLBACK_IMPL(callback_type, name)                                                                       \
    void NodeStorage::register##callback_type(void *receiver, const std::function<callback_type>& callback)            \
    {                                                                                                                  \
        auto connection = d->name.connect(callback);                                                                   \
        d->connections[receiver].push_back(connection);                                                                \
    }

BUILD_CALLBACK_IMPL(StorageClearedCallback, storageClearedSignal)
BUILD_CALLBACK_IMPL(StorageChangedCallback, storageChangedSignal)
BUILD_CALLBACK_IMPL(NodeAddedCallback, nodeAddedSignal)
BUILD_CALLBACK_IMPL(NodeRemovedCallback, nodeRemovedSignal)
BUILD_CALLBACK_IMPL(NodeNameChangedCallback, nodeNameChangedSignal)
BUILD_CALLBACK_IMPL(PhysicalDependencyAddedCallback, physicalDependencyAddedSignal)
BUILD_CALLBACK_IMPL(PhysicalDependencyRemovedCallback, physicalDependencyRemovedSignal)
BUILD_CALLBACK_IMPL(LogicalRelationAddedCallback, logicalRelationAddedSignal)
BUILD_CALLBACK_IMPL(LogicalRelationRemovedCallback, logicalRelationRemovedSignal)
BUILD_CALLBACK_IMPL(EntityReparentCallback, entityReparentSignal)

#undef BUILD_CALLBACK_IMPL
// NOLINTEND

void NodeStorage::unregisterAllCallbacksTo(void *receiver)
{
    if (d->connections.count(receiver) == 0) {
        return;
    }

    for (const auto& c : d->connections.at(receiver)) {
        c.disconnect();
    }
    auto it = std::find_if(d->connections.begin(), d->connections.end(), [&receiver](const auto& e) {
        return e.first == receiver;
    });
    d->connections.erase(it);
}

void NodeStorage::preloadHighLevelComponents()
{
    std::function<void(LakosianNode *)> loadChildrenRecursively = [&](LakosianNode *node) -> void {
        if (node->type() == DiagramType::ComponentType) {
            // Do not load component children (Will be loaded on demand)
            return;
        }

        (void) node->children();
        for (auto *c : node->children()) {
            loadChildrenRecursively(c);
        }
    };

    for (auto *pkg : getTopLevelPackages()) {
        loadChildrenRecursively(pkg);
    }
}

// TODO: See #435 - This modifies the current database, instead of working on a copy of it.
cpp::result<LakosianNode *, ErrorAddPackage> NodeStorage::addPackage(const std::string& name,
                                                                     const std::string& qualifiedName,
                                                                     Codethink::lvtldr::LakosianNode *parent,
                                                                     std::any userdata)
{
    using Kind = ErrorAddPackage::Kind;

    if (findByQualifiedName(lvtshr::DiagramType::PackageType, qualifiedName)) {
        return cpp::fail(ErrorAddPackage{Kind::QualifiedNameAlreadyRegistered, qualifiedName});
    }

    if (parent && !parent->children().empty()) {
        if (parent->children()[0]->type() == DiagramType::ComponentType) {
            return cpp::fail(ErrorAddPackage{Kind::CannotAddPackageToStandalonePackage, {}});
        }
    }

    // Create the package on database
    // A Created package does not have a directory on disk, so the path is empty.
    auto dao = PackageNodeFields{};
    dao.name = name;
    dao.qualifiedName = qualifiedName;
    dao.diskPath = "";
    if (parent) {
        dao.parentId = parent->id();
    }
    d->dbHandler->addFields(dao);

    // Update internal nodes cache
    auto uid = lvtshr::UniqueId{lvtshr::DiagramType::PackageType, dao.id};
    auto [it, _] = d->nodes.emplace(uid, std::make_unique<PackageNode>(*this, *d->dbHandler, dao));
    auto *lakosianNode = it->second.get();

    if (parent) {
        // This is a new element, so addChild should work on the parent.
        // there are no possibilities of the new entity already having the parent as child of
        // this node. so failures here are not expected. don't even return.

        auto result = parent->addChild(lakosianNode);
        if (result.has_error()) {
            return cpp::fail(ErrorAddPackage{Kind::CantAddChildren, result.error().what});
        }
    }

    d->nodeAddedSignal(lakosianNode, std::move(userdata));
    lakosianNode->registerOnNameChanged(this, [this](LakosianNode *node) {
        updateAndNotifyNodeRenameV2<lvtldr::PackageNode>(node);
    });
    d->storageChangedSignal();
    return lakosianNode;
}

cpp::result<void, ErrorRemovePackage> NodeStorage::removePackage(LakosianNode *node)
{
    using Kind = ErrorRemovePackage::Kind;

    if (!node->providers().empty()) {
        return cpp::fail(ErrorRemovePackage{Kind::CannotRemovePackageWithProviders});
    }
    if (!node->clients().empty()) {
        return cpp::fail(ErrorRemovePackage{Kind::CannotRemovePackageWithClients});
    }
    if (!node->children().empty()) {
        return cpp::fail(ErrorRemovePackage{Kind::CannotRemovePackageWithChildren});
    }

    if (node->parent()) {
        dynamic_cast<PackageNode *>(node->parent())->removeChildPackage(dynamic_cast<PackageNode *>(node));
        node->parent()->invalidateChildren();
    }

    // we know that this package has nothing, so it's safe to delete.
    // we may need to provide a better algorithm in the future.
    d->dbHandler->removePackageFieldsById(node->id());

    d->nodeRemovedSignal(node);
    d->nodes.erase(node->uid());
    d->storageChangedSignal();
    return {};
}

cpp::result<LakosianNode *, ErrorAddComponent>
NodeStorage::addComponent(const std::string& name, const std::string& qualifiedName, LakosianNode *parentPackage)
{
    using Kind = ErrorAddComponent::Kind;

    if (parentPackage == nullptr) {
        return cpp::fail(ErrorAddComponent{Kind::MissingParent});
    }
    if (findByQualifiedName(lvtshr::DiagramType::ComponentType, qualifiedName)) {
        return cpp::fail(ErrorAddComponent{Kind::QualifiedNameAlreadyRegistered});
    }
    if (parentPackage->isPackageGroup()) {
        return cpp::fail(ErrorAddComponent{Kind::CannotAddComponentToPkgGroup});
    }

    // Create the component on database
    auto dao = ComponentNodeFields{};
    dao.name = name;
    dao.qualifiedName = qualifiedName;
    dao.packageId = parentPackage->id();
    d->dbHandler->addFields(dao);

    auto uid = lvtshr::UniqueId{lvtshr::DiagramType::ComponentType, dao.id};
    auto [it, _] = d->nodes.emplace(uid, std::make_unique<ComponentNode>(*this, *d->dbHandler, dao));
    auto *lakosianNode = it->second.get();

    auto result = parentPackage->addChild(lakosianNode);
    if (result.has_error()) {
        std::cerr << "Unexpected failure adding a component to the NodeStorage." << std::endl;
        std::cerr << "This means the database in memory is in an inconsistent state" << std::endl;
        std::cerr << "Error: " << result.error().what;
        std::exit(EXIT_FAILURE);
    }

    d->nodeAddedSignal(lakosianNode, std::any());
    lakosianNode->registerOnNameChanged(this, [this](LakosianNode *node) {
        updateAndNotifyNodeRenameV2<lvtldr::ComponentNode>(node);
    });
    d->storageChangedSignal();
    return lakosianNode;
}

cpp::result<void, ErrorRemoveComponent> NodeStorage::removeComponent(LakosianNode *node)
{
    using Kind = ErrorRemoveComponent::Kind;

    if (!node->providers().empty()) {
        return cpp::fail(ErrorRemoveComponent{Kind::CannotRemoveComponentWithProviders});
    }

    if (!node->clients().empty()) {
        return cpp::fail(ErrorRemoveComponent{Kind::CannotRemoveComponentWithClients});
    }
    if (!node->children().empty()) {
        return cpp::fail(ErrorRemoveComponent{Kind::CannotRemoveComponentWithChildren});
    }

    // Must be done before actual removal
    d->nodeRemovedSignal(node);

    // Update cache
    d->nodes.at(node->uid());
    auto e = d->nodes.extract(node->uid());
    assert(!e.empty());
    auto *removedNode = e.mapped().get();
    assert(removedNode == node); // Sanity check
    removedNode->parent()->invalidateChildren();

    // Update database
    d->dbHandler->removeComponentFieldsById(removedNode->id());

    d->storageChangedSignal();
    return {};
}

cpp::result<LakosianNode *, ErrorAddUDT> NodeStorage::addLogicalEntity(const std::string& name,
                                                                       const std::string& qualifiedName,
                                                                       LakosianNode *parent,
                                                                       lvtshr::UDTKind kind)
{
    using ErrorKind = ErrorAddUDT::Kind;

    auto isValidParentType = [&parent]() {
        if (!parent) {
            return false;
        }

        if (parent->type() == DiagramType::ComponentType) {
            return true;
        }

        if (parent->type() == DiagramType::ClassType) {
            auto *parentAsClass = dynamic_cast<TypeNode *>(parent);
            if (parentAsClass->kind() == lvtshr::UDTKind::Class || parentAsClass->kind() == lvtshr::UDTKind::Struct) {
                return true;
            }
        }

        return false;
    }();

    if (!isValidParentType) {
        return cpp::fail(ErrorAddUDT{ErrorKind::BadParentType});
    }

    // Update backend database
    auto dao = TypeNodeFields{};
    dao.name = name;
    dao.qualifiedName = qualifiedName;
    dao.kind = kind;

    if (parent->type() == DiagramType::ComponentType) {
        dao.componentIds.emplace_back(parent->id());
    } else if (parent->type() == DiagramType::ClassType) {
        dao.classNamespaceId = parent->id();
        dao.componentIds.emplace_back(parent->parent()->id());
    }
    d->dbHandler->addFields(dao);

    // Update cache
    auto id = lvtshr::UniqueId{lvtshr::DiagramType::ClassType, dao.id};
    auto *addedNode = fetchFromDBByIdV2<lvtldr::TypeNode>(id);
    assert(addedNode);
    parent->addChild(addedNode).expect("Unexpected failure adding a logical entity to the NodeStorage.");

    d->nodeAddedSignal(addedNode, std::any());
    d->storageChangedSignal();
    return addedNode;
}

cpp::result<void, ErrorRemoveLogicalEntity> NodeStorage::removeLogicalEntity(LakosianNode *node)
{
    using Kind = ErrorRemoveLogicalEntity::Kind;

    if (!node->providers().empty()) {
        return cpp::fail(ErrorRemoveLogicalEntity{Kind::CannotRemoveUDTWithProviders});
    }
    if (!node->clients().empty()) {
        return cpp::fail(ErrorRemoveLogicalEntity{Kind::CannotRemoveUDTWithClients});
    }
    if (!node->children().empty()) {
        return cpp::fail(ErrorRemoveLogicalEntity{Kind::CannotRemoveUDTWithChildren});
    }

    // Update cache
    d->nodes.at(node->uid());
    auto e = d->nodes.extract(node->uid());
    assert(!e.empty());
    auto *removedNode = e.mapped().get();
    assert(removedNode == node); // Sanity check
    auto *parent = removedNode->parent();
    if (parent && dynamic_cast<ComponentNode *>(parent) != nullptr) {
        dynamic_cast<ComponentNode *>(parent)->removeChild(node);
    }

    // Update database
    d->dbHandler->removeUdtFieldsById(removedNode->id());

    // Notify
    d->nodeRemovedSignal(removedNode);
    d->storageChangedSignal();
    return {};
}

cpp::result<void, ErrorAddPhysicalDependency>
NodeStorage::addPhysicalDependency(LakosianNode *source, LakosianNode *target, PhysicalDependencyType type)
{
    using Kind = ErrorAddPhysicalDependency::Kind;

    auto isValidType = [](auto const& e) {
        return e->type() == DiagramType::PackageType || e->type() == DiagramType::ComponentType;
    };
    if (!isValidType(source) || !isValidType(target)) {
        return cpp::fail(ErrorAddPhysicalDependency{Kind::InvalidType});
    }
    if (source == target) {
        return cpp::fail(ErrorAddPhysicalDependency{Kind::SelfRelation});
    }
    if (source->type() == DiagramType::ComponentType || target->type() == DiagramType::ComponentType) {
        if (type == PhysicalDependencyType::AllowedDependency) {
            // There's no such thing as "allowed dependency" between components.
            return cpp::fail(ErrorAddPhysicalDependency{Kind::InvalidType});
        }
    } else {
        auto *sourcePackage = dynamic_cast<PackageNode *>(source);
        auto *targetPackage = dynamic_cast<PackageNode *>(target);
        if (type == PhysicalDependencyType::AllowedDependency) {
            if (sourcePackage->hasAllowedDependency(targetPackage)) {
                return cpp::fail(ErrorAddPhysicalDependency{Kind::DependencyAlreadyExists});
            }
        }

        if (type == PhysicalDependencyType::ConcreteDependency) {
            if (sourcePackage->hasConcreteDependency(targetPackage)) {
                return cpp::fail(ErrorAddPhysicalDependency{Kind::DependencyAlreadyExists});
            }
        }
    }

    auto isPkgGrp = [](auto *e) {
        return e && e->type() == DiagramType::PackageType && e->parent() == nullptr;
    };
    auto isStandalonePackage = [](auto *e) {
        return e && e->type() == DiagramType::PackageType
            && (e->children().empty() || e->children()[0]->type() == DiagramType::ComponentType);
    };
    auto isPkg = [](auto *e) {
        return e && e->type() == DiagramType::PackageType && e->parent() != nullptr;
    };
    auto isComponent = [](auto *e) {
        return e && e->type() == DiagramType::ComponentType;
    };

    if (isPkgGrp(source)) {
        auto *targetPackage = dynamic_cast<PackageNode *>(target);
        if (!(isPkgGrp(target) || (targetPackage && targetPackage->isStandalone()))) {
            // A package group cannot depend on an entity from a different level
            return cpp::fail(ErrorAddPhysicalDependency{Kind::HierarchyLevelMismatch});
        }

        switch (type) {
        case PhysicalDependencyType::ConcreteDependency: {
            dynamic_cast<PackageNode *>(source)->addConcreteDependency(dynamic_cast<PackageNode *>(target));
            break;
        }
        case PhysicalDependencyType::AllowedDependency: {
            dynamic_cast<PackageNode *>(source)->addAllowedDependency(dynamic_cast<PackageNode *>(target));
            break;
        }
        }
    } else if (isPkg(source)) {
        if (!isPkg(target)) {
            // A package cannot depend on an entity from a different level
            return cpp::fail(ErrorAddPhysicalDependency{Kind::HierarchyLevelMismatch});
        }
        if (source->parent() != target->parent() && !source->parent()->hasProvider(target->parent())) {
            // A package can only depend on other if there's a dependency between their parents
            return cpp::fail(ErrorAddPhysicalDependency{Kind::MissingParentDependency});
        }

        switch (type) {
        case PhysicalDependencyType::ConcreteDependency: {
            dynamic_cast<PackageNode *>(source)->addConcreteDependency(dynamic_cast<PackageNode *>(target));
            break;
        }
        case PhysicalDependencyType::AllowedDependency: {
            dynamic_cast<PackageNode *>(source)->addAllowedDependency(dynamic_cast<PackageNode *>(target));
            break;
        }
        }
    } else if (isComponent(source)) {
        if (!isComponent(target)) {
            // Components can only depend on other components
            return cpp::fail(ErrorAddPhysicalDependency{Kind::HierarchyLevelMismatch});
        }

        if (source->parent() != target->parent()) {
            // Rules for components with different parents

            auto isAllowedDueToPkgGroupToStandalonePkgDependency = [&]() {
                // If this component is within a package group and the target component is within a standalone group ...
                if (isPkgGrp(source->parent()->parent()) && isStandalonePackage(target->parent())) {
                    auto *pkggrp = source->parent()->parent();
                    auto *standalonegrp = target->parent();
                    // ... and there's a dependency between the source's group and target's standalone package ...
                    if (pkggrp->hasProvider(standalonegrp)) {
                        // ... then the component's dependency is allowed.
                        return true;
                    }
                }
                return false;
            }();
            auto isAllowedDueToFwdDependency = source->parent()->hasProvider(target->parent());

            if (!isAllowedDueToPkgGroupToStandalonePkgDependency && !isAllowedDueToFwdDependency) {
                // A component can only depend on other if there's a dependency between their parents
                return cpp::fail(ErrorAddPhysicalDependency{Kind::MissingParentDependency});
            }
        }

        dynamic_cast<ComponentNode *>(source)->addConcreteDependency(dynamic_cast<ComponentNode *>(target));
    }

    // TODO [#455]: It is possible to update the data structures instead of invalidate them.
    source->invalidateProviders();
    target->invalidateClients();

    d->physicalDependencyAddedSignal(source, target, type);
    d->storageChangedSignal();
    return {};
}

cpp::result<void, ErrorRemovePhysicalDependency>
NodeStorage::removePhysicalDependency(LakosianNode *source, LakosianNode *target, PhysicalDependencyType type)
{
    using Kind = ErrorRemovePhysicalDependency::Kind;

    if (!source->hasProvider(target)) {
        return cpp::fail(ErrorRemovePhysicalDependency{Kind::InexistentRelation});
    }

    // Package Group
    auto isPkgGrp = [](auto const& e) {
        return e->type() == DiagramType::PackageType && e->parent() == nullptr;
    };
    if (isPkgGrp(source)) {
        if (type == PhysicalDependencyType::ConcreteDependency) {
            dynamic_cast<PackageNode *>(source)->removeConcreteDependency(dynamic_cast<PackageNode *>(target));
        } else if (type == PhysicalDependencyType::AllowedDependency) {
            dynamic_cast<PackageNode *>(source)->removeAllowedDependency(dynamic_cast<PackageNode *>(target));
        }
    }

    // Package
    auto isPkg = [](auto const& e) {
        return e->type() == DiagramType::PackageType && e->parent() != nullptr;
    };
    if (isPkg(source)) {
        if (type == PhysicalDependencyType::ConcreteDependency) {
            dynamic_cast<PackageNode *>(source)->removeConcreteDependency(dynamic_cast<PackageNode *>(target));
        } else if (type == PhysicalDependencyType::AllowedDependency) {
            dynamic_cast<PackageNode *>(source)->removeAllowedDependency(dynamic_cast<PackageNode *>(target));
        }
    }

    // Component
    auto isComponent = [](auto const& e) {
        return e->type() == DiagramType::ComponentType;
    };

    if (isComponent(source) && isComponent(target)) {
        dynamic_cast<ComponentNode *>(source)->removeConcreteDependency(dynamic_cast<ComponentNode *>(target));
    }

    // TODO [#455]: It is possible to update the data structures instead of invalidate them.
    source->invalidateProviders();
    target->invalidateClients();

    d->physicalDependencyRemovedSignal(source, target);
    d->storageChangedSignal();
    return {};
}

cpp::result<void, ErrorAddLogicalRelation>
NodeStorage::addLogicalRelation(TypeNode *source, TypeNode *target, LakosRelationType type)
{
    using Kind = ErrorAddLogicalRelation::Kind;

    if (source == nullptr || target == nullptr) {
        return cpp::fail(ErrorAddLogicalRelation{Kind::InvalidRelation});
    }
    if (source == target) {
        return cpp::fail(ErrorAddLogicalRelation{Kind::SelfRelation});
    }
    if (type != LakosRelationType::IsA && type != LakosRelationType::UsesInTheImplementation
        && type != LakosRelationType::UsesInTheInterface) {
        return cpp::fail(ErrorAddLogicalRelation{Kind::InvalidLakosRelationType});
    }
    if (source->hasProvider(target)) {
        return cpp::fail(ErrorAddLogicalRelation{Kind::AlreadyHaveDependency});
    }
    if (source->parent() != target->parent() && !source->parent()->hasProvider(target->parent())) {
        // different semantics here. if we are on the class level, the next parent relation can be an
        // isA, usesInImplementation, usesInInterface.
        // but if it's a component, there's only one type of relation possible, a Dependency.
        // so we need two errors to sinalize that.
        using FieldType = Codethink::lvtshr::DiagramType;
        if (source->parent()->type() == FieldType::ComponentType
            && target->parent()->type() == FieldType::ComponentType) {
            return cpp::fail(ErrorAddLogicalRelation{Kind::ComponentDependencyRequired});
        }
        return cpp::fail(ErrorAddLogicalRelation{Kind::ParentDependencyRequired});
    }

    // Update database
    if (type == LakosRelationType::IsA) {
        auto& parent = target;
        auto& child = source;
        d->dbHandler->addClassHierarchy(parent->id(), child->id());
    }
    if (type == LakosRelationType::UsesInTheImplementation) {
        d->dbHandler->addImplementationRelationship(source->id(), target->id());
    }
    if (type == LakosRelationType::UsesInTheInterface) {
        d->dbHandler->addInterfaceRelationship(source->id(), target->id());
    }

    // TODO [#455]: It is possible to update the data structures instead of invalidate them.
    source->invalidateProviders();
    target->invalidateClients();

    d->logicalRelationAddedSignal(source, target, type);
    d->storageChangedSignal();
    return {};
}

cpp::result<void, ErrorRemoveLogicalRelation>
NodeStorage::removeLogicalRelation(TypeNode *source, TypeNode *target, LakosRelationType type)
{
    using Kind = ErrorRemoveLogicalRelation::Kind;

    auto r = [&]() -> cpp::result<void, ErrorRemoveLogicalRelation> {
        if (type == LakosRelationType::IsA) {
            auto& parent = target;
            auto& child = source;
            if (!dynamic_cast<TypeNode *>(child)->isA(dynamic_cast<TypeNode *>(parent))) {
                return cpp::fail(ErrorRemoveLogicalRelation{Kind::InexistentRelation});
            }
            d->dbHandler->removeClassHierarchy(parent->id(), child->id());
            return {};
        }
        if (type == LakosRelationType::UsesInTheImplementation) {
            if (!dynamic_cast<TypeNode *>(source)->usesInTheImplementation(dynamic_cast<TypeNode *>(target))) {
                return cpp::fail(ErrorRemoveLogicalRelation{Kind::InexistentRelation});
            }
            d->dbHandler->removeImplementationRelationship(source->id(), target->id());
            return {};
        }
        if (type == LakosRelationType::UsesInTheInterface) {
            if (!dynamic_cast<TypeNode *>(source)->usesInTheInterface(dynamic_cast<TypeNode *>(target))) {
                return cpp::fail(ErrorRemoveLogicalRelation{Kind::InexistentRelation});
            }
            d->dbHandler->removeInterfaceRelationship(source->id(), target->id());
            return {};
        }
        return cpp::fail(ErrorRemoveLogicalRelation{Kind::InvalidLakosRelationType});
    }();
    if (r.has_error()) {
        return r;
    }

    // TODO [#455]: It is possible to update the data structures instead of invalidate them.
    source->invalidateProviders();
    target->invalidateClients();

    d->logicalRelationRemovedSignal(source, target, type);
    d->storageChangedSignal();
    return {};
}

cpp::result<void, ErrorReparentEntity> NodeStorage::reparentEntity(LakosianNode *entity, LakosianNode *newParent)
{
    using Kind = ErrorReparentEntity::Kind;

    if (entity == nullptr || entity->type() != DiagramType::ComponentType) {
        return cpp::fail(ErrorReparentEntity{Kind::InvalidEntity});
    }
    if (newParent == nullptr || newParent->type() != DiagramType::PackageType) {
        return cpp::fail(ErrorReparentEntity{Kind::InvalidParent});
    }

    auto *oldParent = dynamic_cast<PackageNode *>(entity->parent());
    for (auto *udt : entity->children()) {
        dynamic_cast<TypeNode *>(udt)->setParentPackageId(newParent->id());
    }
    dynamic_cast<ComponentNode *>(entity)->setParentPackageId(newParent->id());
    oldParent->removeChild(entity);
    (void) newParent->addChild(dynamic_cast<ComponentNode *>(entity));

    // Update (invalidate) caches
    oldParent->invalidateChildren();
    newParent->invalidateChildren();
    entity->invalidateParent();

    d->entityReparentSignal(entity, oldParent, newParent);
    d->storageChangedSignal();
    return {};
}

LakosianNode *NodeStorage::findById(const lvtshr::UniqueId& id)
{
    // Search for the node on cache
    if (d->nodes.find(id) != d->nodes.end()) {
        return d->nodes.at(id).get();
    }

    switch (id.diagramType()) {
    case lvtshr::DiagramType::ClassType: {
        return fetchFromDBByIdV2<lvtldr::TypeNode>(id);
    }
    case lvtshr::DiagramType::ComponentType: {
        return fetchFromDBByIdV2<lvtldr::ComponentNode>(id);
    }
    case lvtshr::DiagramType::PackageType: {
        return fetchFromDBByIdV2<lvtldr::PackageNode>(id);
    }
    case lvtshr::DiagramType::RepositoryType: {
        return fetchFromDBByIdV2<lvtldr::RepositoryNode>(id);
    }
    case DiagramType::NoneType:
        break;
    }
    return nullptr;
}

LakosianNode *NodeStorage::findByQualifiedName(lvtshr::DiagramType type, const std::string& qualifiedName)
{
    // Search for the node on cache
    auto it = std::find_if(d->nodes.begin(), d->nodes.end(), [&](auto const& it) {
        auto const& node = it.second;
        return node->type() == type && node->qualifiedName() == qualifiedName;
    });
    if (it != d->nodes.end()) {
        return it->second.get();
    }

    // Search for the node in the backend
    switch (type) {
    case lvtshr::DiagramType::ClassType: {
        return fetchFromDBByQualifiedNameV2<lvtldr::TypeNode>(qualifiedName);
    }
    case lvtshr::DiagramType::ComponentType: {
        return fetchFromDBByQualifiedNameV2<lvtldr::ComponentNode>(qualifiedName);
    }
    case lvtshr::DiagramType::PackageType: {
        return fetchFromDBByQualifiedNameV2<lvtldr::PackageNode>(qualifiedName);
    }
    case lvtshr::DiagramType::RepositoryType: {
        return fetchFromDBByQualifiedNameV2<lvtldr::RepositoryNode>(qualifiedName);
    }
    case lvtshr::DiagramType::NoneType: {
        break;
    }
    }

    return nullptr;
}

LakosianNode *NodeStorage::findByQualifiedName(const std::string& qualifiedName)
{
    using lvtshr::DiagramType;
    for (auto const& type :
         {DiagramType::RepositoryType, DiagramType::PackageType, DiagramType::ComponentType, DiagramType::ClassType}) {
        auto *lakosianNode = findByQualifiedName(type, qualifiedName);
        if (lakosianNode) {
            return lakosianNode;
        }
    }
    return nullptr;
}

std::vector<LakosianNode *> NodeStorage::getTopLevelPackages()
{
    auto topLvlEntityIds = d->dbHandler->getTopLevelEntityIds();
    auto out = std::vector<LakosianNode *>{};
    for (auto&& uid : topLvlEntityIds) {
        auto *maybePackage = findById(uid);
        if (maybePackage) {
            out.emplace_back(maybePackage);
        }
    }
    return out;
}

void NodeStorage::clear()
{
    d->storageClearedSignal();
    d->nodes.clear();
}

template<typename LDR_TYPE>
auto getFieldsByQualifiedName(DatabaseHandler& dbHandler, const std::string& qualifiedName)
{
}
template<>
auto getFieldsByQualifiedName<ComponentNode>(DatabaseHandler& dbHandler, const std::string& qualifiedName)
{
    return dbHandler.getComponentFieldsByQualifiedName(qualifiedName);
}
template<>
auto getFieldsByQualifiedName<PackageNode>(DatabaseHandler& dbHandler, const std::string& qualifiedName)
{
    return dbHandler.getPackageFieldsByQualifiedName(qualifiedName);
}
template<>
auto getFieldsByQualifiedName<TypeNode>(DatabaseHandler& dbHandler, const std::string& qualifiedName)
{
    return dbHandler.getUdtFieldsByQualifiedName(qualifiedName);
}
template<>
auto getFieldsByQualifiedName<RepositoryNode>(DatabaseHandler& dbHandler, const std::string& qualifiedName)
{
    return dbHandler.getRepositoryFieldsByQualifiedName(qualifiedName);
}

template<typename LDR_TYPE>
LakosianNode *NodeStorage::fetchFromDBByQualifiedNameV2(const std::string& qualifiedName)
{
    auto dao = getFieldsByQualifiedName<LDR_TYPE>(*d->dbHandler, qualifiedName);
    if (dao.id == -1) {
        return nullptr;
    }

    auto uid = lvtshr::UniqueId{LakosianNodeType<LDR_TYPE>::diagramType, dao.id};
    auto [it, _] = d->nodes.emplace(uid, std::make_unique<LDR_TYPE>(*this, *d->dbHandler, dao));
    auto *lakosianNode = it->second.get();
    lakosianNode->registerOnNameChanged(this, [this](LakosianNode *node) {
        updateAndNotifyNodeRenameV2<LDR_TYPE>(node);
    });
    return lakosianNode;
}

template<typename LDR_TYPE>
auto getFieldsById(DatabaseHandler& dbHandler, RecordNumberType id)
{
}
template<>
auto getFieldsById<ComponentNode>(DatabaseHandler& dbHandler, RecordNumberType id)
{
    return dbHandler.getComponentFieldsById(id);
}
template<>
auto getFieldsById<PackageNode>(DatabaseHandler& dbHandler, RecordNumberType id)
{
    return dbHandler.getPackageFieldsById(id);
}
template<>
auto getFieldsById<TypeNode>(DatabaseHandler& dbHandler, RecordNumberType id)
{
    return dbHandler.getUdtFieldsById(id);
}
template<>
auto getFieldsById<RepositoryNode>(DatabaseHandler& dbHandler, RecordNumberType id)
{
    return dbHandler.getRepositoryFieldsById(id);
}

template<typename LDR_TYPE>
LakosianNode *NodeStorage::fetchFromDBByIdV2(const lvtshr::UniqueId& uid)
{
    auto dao = getFieldsById<LDR_TYPE>(*d->dbHandler, uid.recordNumber());
    if (dao.id == -1) {
        return nullptr;
    }

    auto [it, _] = d->nodes.emplace(uid, std::make_unique<LDR_TYPE>(*this, *d->dbHandler, dao));
    auto *lakosianNode = it->second.get();
    lakosianNode->registerOnNameChanged(this, [this](LakosianNode *node) {
        updateAndNotifyNodeRenameV2<LDR_TYPE>(node);
    });
    return lakosianNode;
}

template<typename LDR_TYPE>
void NodeStorage::updateAndNotifyNodeRenameV2(LakosianNode *node)
{
    d->dbHandler->updateFields(LDR_TYPE::from(node)->getFields());
    d->nodeNameChangedSignal(node);
    d->storageChangedSignal();
}
