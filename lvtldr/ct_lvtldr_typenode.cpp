// ct_lvtldr_typenode.cpp                                            -*-C++-*-

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
#include <ct_lvtldr_typenode.h>

namespace Codethink::lvtldr {

using namespace lvtshr;

// ==========================
// class TypeNode
// ==========================

TypeNode::TypeNode(NodeStorage& store): LakosianNode(store, std::nullopt)
{
    // Only to be used on tests
}

TypeNode::TypeNode(NodeStorage& store,
                   std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler,
                   std::optional<TypeNodeFields> fields):
    LakosianNode(store, dbHandler), d_dbHandler(dbHandler), d_fields(*fields)
{
    setName(d_fields.name);
    d_qualifiedNameParts = NamingUtils::buildQualifiedNamePrefixParts(d_fields.qualifiedName, "::");
}

TypeNode::~TypeNode() noexcept = default;

TypeNode::TypeNode(TypeNode&&) noexcept = default;

lvtshr::DiagramType TypeNode::type() const
{
    return lvtshr::DiagramType::ClassType;
}

lvtshr::UDTKind TypeNode::kind() const
{
    return d_fields.kind;
}

std::string TypeNode::qualifiedName() const
{
    return NamingUtils::buildQualifiedName(d_qualifiedNameParts, name(), "::");
}

std::string TypeNode::parentName()
{
    if (!d_fields.parentPackageId) {
        return "";
    }
    // TODO 695: Should this be Package or Component?
    auto *node = d->store.findById({DiagramType::PackageType, *d_fields.parentPackageId});
    return node->qualifiedName();
}

void TypeNode::setParentPackageId(Codethink::lvtshr::UniqueId::RecordNumberType id)
{
    d_fields.parentPackageId = id;
    d_dbHandler->get().updateFields(d_fields);
}

long long TypeNode::id() const
{
    return d_fields.id;
}

lvtshr::UniqueId TypeNode::uid() const
{
    return {lvtshr::DiagramType::ClassType, id()};
}

LakosianNode::IsLakosianResult TypeNode::isLakosian()
{
    return IsLakosianResult::IsLakosian;
}

void TypeNode::loadParent()
{
    if (d->parentLoaded) {
        return;
    }
    d->parentLoaded = true;

    if (d_fields.classNamespaceId) {
        d->parent = d->store.findById({DiagramType::ClassType, *d_fields.classNamespaceId});
        return;
    }

    // If not found, use component for parent
    // unfortunately UDT <-> SourceComponent is many to many
    // although in the common case there should be only one component
    if (d_fields.componentIds.empty()) {
        d->parent = nullptr;
        return;
    }
    // If found, use the first component in the list
    // TODO: if necessary, figure out a good heuristic for this
    auto firstComponent = d_fields.componentIds[0];
    d->parent = d->store.findById({DiagramType::ComponentType, firstComponent});
}

void TypeNode::loadChildren()
{
    if (d->childrenLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getUdtFieldsById(d_fields.id);
    d->childrenLoaded = true;

    d->children.clear();
    d->children.reserve(d_fields.nestedTypeIds.size());
    for (auto&& id : d_fields.nestedTypeIds) {
        LakosianNode *node = d->store.findById({DiagramType::ClassType, id});
        d->children.push_back(node);
    }
}

cpp::result<void, AddChildError> TypeNode::addChild(LakosianNode *child)
{
    // don't add things twice.
    if (std::find(std::begin(d->children), std::end(d->children), child) != std::end(d->children)) {
        std::string errorString = "The entity " + child->name() + " is already a child of" + name();
        return cpp::fail(AddChildError{errorString});
    }

    if (dynamic_cast<TypeNode *>(child) != nullptr) {
        d_fields.nestedTypeIds.emplace_back(child->id());
    }

    d->onChildCountChanged(d->children.size());
    d->children.push_back(child);
    return {};
}

bool TypeNode::isA(TypeNode *other)
{
    // TODO: Lazy check instead of getting info from database
    d_fields = (d_dbHandler->get()).getUdtFieldsById(d_fields.id);

    auto& v = d_fields.isAIds;
    return std::find(v.begin(), v.end(), other->id()) != v.end();
}

bool TypeNode::usesInTheImplementation(TypeNode *other)
{
    // TODO: Lazy check instead of getting info from database
    d_fields = (d_dbHandler->get()).getUdtFieldsById(d_fields.id);

    auto& v = d_fields.usesInImplementationIds;
    return std::find(v.begin(), v.end(), other->id()) != v.end();
}

bool TypeNode::usesInTheInterface(TypeNode *other)
{
    // TODO: Lazy check instead of getting info from database
    d_fields = (d_dbHandler->get()).getUdtFieldsById(d_fields.id);

    auto& v = d_fields.usesInInterfaceIds;
    return std::find(v.begin(), v.end(), other->id()) != v.end();
}

void TypeNode::loadProviders()
{
    if (d->providersLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getUdtFieldsById(d_fields.id);
    d->providersLoaded = true;

    d->providers.reserve(d_fields.isAIds.size() + d_fields.usesInInterfaceIds.size()
                         + d_fields.usesInImplementationIds.size());

    for (auto&& id : d_fields.isAIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->providers.emplace_back(lvtshr::LakosRelationType::IsA, node);
    }
    for (auto&& id : d_fields.usesInInterfaceIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->providers.emplace_back(lvtshr::LakosRelationType::UsesInTheInterface, node);
    }
    for (auto&& id : d_fields.usesInImplementationIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->providers.emplace_back(lvtshr::LakosRelationType::UsesInTheImplementation, node);
    }
}

void TypeNode::loadClients()
{
    if (d->clientsLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getUdtFieldsById(d_fields.id);
    d->clientsLoaded = true;

    d->clients.reserve(d_fields.isBaseOfIds.size() + d_fields.usedByInterfaceIds.size()
                       + d_fields.usedByImplementationIds.size());

    for (auto&& id : d_fields.isBaseOfIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->clients.emplace_back(lvtshr::LakosRelationType::IsA, node);
    }
    for (auto&& id : d_fields.usedByInterfaceIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->clients.emplace_back(lvtshr::LakosRelationType::UsesInTheInterface, node);
    }
    for (auto&& id : d_fields.usedByImplementationIds) {
        auto *node = d->store.findById({DiagramType::ClassType, id});

        // skip if one is a namepace of the other (or vice-versa)
        if (d_fields.classNamespaceId == node->id()
            || dynamic_cast<TypeNode *>(node)->classNamespaceId() == d_fields.id) {
            continue;
        }
        d->clients.emplace_back(lvtshr::LakosRelationType::UsesInTheImplementation, node);
    }
}

bool TypeNode::hasClassNamespace() const
{
    return d_fields.classNamespaceId.has_value();
}

lvtshr::UniqueId::RecordNumberType TypeNode::classNamespaceId() const
{
    if (d_fields.classNamespaceId) {
        return *d_fields.classNamespaceId;
    }
    return -1;
}

} // namespace Codethink::lvtldr
