// ct_lvtmdb_fieldobject.h                                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_FIELDOBJECT
#define INCLUDED_CT_LVTMDB_FIELDOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>
#include <ct_lvtmdb_util.h>

#include <ct_lvtshr_graphenums.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class TypeObject;

// ===================
// class FieldObject
// ===================

class LVTMDB_EXPORT FieldObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    std::string d_signature;
    // Type signature of this field

    lvtshr::AccessSpecifier d_access;
    // Access mode of the field

    bool d_isStatic;
    // True if the field is static

    TypeObject *d_parent_p;
    // The UDT containing this field

    std::vector<TypeObject *> d_variableTypes;
    // A list of classes which are used in this field's type and used in
    // any templates that might make up this field's type

  public:
    // CREATORS
    FieldObject(std::string qualifiedName,
                std::string name,
                std::string signature,
                lvtshr::AccessSpecifier access,
                bool isStatic,
                TypeObject *parent);

    ~FieldObject() noexcept override;

    FieldObject(FieldObject&& other) noexcept;

    FieldObject& operator=(FieldObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] const std::string& signature() const;

    [[nodiscard]] lvtshr::AccessSpecifier access() const;

    [[nodiscard]] bool isStatic() const;

    [[nodiscard]] TypeObject *parent() const;

    [[nodiscard]] const std::vector<TypeObject *>& variableTypes() const;

    // MODIFIERS
    void addType(TypeObject *type);
    // an exclusive lock on this is required before calling this method
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_FIELDOBJECT
