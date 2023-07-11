// ct_lvtmdb_databaseobject.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_DATABASEOBJECT
#define INCLUDED_CT_LVTMDB_DATABASEOBJECT

//@PURPOSE: Share code between lvtmdb::*Object

#include <lvtmdb_export.h>

#include <ct_lvtmdb_lockable.h>

#include <algorithm>
#include <string>
#include <type_traits>

namespace Codethink::lvtmdb {

// =====================
// class DatabaseObject
// =====================

class LVTMDB_EXPORT DatabaseObject : public Lockable {
    // See locking discipline in superclass. That applies here.

  private:
    std::string d_qualifiedName;
    // Full path of this variable

    std::string d_name;
    // Name of this variable

  public:
    // CREATORS
    DatabaseObject(std::string qualifiedName, std::string name);

    ~DatabaseObject() noexcept override;

    DatabaseObject(DatabaseObject&& other) noexcept;

    DatabaseObject& operator=(DatabaseObject&& other) noexcept;

    // ACCESSORS
#ifdef LVTMDB_LOCK_DEBUGGING
    [[nodiscard]] const std::string& debugName() const;
#endif

    [[nodiscard]] const std::string& qualifiedName() const;

    [[nodiscard]] const std::string& name() const;

    // CLASS METHODS
    template<class DERIVED>
    static void addPeerRelationship(DERIVED *source,
                                    DERIVED *target,
                                    std::vector<DERIVED *>& sourceList,
                                    std::vector<DERIVED *>& targetList)
    // add a peer relationship between source and target
    // for example, when adding a package dependency relationship:
    //     DERIVED would be PackageObject
    //     sourceList would be source.d_forwardDeps
    //     targetList would be target.d_reverseDeps
    {
        static_assert(std::is_base_of_v<DatabaseObject, DERIVED>);

        if (source == target) {
            // deadlock
            return;
        }

        // check if the relationship already exists. This lets us avoid getting any
        // exclusive locks
        {
            auto lock = source->readOnlyLock();
            (void) lock; // cppcheck
            const auto it = std::find(sourceList.begin(), sourceList.end(), target);
            if (it != sourceList.end()) {
                return;
            }
        }

        auto locks = Lockable::rwLockTwo(source, target);
        (void) locks; // cppcheck

        // we already checked for duplicates above
        sourceList.push_back(target);
        targetList.push_back(source);
    }

    template<class DERIVED>
    static void removePeerRelationship(DERIVED *source,
                                       DERIVED *target,
                                       std::vector<DERIVED *>& sourceList,
                                       std::vector<DERIVED *>& targetList)
    // remove a peer relationship between source and target
    {
        static_assert(std::is_base_of_v<DatabaseObject, DERIVED>);

        if (source == target) {
            // deadlock
            return;
        }

        auto locks = Lockable::rwLockTwo(source, target);
        (void) locks; // cppcheck

        // we already checked for duplicates above
        sourceList.erase(std::remove(sourceList.begin(), sourceList.end(), target), sourceList.end());
        targetList.erase(std::remove(targetList.begin(), targetList.end(), source), targetList.end());
    }
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_DATABASEOBJECT
