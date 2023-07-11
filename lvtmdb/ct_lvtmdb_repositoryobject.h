// ct_lvtmdb_repositoryobject.h                                      -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_REPOSITORYOBJECT
#define INCLUDED_CT_LVTMDB_REPOSITORYOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class PackageObject;

// ====================
// class RepositoryObject
// ====================

class LVTMDB_EXPORT RepositoryObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    std::vector<PackageObject *> d_children;
    std::string d_diskPath;

  public:
    // CREATORS
    RepositoryObject(std::string qualifiedName, std::string name, std::string path);

    ~RepositoryObject() noexcept override;

    RepositoryObject(RepositoryObject&& other) noexcept;

    RepositoryObject& operator=(RepositoryObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] const std::vector<PackageObject *>& children() const;
    [[nodiscard]] const std::string& diskPath() const;

    // MANIPULATORS
    void addChild(PackageObject *pkg);
    // pkg->parent() should already point to this
    // an exclusive lock on this is required before calling this method
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_PACKAGEOBJECT
