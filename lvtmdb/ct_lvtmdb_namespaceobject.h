// ct_lvtmdb_namespaceobject.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_NAMESPACEOBJECT
#define INCLUDED_CT_LVTMDB_NAMESPACEOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class FileObject;
class FunctionObject;
class TypeObject;
class VariableObject;

// ======================
// class NamespaceObject
// ======================

class LVTMDB_EXPORT NamespaceObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    NamespaceObject *d_parent_p;
    // Null if this is a package group

    std::vector<NamespaceObject *> d_children;
    // Child namespaces

    std::vector<TypeObject *> d_typeChildren;

    std::vector<FileObject *> d_files;

    std::vector<FunctionObject *> d_functions;

    std::vector<VariableObject *> d_variables;

  public:
    // CREATORS
    NamespaceObject(std::string qualifiedName, std::string name, NamespaceObject *parent);

    ~NamespaceObject() noexcept override;

    NamespaceObject(NamespaceObject&& other) noexcept;

    NamespaceObject& operator=(NamespaceObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] NamespaceObject *parent() const;

    [[nodiscard]] const std::vector<NamespaceObject *>& children() const;

    [[nodiscard]] const std::vector<TypeObject *>& typeChildren() const;

    [[nodiscard]] const std::vector<FileObject *>& files() const;

    [[nodiscard]] const std::vector<FunctionObject *>& functions() const;

    [[nodiscard]] const std::vector<VariableObject *>& variables() const;

    // MANIPULATORS
    void addChild(NamespaceObject *child);
    // child->parent() should already point to this
    // an exclusive lock on this is required before calling this method

    void removeChild(NamespaceObject *child);

    void addType(TypeObject *type);
    // The type should already know that it belongs to this namespace
    // an exclusive lock on this is required before calling this method

    void removeType(TypeObject *type);

    void addFile(FileObject *file);
    // The file should already know that it belongs to this namespace
    // an exclusive lock on this is required before calling this method

    void removeFile(FileObject *file);
    // The file should already know that it belongs to this namespace
    // an exclusive lock on this is required before calling this method

    void addFunction(FunctionObject *function);
    // The function should already know that it belongs to this namespace
    // an exclusive lock on this is required before calling this method

    void removeFunction(FunctionObject *function);

    void addVariable(VariableObject *variable);
    // The variable should already know that it belongs to this namespace
    // an exclusive lock on this is required before calling this method

    void removeVariable(VariableObject *variable);
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_NAMESPACEOBJECT
