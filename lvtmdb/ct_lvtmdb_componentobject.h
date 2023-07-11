// ct_lvtmdb_componentobject.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_COMPONENTOBJECT
#define INCLUDED_CT_LVTMDB_COMPONENTOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class FileObject;
class PackageObject;
class TypeObject;

// ======================
// class ComponentObject
// ======================

class LVTMDB_EXPORT ComponentObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    std::vector<FileObject *> d_files;

    PackageObject *d_package_p;

    std::vector<ComponentObject *> d_forwardDeps;
    // Components this components has physical dependency upon

    std::vector<ComponentObject *> d_reverseDeps;
    // Components with physical dependency upon this component

    std::vector<TypeObject *> d_types;
    // Types in this component

  public:
    // CREATORS
    ComponentObject(std::string qualifiedName, std::string name, PackageObject *package);

    ~ComponentObject() noexcept override;

    ComponentObject(ComponentObject&& other) noexcept;

    ComponentObject& operator=(ComponentObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] const std::vector<FileObject *>& files() const;

    [[nodiscard]] PackageObject *package() const;

    [[nodiscard]] const std::vector<ComponentObject *>& forwardDependencies() const;

    [[nodiscard]] const std::vector<ComponentObject *>& reverseDependencies() const;

    [[nodiscard]] const std::vector<TypeObject *>& types() const;

    // MODIFIERS
    void addFile(FileObject *file);
    // Requires an exclusive lock on this component. Does not modify the file

    void removeFile(FileObject *file);

    void addType(TypeObject *type);
    // Requires an exclusive lock on this component. Does not modify the file

    // CLASS METHODS
    static void addDependency(ComponentObject *source, ComponentObject *target);
    // On calling this function, neither source nor target should be locked
    // (readOnly or for writing). This function handles locking internally.
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_COMPONENTOBJECT
