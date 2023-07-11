// ct_lvtmdb_componentobject.cpp                                      -*-C++-*-

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

#include <ct_lvtmdb_componentobject.h>

#include <ct_lvtmdb_util.h>

#include <cassert>

namespace Codethink::lvtmdb {

ComponentObject::ComponentObject(std::string qualifiedName, std::string name, PackageObject *package):
    DatabaseObject(std::move(qualifiedName), std::move(name)), d_package_p(package)
{
}

ComponentObject::~ComponentObject() noexcept = default;

ComponentObject::ComponentObject(ComponentObject&&) noexcept = default;

ComponentObject& ComponentObject::operator=(ComponentObject&&) noexcept = default;

const std::vector<FileObject *>& ComponentObject::files() const
{
    assertReadable();
    return d_files;
}

PackageObject *ComponentObject::package() const
{
    assertReadable();
    return d_package_p;
}

const std::vector<ComponentObject *>& ComponentObject::forwardDependencies() const
{
    assertReadable();
    return d_forwardDeps;
}

const std::vector<ComponentObject *>& ComponentObject::reverseDependencies() const
{
    assertReadable();
    return d_reverseDeps;
}

const std::vector<TypeObject *>& ComponentObject::types() const
{
    assertReadable();
    return d_types;
}

void ComponentObject::addFile(FileObject *file)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_files, file);
}

void ComponentObject::removeFile(FileObject *file)
{
    assertWritable();
    d_files.erase(std::remove(d_files.begin(), d_files.end(), file), d_files.end());
}

void ComponentObject::addType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_types, type);
}

void ComponentObject::addDependency(ComponentObject *source, ComponentObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_forwardDeps, target->d_reverseDeps);
}

} // namespace Codethink::lvtmdb
