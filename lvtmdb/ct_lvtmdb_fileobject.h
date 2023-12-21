// ct_lvtmdb_fileobject.h                                             -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_FILEOBJECT
#define INCLUDED_CT_LVTMDB_FILEOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class ComponentObject;
class NamespaceObject;
class PackageObject;
class TypeObject;
class FunctionObject;

// ===================
// class FileObject
// ===================

class LVTMDB_EXPORT FileObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    bool d_isHeader;

    std::string d_hash;
    // MD5 hash of the file at the time it was added to the database

    std::vector<FileObject *> d_forwardIncludes;
    // Files included by this file
    // Pointers owned by ObjectStore

    std::vector<FileObject *> d_reverseIncludes;
    // Files which include this file
    // Pointers owned by ObjectStore

    PackageObject *d_package_p;
    ComponentObject *d_component_p;

    std::vector<NamespaceObject *> d_namespaces;

    std::vector<TypeObject *> d_types;

    std::vector<FunctionObject *> d_globalfunctions;

  public:
    // CREATORS
    // TODO: Component already has "package", remove this from the constructor.
    FileObject(std::string qualifiedName,
               std::string name,
               bool isHeader,
               std::string hash,
               PackageObject *package,
               ComponentObject *component);

    ~FileObject() noexcept override;

    FileObject(FileObject&& other) noexcept;

    FileObject& operator=(FileObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] bool isHeader() const;

    [[nodiscard]] const std::string& hash() const;

    [[nodiscard]] const std::vector<FileObject *>& forwardIncludes() const;

    [[nodiscard]] const std::vector<FileObject *>& reverseIncludes() const;

    [[nodiscard]] PackageObject *package() const;

    [[nodiscard]] ComponentObject *component() const;

    [[nodiscard]] const std::vector<NamespaceObject *>& namespaces() const;

    [[nodiscard]] const std::vector<FunctionObject *>& globalFunctions() const;

    [[nodiscard]] const std::vector<TypeObject *>& types() const;

    // MANIPULATORS
    void setHash(std::string hash);
    // Update the stored hash for a file

    void addNamespace(NamespaceObject *nmspc);
    // An exclusive lock on this is required before calling this method

    void addGlobalFunction(FunctionObject *fnc);
    // An exclusive lock on this is required before calling this method

    void addType(TypeObject *type);
    // An exclusive lock on this is required before calling this method

    // CLASS METHODS
    static void addIncludeRelation(FileObject *source, FileObject *target);
    // On calling this function, neither source nor target should be locked
    // (readOnly or for writing). This function handles locking internally.

    static void removeIncludeRelation(FileObject *source, FileObject *target);
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_FILEOBJECT
