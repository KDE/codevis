// ct_lvtldr_repositorynode.cpp                                      -*-C++-*-

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
#include <ct_lvtldr_repositorynode.h>

namespace Codethink::lvtldr {

using namespace Codethink::lvtshr;

// ==========================
// class RepositoryNode
// ==========================

RepositoryNode::RepositoryNode(NodeStorage& store,
                               std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler,
                               std::optional<RepositoryNodeFields> fields):
    LakosianNode(store, dbHandler), d_dbHandler(dbHandler), d_fields(*fields)
{
    setName(d_fields.name);
}

RepositoryNode::~RepositoryNode() noexcept = default;

lvtshr::DiagramType RepositoryNode::type() const
{
    return lvtshr::DiagramType::RepositoryType;
}

bool RepositoryNode::isPackageGroup()
{
    return false;
}

std::string RepositoryNode::qualifiedName() const
{
    return d_fields.qualifiedName;
}

std::string RepositoryNode::parentName()
{
    return "";
}

long long RepositoryNode::id() const
{
    return d_fields.id;
}

lvtshr::UniqueId RepositoryNode::uid() const
{
    return {lvtshr::DiagramType::RepositoryType, id()};
}

LakosianNode::IsLakosianResult RepositoryNode::isLakosian()
{
    return IsLakosianResult::IsLakosian;
}

cpp::result<void, AddChildError> RepositoryNode::addChild(LakosianNode *child)
{
    return cpp::fail(AddChildError{"Dynamically adding children to a repository is not supported."});
}

void RepositoryNode::loadParent()
{
    d->parentLoaded = true;
    d->parent = nullptr;
}

void RepositoryNode::loadChildrenIds()
{
    // TODO: IMPLEMENT ME
}

void RepositoryNode::loadChildren()
{
    if (d->childrenLoaded) {
        return;
    }

    auto pkgChildrenIds = d_fields.childPackagesIds;
    d->children.clear();
    d->innerPackages.clear();
    d->children.reserve(pkgChildrenIds.size());
    d->innerPackages.reserve(pkgChildrenIds.size());
    for (auto& id : pkgChildrenIds) {
        LakosianNode *node = d->store.findById({DiagramType::PackageType, id});
        assert(node);
        d->children.push_back(node);
        d->innerPackages.push_back(node);
    }
    d->childrenLoaded = true;
}

void RepositoryNode::loadProviders()
{
    d->providersLoaded = true;
}

void RepositoryNode::loadClients()
{
    d->clientsLoaded = true;
}

} // namespace Codethink::lvtldr
