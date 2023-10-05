// ct_lvtldr_packagenode.cpp                                         -*-C++-*-

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

#include <ct_lvtldr_packagenode.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtshr_stringhelpers.h>
#include <regex>

namespace {
using namespace Codethink;

std::string fixQName(const std::string& qname)
{
    return std::regex_replace(qname, std::regex("\\\\"), "/");
}

} // namespace

namespace Codethink::lvtldr {

using namespace Codethink::lvtshr;

// ==========================
// class PackageNode
// ==========================

PackageNode::PackageNode(NodeStorage& store,
                         std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler,
                         std::optional<PackageNodeFields> fields):
    LakosianNode(store, dbHandler), d_dbHandler(dbHandler), d_fields(*fields)
{
    d_qualifiedNameParts = NamingUtils::buildQualifiedNamePrefixParts(fixQName(d_fields.qualifiedName), "/");
    setName(d_fields.name);
}

PackageNode::~PackageNode() noexcept = default;

lvtshr::DiagramType PackageNode::type() const
{
    return lvtshr::DiagramType::PackageType;
}

bool PackageNode::isPackageGroup()
{
    // TODO: Properly handle caching system
    d->childrenLoaded = false;
    loadChildren();
    return !d->innerPackages.empty();
}

std::string PackageNode::dirPath() const
{
    return d_fields.diskPath;
}

bool PackageNode::hasRepository() const
{
    return d_fields.sourceRepositoryId.has_value();
}

bool PackageNode::isStandalone() const
{
    if (d_qualifiedNameParts.empty()) {
        return false;
    }
    return d_qualifiedNameParts[0] == "standalones";
}

void PackageNode::removeChildPackage(PackageNode *child)
{
    auto& v = d_fields.childPackagesIds;
    v.erase(std::remove(v.begin(), v.end(), child->id()), v.end());
}

void PackageNode::addConcreteDependency(PackageNode *other)
{
    d_fields.providerIds.emplace_back(other->id());
    other->d_fields.clientIds.emplace_back(d_fields.id);
    d_dbHandler->get().addConcreteDependency(d_fields.id, other->id());
}

void PackageNode::removeConcreteDependency(PackageNode *other)
{
    {
        auto& v = d_fields.providerIds;
        v.erase(std::remove(v.begin(), v.end(), other->id()), v.end());
    }
    {
        auto& v = other->d_fields.clientIds;
        v.erase(std::remove(v.begin(), v.end(), other->id()), v.end());
    }
    d_dbHandler->get().removeConcreteDependency(d_fields.id, other->id());
}

bool PackageNode::hasConcreteDependency(LakosianNode *other)
{
    auto& v = d_fields.providerIds;
    return std::find(v.begin(), v.end(), other->id()) != v.end();
}

std::string PackageNode::canonicalName() const
{
    // A '+' in a package name often indicates externally driven naming exceptions.
    // Example: bsl+bslhdrs
    auto canonicalName = name();
    if (canonicalName.find('+') == 3) {
        canonicalName = canonicalName.substr(0, 3);
    }
    return canonicalName;
}

namespace {
// return true if this element is inside of the searchNode's children, recursive.
bool existsInTree(LakosianNode *entity, LakosianNode *searchNode)
{
    if (entity == searchNode) {
        return true;
    }

    auto const& children = searchNode->children();
    return std::any_of(children.cbegin(), children.cend(), [&entity](LakosianNode *child) {
        return existsInTree(entity, child);
    });
}
} // namespace

cpp::result<void, AddChildError> PackageNode::addChild(LakosianNode *child)
{
    // don't allow packages with circular dependencies
    auto childChildren = child->children();
    if (existsInTree(this, child)) {
        const std::string errorString = "The entity" + name() + "is already connected to" + child->name()
            + "as a child, it can't be set as a parent";
        return cpp::fail(AddChildError{errorString});
    }

    // Don't add the same child twice
    if (std::find(std::begin(d->children), std::end(d->children), child) != std::end(d->children)) {
        const std::string errorString = "The entity" + child->name() + "is already a child of" + name();
        return cpp::fail(AddChildError{errorString});
    }

    d->children.push_back(child);
    if (dynamic_cast<PackageNode *>(child) != nullptr) {
        d->innerPackages.push_back(child);
        d_fields.childPackagesIds.push_back(child->id());
    } else {
        d_fields.childComponentsIds.push_back(child->id());
    }
    auto& dbHandler = d_dbHandler->get();
    dbHandler.updateFields(d_fields);
    Q_EMIT onChildCountChanged(d->children.size());
    return {};
}

void PackageNode::removeChild(LakosianNode *child)
{
    {
        auto& v = d->children;
        v.erase(std::remove_if(v.begin(),
                               v.end(),
                               [&child](auto&& e) {
                                   return e->id() == child->id();
                               }),
                v.end());
    }

    if (dynamic_cast<PackageNode *>(child) != nullptr) {
        {
            auto& v = d->innerPackages;
            v.erase(std::remove_if(v.begin(),
                                   v.end(),
                                   [&child](auto&& e) {
                                       return e->id() == child->id();
                                   }),
                    v.end());
        }
        {
            auto& v = d_fields.childPackagesIds;
            v.erase(std::remove(v.begin(), v.end(), child->id()), v.end());
        }
    } else {
        auto& v = d_fields.childComponentsIds;
        v.erase(std::remove(v.begin(), v.end(), child->id()), v.end());
    }
    d_dbHandler->get().updateFields(d_fields);
    Q_EMIT onChildCountChanged(d->children.size());
}

void PackageNode::setName(std::string const& newName)
{
    LakosianNode::setName(newName);
    d_fields.name = name();
    d_fields.qualifiedName = qualifiedName();
    d_dbHandler->get().updateFields(d_fields);
}

std::string PackageNode::qualifiedName() const
{
    return NamingUtils::buildQualifiedName(d_qualifiedNameParts, name(), "/");
}

std::string PackageNode::parentName()
{
    auto *parentEntity = parent();
    if (!parentEntity) {
        return "";
    }
    return parentEntity->name();
}

long long PackageNode::id() const
{
    return d_fields.id;
}

lvtshr::UniqueId PackageNode::uid() const
{
    return {lvtshr::DiagramType::PackageType, id()};
}

LakosianNode::IsLakosianResult PackageNode::isLakosian()
{
    // Package and package group naming rules are available at
    // https://github.com/bloomberg/bde/wiki/Physical-Code-Organization
    auto *parentPkg = parent();

    // Standalone package naming rules
    if (!parentPkg && QString::fromStdString(name()).startsWith("s_")) {
        auto activeName = name();
        // 2 for s_ + 3-to-6 for the actual package name
        if (activeName.size() < 2 + 3 || activeName.size() > 2 + 6) {
            return IsLakosianResult::PackageNameInvalidNumberOfChars;
        }
        return IsLakosianResult::IsLakosian;
    }

    // Package naming rules
    if (parentPkg) {
        if (!parentPkg->isPackageGroup()) {
            return IsLakosianResult::PackageParentIsNotGroup;
        }

        // A '+' in a package name often indicates externally driven naming exceptions.
        // Example: bsl+bslhdrs
        auto activeName = name();
        if (activeName.find('+') == 3) {
            activeName = activeName.substr(0, 3);
            if (activeName != parentPkg->name()) {
                return IsLakosianResult::PackagePrefixDiffersFromGroup;
            }
        }

        if (!lvtshr::StrUtil::beginsWith(activeName, parentPkg->name())) {
            return IsLakosianResult::PackagePrefixDiffersFromGroup;
        }

        if (activeName.size() < 3 || activeName.size() > 6) {
            return IsLakosianResult::PackageNameInvalidNumberOfChars;
        }
        return IsLakosianResult::IsLakosian;
    }

    // Package Group naming rules
    if (name().size() != 3) {
        return IsLakosianResult::PackageGroupNameInvalidNumberOfChars;
    }
    return IsLakosianResult::IsLakosian;
}

void PackageNode::loadParent()
{
    if (d->parentLoaded) {
        return;
    }
    d->parentLoaded = true;

    d->parent = nullptr;
    if (d_fields.sourceRepositoryId) {
        d->parent = d->store.findById({DiagramType::RepositoryType, *d_fields.sourceRepositoryId});
    } else if (d_fields.parentId) {
        d->parent = d->store.findById({DiagramType::PackageType, *d_fields.parentId});
    }
}

void PackageNode::loadChildren()
{
    if (d->childrenLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getPackageFieldsById(d_fields.id);
    d->childrenLoaded = true;

    auto pkgChildrenIds = d_fields.childPackagesIds;
    auto compChildrenIds = d_fields.childComponentsIds;
    d->children.clear();
    d->innerPackages.clear();
    d->children.reserve(pkgChildrenIds.size() + compChildrenIds.size());
    d->innerPackages.reserve(pkgChildrenIds.size());

    for (auto& id : pkgChildrenIds) {
        LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
        assert(node);
        d->children.push_back(node);
        d->innerPackages.push_back(node);
    }

    for (auto& id : compChildrenIds) {
        LakosianNode *node = d->store.findById({DiagramType::ComponentType, id});
        assert(node);
        d->children.push_back(node);
    }
}

void PackageNode::loadProviders()
{
    if (d->providersLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getPackageFieldsById(d_fields.id);
    d->providersLoaded = true;

    if (d_fields.parentId) {
        // package
        for (auto&& id : d_fields.providerIds) {
            LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
            d->providers.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
        }
    } else {
        // package group
        for (auto&& id : d_fields.providerIds) {
            LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
            d->providers.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
        }

        for (auto&& childId : d_fields.childPackagesIds) {
            auto *child = d->store.findById({DiagramType::PackageType, childId});
            for (auto&& edge : child->providers()) {
                auto *providerPkgGroup = edge.other()->parent();
                if (!providerPkgGroup) {
                    continue;
                }
                if (providerPkgGroup->id() == id()) {
                    continue;
                }
                d->providers.emplace_back(LakosianEdge{lvtshr::PackageDependency, providerPkgGroup});
            }
        }
    }
}

void PackageNode::loadClients()
{
    if (d->clientsLoaded) {
        return;
    }
    d_fields = d_dbHandler->get().getPackageFieldsById(d_fields.id);
    d->clientsLoaded = true;

    if (d_fields.parentId) {
        // package
        for (auto&& id : d_fields.clientIds) {
            LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
            d->clients.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
        }
    } else {
        // package group
        for (auto&& id : d_fields.clientIds) {
            LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
            d->clients.emplace_back(LakosianEdge{lvtshr::PackageDependency, node});
        }

        for (auto&& childId : d_fields.childPackagesIds) {
            auto *child = d->store.findById({DiagramType::PackageType, childId});
            for (auto&& edge : child->clients()) {
                auto *clientPkgGroup = edge.other()->parent();
                if (!clientPkgGroup) {
                    continue;
                }
                if (clientPkgGroup->id() == id()) {
                    continue;
                }
                d->clients.emplace_back(LakosianEdge{lvtshr::PackageDependency, clientPkgGroup});
            }
        }
    }
}

} // namespace Codethink::lvtldr
