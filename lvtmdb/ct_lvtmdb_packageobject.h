// ct_lvtmdb_packageobject.h                                          -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_PACKAGEOBJECT
#define INCLUDED_CT_LVTMDB_PACKAGEOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class RepositoryObject;
class ComponentObject;
class FileObject;
class TypeObject;

// ====================
// class PackageObject
// ====================

class LVTMDB_EXPORT PackageObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    PackageObject *d_parent_p;
    // Null if this is a package group

    std::vector<PackageObject *> d_children;
    // Child packages (if this is a package group)

    // deliberately skip SourcePackage::sourceFiles()
    // because this isn't needed now wew have components

    std::vector<ComponentObject *> d_components;

    std::vector<PackageObject *> d_forwardDeps;

    std::vector<PackageObject *> d_reverseDeps;

    std::vector<TypeObject *> d_types;

    std::string d_diskPath;

    RepositoryObject *d_repository;
    // Null if inside no repository

  public:
    // CREATORS
    PackageObject(std::string qualifiedName,
                  std::string name,
                  std::string path,
                  PackageObject *parent,
                  RepositoryObject *repository);

    ~PackageObject() noexcept override;

    PackageObject(PackageObject&& other) noexcept;

    PackageObject& operator=(PackageObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] RepositoryObject *repository() const;

    [[nodiscard]] PackageObject *parent() const;

    [[nodiscard]] const std::vector<PackageObject *>& children() const;

    [[nodiscard]] const std::vector<ComponentObject *>& components() const;

    [[nodiscard]] const std::vector<PackageObject *>& forwardDependencies() const;

    [[nodiscard]] const std::vector<PackageObject *>& reverseDependencies() const;

    [[nodiscard]] const std::vector<TypeObject *>& types() const;

    [[nodiscard]] const std::string& diskPath() const;

    // MANIPULATORS
    void addChild(PackageObject *pkg);
    void removeChild(PackageObject *pkg);

    // pkg->parent() should already point to this
    // an exclusive lock on this is required before calling this method

    void addComponent(ComponentObject *component);
    // adds component to this package
    // an exclusive lock on this is required before calling this method

    void removeComponent(ComponentObject *component);

    void addType(TypeObject *type);
    // adds a type to this package
    // an exclusive lock on this is required before calling this method

    void removeType(TypeObject *type);

    // CLASS METHODS
    static void addDependency(PackageObject *source, PackageObject *target);
    // On calling this function, neither source nor target should be locked
    // (readOnly or for writing). This function handles locking internally.
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_PACKAGEOBJECT
