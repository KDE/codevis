// ct_lvtmdb_fileobject.cpp                                           -*-C++-*-

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

#include <ct_lvtmdb_fileobject.h>

#include <ct_lvtmdb_util.h>

#include <cassert>
#include <iostream>

namespace Codethink::lvtmdb {

FileObject::FileObject(std::string qualifiedName,
                       std::string name,
                       bool isHeader,
                       std::string hash,
                       PackageObject *package,
                       ComponentObject *component):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_isHeader(isHeader),
    d_hash(std::move(hash)),
    d_package_p(package),
    d_component_p(component)
{
}

FileObject::~FileObject() noexcept = default;

FileObject::FileObject(FileObject&&) noexcept = default;

FileObject& FileObject::operator=(FileObject&&) noexcept = default;

bool FileObject::isHeader() const
{
    assertReadable();
    return d_isHeader;
}

const std::string& FileObject::hash() const
{
    assertReadable();
    return d_hash;
}

const std::vector<FileObject *>& FileObject::forwardIncludes() const
{
    assertReadable();
    return d_forwardIncludes;
}

const std::vector<FileObject *>& FileObject::reverseIncludes() const
{
    assertReadable();
    return d_reverseIncludes;
}

PackageObject *FileObject::package() const
{
    assertReadable();
    return d_package_p;
}

ComponentObject *FileObject::component() const
{
    assertReadable();
    return d_component_p;
}

const std::vector<NamespaceObject *>& FileObject::namespaces() const
{
    assertReadable();
    return d_namespaces;
}

const std::vector<TypeObject *>& FileObject::types() const
{
    assertReadable();
    return d_types;
}

void FileObject::setHash(std::string hash)
{
    assertWritable();
    d_hash = std::move(hash);
}

void FileObject::addNamespace(NamespaceObject *nmspc)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_namespaces, nmspc);
}

void FileObject::addType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_types, type);
}

void FileObject::addIncludeRelation(FileObject *source, FileObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_forwardIncludes, target->d_reverseIncludes);
}

void FileObject::removeIncludeRelation(FileObject *source, FileObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_forwardIncludes, target->d_reverseIncludes);
}

} // namespace Codethink::lvtmdb
