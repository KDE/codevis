// ct_lvtmdb_typeobject.cpp

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

#include "ct_lvtmdb_componentobject.h"
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtmdb_util.h>

#include <cassert>

namespace Codethink::lvtmdb {

TypeObject::TypeObject(std::string qualifiedName,
                       std::string name,
                       lvtshr::UDTKind kind,
                       lvtshr::AccessSpecifier access,
                       NamespaceObject *nmspc,
                       PackageObject *package,
                       TypeObject *parent):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_kind(kind),
    d_access(access),
    d_namespace_p(nmspc),
    d_package_p(package),
    d_parent_p(parent)
{
}

TypeObject::~TypeObject() noexcept = default;

TypeObject::TypeObject(TypeObject&&) noexcept = default;

TypeObject& TypeObject::operator=(TypeObject&&) noexcept = default;

lvtshr::UDTKind TypeObject::kind() const
{
    assertReadable();
    return d_kind;
}

lvtshr::AccessSpecifier TypeObject::access() const
{
    assertReadable();
    return d_access;
}

NamespaceObject *TypeObject::parentNamespace() const
{
    assertReadable();
    return d_namespace_p;
}

PackageObject *TypeObject::package() const
{
    assertReadable();
    return d_package_p;
}

TypeObject *TypeObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

const std::vector<TypeObject *>& TypeObject::children() const
{
    assertReadable();
    return d_children;
}

const std::vector<TypeObject *>& TypeObject::subclasses() const
{
    assertReadable();
    return d_subclasses;
}

const std::vector<TypeObject *>& TypeObject::superclasses() const
{
    assertReadable();
    return d_superclasses;
}

const std::vector<TypeObject *>& TypeObject::usesInTheInterface() const
{
    assertReadable();
    return d_usesInTheInterface;
}

const std::vector<TypeObject *>& TypeObject::revUsesInTheInterface() const
{
    assertReadable();
    return d_revUsesInTheInterface;
}

const std::vector<TypeObject *>& TypeObject::usesInTheImplementation() const
{
    assertReadable();
    return d_usesInTheImplementation;
}

const std::vector<TypeObject *>& TypeObject::revUsesInTheImplementation() const
{
    assertReadable();
    return d_revUsesInTheImplementation;
}

const std::vector<FileObject *>& TypeObject::files() const
{
    assertReadable();
    return d_files;
}

const std::vector<ComponentObject *>& TypeObject::components() const
{
    assertReadable();
    return d_components;
}

const std::vector<MethodObject *>& TypeObject::methods() const
{
    assertReadable();
    return d_methods;
}

const std::vector<FieldObject *>& TypeObject::fields() const
{
    assertReadable();
    return d_fields;
}

void TypeObject::setPackage(PackageObject *package)
{
    assertWritable();
    d_package_p = package;
}

void TypeObject::addChild(TypeObject *child)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_children, child);
}

void TypeObject::addFile(FileObject *file)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_files, file);
}

void TypeObject::removeFile(FileObject *file)
{
    assertWritable();
    d_files.erase(std::remove(d_files.begin(), d_files.end(), file), d_files.end());
}

void TypeObject::addComponent(ComponentObject *component)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_components, component);
}

void TypeObject::addMethod(MethodObject *method)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_methods, method);
}

void TypeObject::addField(FieldObject *field)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_fields, field);
}

void TypeObject::addIsARelationship(TypeObject *subclass, TypeObject *superclass)
{
    assert(subclass);
    assert(superclass);
    addPeerRelationship(subclass, superclass, subclass->d_superclasses, superclass->d_subclasses);
}

void TypeObject::addUsesInTheInterface(TypeObject *source, TypeObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_usesInTheInterface, target->d_revUsesInTheInterface);
}

void TypeObject::addUsesInTheImplementation(TypeObject *source, TypeObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_usesInTheImplementation, target->d_revUsesInTheImplementation);
}

void TypeObject::setUniqueFile(FileObject *file)
{
    assertWritable();
    d_files.clear();
    d_files.push_back(file);
}

void TypeObject::setUniqueComponent(ComponentObject *comp)
{
    assertWritable();
    d_components.clear();
    d_components.push_back(comp);
}

} // namespace Codethink::lvtmdb
