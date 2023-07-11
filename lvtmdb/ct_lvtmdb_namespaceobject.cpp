// ct_lvtmdb_namespaceobject.cpp                                      -*-C++-*-

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

#include <ct_lvtmdb_namespaceobject.h>

#include <ct_lvtmdb_util.h>

#include <algorithm>

namespace Codethink::lvtmdb {

NamespaceObject::NamespaceObject(std::string qualifiedName, std::string name, NamespaceObject *parent):
    DatabaseObject(std::move(qualifiedName), std::move(name)), d_parent_p(parent)
{
}

NamespaceObject::~NamespaceObject() noexcept = default;

NamespaceObject::NamespaceObject(NamespaceObject&&) noexcept = default;

NamespaceObject& NamespaceObject::operator=(NamespaceObject&&) noexcept = default;

NamespaceObject *NamespaceObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

const std::vector<NamespaceObject *>& NamespaceObject::children() const
{
    assertReadable();
    return d_children;
}

const std::vector<TypeObject *>& NamespaceObject::typeChildren() const
{
    assertReadable();
    return d_typeChildren;
}

const std::vector<FileObject *>& NamespaceObject::files() const
{
    assertReadable();
    return d_files;
}

const std::vector<FunctionObject *>& NamespaceObject::functions() const
{
    assertReadable();
    return d_functions;
}

const std::vector<VariableObject *>& NamespaceObject::variables() const
{
    assertReadable();
    return d_variables;
}

void NamespaceObject::addChild(NamespaceObject *child)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_children, child);
}

void NamespaceObject::removeChild(NamespaceObject *child)
{
    assertWritable();
    d_children.erase(std::remove(d_children.begin(), d_children.end(), child), d_children.end());
}

void NamespaceObject::addType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_typeChildren, type);
}

void NamespaceObject::removeType(TypeObject *type)
{
    assertWritable();
    d_typeChildren.erase(std::remove(d_typeChildren.begin(), d_typeChildren.end(), type), d_typeChildren.end());
}

void NamespaceObject::addFile(FileObject *file)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_files, file);
}

void NamespaceObject::removeFile(FileObject *file)
{
    assertWritable();
    d_files.erase(std::remove(d_files.begin(), d_files.end(), file), d_files.end());
}

void NamespaceObject::addFunction(FunctionObject *function)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_functions, function);
}

void NamespaceObject::removeFunction(FunctionObject *function)
{
    assertWritable();
    d_functions.erase(std::remove(d_functions.begin(), d_functions.end(), function), d_functions.end());
}

void NamespaceObject::addVariable(VariableObject *variable)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_variables, variable);
}

void NamespaceObject::removeVariable(VariableObject *var)
{
    assertWritable();
    d_variables.erase(std::remove(d_variables.begin(), d_variables.end(), var), d_variables.end());
}

} // namespace Codethink::lvtmdb
