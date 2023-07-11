// ct_lvtmdb_typeobject.h                                             -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_TYPEOBJECT
#define INCLUDED_CT_LVTMDB_TYPEOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class ComponentObject;
class FieldObject;
class FileObject;
class MethodObject;
class NamespaceObject;
class PackageObject;

// ====================
// class PackageObject
// ====================

class LVTMDB_EXPORT TypeObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    // DATA
    lvtshr::UDTKind d_kind;
    // What kind of type is this? e.g. class, struct, union, enum

    lvtshr::AccessSpecifier d_access;

    NamespaceObject *d_namespace_p;
    // Immediate parent namespace

    PackageObject *d_package_p;
    // Immediate parent package

    TypeObject *d_parent_p;
    // C++ type enclosing this type (or nullptr)
    // This maps to UserDefinedType::d_classNamespace_p **not** ::d_parents

    std::vector<TypeObject *> d_children;
    // C++ types nested inside this type
    // This maps to UserDefinedType::d_revClassnamespace **not** ::d_children

    std::vector<TypeObject *> d_subclasses;
    // subclasses of this type
    // Maps to UserDefinedType::d_children

    std::vector<TypeObject *> d_superclasses;
    // superclasses of this type
    // Maps to UserDefinedType::d_parents

    std::vector<TypeObject *> d_usesInTheInterface;
    std::vector<TypeObject *> d_revUsesInTheInterface;

    std::vector<TypeObject *> d_usesInTheImplementation;
    std::vector<TypeObject *> d_revUsesInTheImplementation;

    std::vector<FileObject *> d_files;
    // Soruce files in which this type is defined

    std::vector<ComponentObject *> d_components;
    // Components in which this type is defined

    std::vector<MethodObject *> d_methods;
    // Methods of this type

    std::vector<FieldObject *> d_fields;
    // Data members of this type

  public:
    // CREATORS
    TypeObject(std::string qualifiedName,
               std::string name,
               lvtshr::UDTKind kind,
               lvtshr::AccessSpecifier access,
               NamespaceObject *nmspc,
               PackageObject *package,
               TypeObject *parent);

    ~TypeObject() noexcept override;

    TypeObject(TypeObject&& other) noexcept;

    TypeObject& operator=(TypeObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] lvtshr::UDTKind kind() const;

    [[nodiscard]] lvtshr::AccessSpecifier access() const;

    [[nodiscard]] NamespaceObject *parentNamespace() const;

    [[nodiscard]] PackageObject *package() const;

    [[nodiscard]] TypeObject *parent() const;
    [[nodiscard]] const std::vector<TypeObject *>& children() const;

    [[nodiscard]] const std::vector<TypeObject *>& subclasses() const;
    [[nodiscard]] const std::vector<TypeObject *>& superclasses() const;

    [[nodiscard]] const std::vector<TypeObject *>& usesInTheInterface() const;
    [[nodiscard]] const std::vector<TypeObject *>& revUsesInTheInterface() const;

    [[nodiscard]] const std::vector<TypeObject *>& usesInTheImplementation() const;
    [[nodiscard]] const std::vector<TypeObject *>& revUsesInTheImplementation() const;

    [[nodiscard]] const std::vector<FileObject *>& files() const;

    [[nodiscard]] const std::vector<ComponentObject *>& components() const;

    [[nodiscard]] const std::vector<MethodObject *>& methods() const;

    [[nodiscard]] const std::vector<FieldObject *>& fields() const;

    // MANIPULATORS
    void setPackage(PackageObject *package);
    void addChild(TypeObject *child);
    void addFile(FileObject *file);
    void setUniqueFile(FileObject *file);

    void removeFile(FileObject *file);
    void addComponent(ComponentObject *comp);
    void setUniqueComponent(ComponentObject *comp);
    void addMethod(MethodObject *method);
    void addField(FieldObject *field);

    // CLASS METHODS
    static void addIsARelationship(TypeObject *subclass, TypeObject *superclass);
    // On calling this function, neither subclass nor superclass should be
    // locked

    static void addUsesInTheInterface(TypeObject *source, TypeObject *target);
    // On calling this function, neither source nor target should be
    // locked

    static void addUsesInTheImplementation(TypeObject *source, TypeObject *target);
    // On calling this function, neither source nor target should be
    // locked
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_TYPEOBJECT
