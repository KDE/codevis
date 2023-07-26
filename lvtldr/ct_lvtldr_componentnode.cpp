// ct_lvtldr_componentnode.cpp                                       -*-C++-*-

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

#include <ct_lvtldr_componentnode.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtshr_stringhelpers.h>
#include <regex>

namespace {
using namespace Codethink;

std::string fixQName(const std::string& qname)
{
    return std::regex_replace(qname, std::regex("\\\\"), "/");
}

} // namespace

// ==========================
// class ComponentNode
// ==========================

namespace Codethink::lvtldr {

using namespace lvtshr;

ComponentNode::ComponentNode(NodeStorage& store): LakosianNode(store, std::nullopt)
{
    // Only to be used on tests
    // TODO: Let d_dbo be a provider interface instead of a dbo pointer directly
}

ComponentNode::ComponentNode(NodeStorage& store,
                             std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler,
                             std::optional<ComponentNodeFields> fields):
    LakosianNode(store, dbHandler), d_dbHandler(dbHandler), d_fields(*fields)
{
    setName(d_fields.name);
    d_qualifiedNameParts = NamingUtils::buildQualifiedNamePrefixParts(fixQName(d_fields.qualifiedName), "/");
}

ComponentNode::~ComponentNode() noexcept = default;

void ComponentNode::setParentPackageId(Codethink::lvtshr::UniqueId::RecordNumberType id)
{
    d_fields.packageId = id;
    d_dbHandler->get().updateFields(d_fields);
}

void ComponentNode::addConcreteDependency(ComponentNode *other)
{
    d_fields.providerIds.emplace_back(other->id());
    other->d_fields.clientIds.emplace_back(d_fields.id);
    d_dbHandler->get().addComponentDependency(d_fields.id, other->id());
}

void ComponentNode::removeConcreteDependency(ComponentNode *other)
{
    {
        auto& v = d_fields.providerIds;
        v.erase(std::remove(v.begin(), v.end(), other->id()), v.end());
    }
    {
        auto& v = other->d_fields.clientIds;
        v.erase(std::remove(v.begin(), v.end(), other->id()), v.end());
    }
    d_dbHandler->get().removeComponentDependency(d_fields.id, other->id());
}

lvtshr::DiagramType ComponentNode::type() const
{
    return lvtshr::DiagramType::ComponentType;
}

std::string ComponentNode::qualifiedName() const
{
    return NamingUtils::buildQualifiedName(d_qualifiedNameParts, name(), "/");
}

std::string ComponentNode::parentName()
{
    auto *parentPkg = dynamic_cast<PackageNode *>(parent());
    if (!parentPkg) {
        return "";
    }
    return parentPkg->name();
}

long long ComponentNode::id() const
{
    return d_fields.id;
}

lvtshr::UniqueId ComponentNode::uid() const
{
    return {lvtshr::DiagramType::ComponentType, id()};
}

LakosianNode::IsLakosianResult ComponentNode::isLakosian()
{
    auto *parentPkg = dynamic_cast<PackageNode *>(parent());
    if (!parentPkg) {
        return IsLakosianResult::ComponentHasNoPackage;
    }
    if (!lvtshr::StrUtil::beginsWith(name(), parentPkg->canonicalName() + "_")) {
        return IsLakosianResult::ComponentDoesntStartWithParentName;
    }
    return IsLakosianResult::IsLakosian;
}

void ComponentNode::loadParent()
{
    if (d->parentLoaded) {
        return;
    }
    d->parentLoaded = true;

    if (!d_fields.packageId) {
        d->parent = nullptr;
        return;
    }

    d->parent = d->store.findById({DiagramType::PackageType, *d_fields.packageId});
}

void ComponentNode::loadChildren()
{
    if (d->childrenLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getComponentFieldsById(d_fields.id);
    d->childrenLoaded = true;

    d->children.clear();
    d->children.reserve(d_fields.childUdtIds.size());
    for (auto&& id : d_fields.childUdtIds) {
        LakosianNode *node = d->store.findById({DiagramType::ClassType, id});
        if (dynamic_cast<TypeNode *>(node)->hasClassNamespace()) {
            continue;
        }
        d->children.push_back(node);
    }
}

void ComponentNode::loadProviders()
{
    if (d->providersLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getComponentFieldsById(d_fields.id);
    d->providersLoaded = true;

    for (auto&& id : d_fields.providerIds) {
        LakosianNode *node = d->store.findById({DiagramType::ComponentType, id});
        d->providers.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
    }
}

void ComponentNode::loadClients()
{
    if (d->clientsLoaded) {
        return;
    }
    d->clientsLoaded = true;

    for (auto&& id : d_fields.clientIds) {
        LakosianNode *node = d->store.findById({DiagramType::ComponentType, id});
        d->clients.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
    }
}

cpp::result<void, AddChildError> ComponentNode::addChild(LakosianNode *child)
{
    // don't add things twice.
    if (std::find(std::begin(d->children), std::end(d->children), child) != std::end(d->children)) {
        return cpp::fail(AddChildError{"The entity is already a child of this node"});
    }
    Q_EMIT onChildCountChanged(d->children.size());
    d->children.push_back(child);
    return {};
}

void ComponentNode::removeChild(LakosianNode *child)
{
    Q_EMIT onChildCountChanged(d->children.size());
    {
        auto& v = d_fields.childUdtIds;
        v.erase(std::remove(v.begin(), v.end(), child->id()), v.end());
    }
    {
        auto& v = d->children;
        v.erase(std::remove(v.begin(), v.end(), child), v.end());
    }
}

} // namespace Codethink::lvtldr
