// ct_lvtmdb_variableobject.h                                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_VARIABLEOBJECT
#define INCLUDED_CT_LVTMDB_VARIABLEOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>

namespace Codethink::lvtmdb {

// Forward Declarations
class NamespaceObject;

// ===================
// class VariableObject
// ===================

class LVTMDB_EXPORT VariableObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    std::string d_signature;
    // Type signature of this variable

    bool d_isGlobal;
    // True if the variable isn't in a namespace

    NamespaceObject *d_parent_p;

  public:
    // CREATORS
    VariableObject(
        std::string qualifiedName, std::string name, std::string signature, bool isGlobal, NamespaceObject *parent);

    ~VariableObject() noexcept override;

    VariableObject(VariableObject&& other) noexcept;

    VariableObject& operator=(VariableObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] const std::string& signature() const;

    [[nodiscard]] bool isGlobal() const;

    [[nodiscard]] NamespaceObject *parent() const;
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_VARIABLEOBJECT
