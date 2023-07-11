// ct_lvtmdb_packageobject.cpp                                        -*-C++-*-

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

#include <ct_lvtmdb_packageobject.h>

#include <ct_lvtmdb_util.h>

#include <algorithm>
#include <cassert>

namespace Codethink::lvtmdb {

PackageObject::PackageObject(std::string qualifiedName,
                             std::string name,
                             std::string diskPath,
                             PackageObject *parent,
                             RepositoryObject *repository):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_parent_p(parent),
    d_diskPath(std::move(diskPath)),
    d_repository(repository)
{
}

PackageObject::~PackageObject() noexcept = default;

PackageObject::PackageObject(PackageObject&&) noexcept = default;

PackageObject& PackageObject::operator=(PackageObject&&) noexcept = default;

PackageObject *PackageObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

RepositoryObject *PackageObject::repository() const
{
    assertReadable();
    return d_repository;
}

const std::string& PackageObject::diskPath() const
{
    assertReadable();
    return d_diskPath;
}

const std::vector<PackageObject *>& PackageObject::children() const
{
    assertReadable();
    return d_children;
}

const std::vector<ComponentObject *>& PackageObject::components() const
{
    assertReadable();
    return d_components;
}

const std::vector<PackageObject *>& PackageObject::forwardDependencies() const
{
    assertReadable();
    return d_forwardDeps;
}

const std::vector<PackageObject *>& PackageObject::reverseDependencies() const
{
    assertReadable();
    return d_reverseDeps;
}

const std::vector<TypeObject *>& PackageObject::types() const
{
    assertReadable();
    return d_types;
}

void PackageObject::addChild(PackageObject *pkg)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_children, pkg);
}

void PackageObject::removeChild(PackageObject *pkg)
{
    assertWritable();
    d_children.erase(std::remove(d_children.begin(), d_children.end(), pkg), d_children.end());
}

void PackageObject::addComponent(ComponentObject *component)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_components, component);
}

void PackageObject::removeComponent(ComponentObject *component)
{
    assertWritable();
    d_components.erase(std::remove(d_components.begin(), d_components.end(), component), d_components.end());
}

void PackageObject::addType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_types, type);
}

void PackageObject::removeType(TypeObject *type)
{
    assertWritable();
    d_types.erase(std::remove(d_types.begin(), d_types.end(), type), d_types.end());
}

void PackageObject::addDependency(PackageObject *source, PackageObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_forwardDeps, target->d_reverseDeps);
}

} // namespace Codethink::lvtmdb
