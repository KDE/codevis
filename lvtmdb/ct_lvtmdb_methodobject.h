// ct_lvtmdb_methodobject.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_METHODOBJECT
#define INCLUDED_CT_LVTMDB_METHODOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_functionbase.h>

#include <ct_lvtshr_graphenums.h>

#include <string>
#include <vector>

namespace Codethink::lvtmdb {

// Forward Declarations
class TypeObject;

// =====================
// class MethodObject
// =====================

class LVTMDB_EXPORT MethodObject : public FunctionBase {
    // See locking discipline in superclass. That applies here.

  private:
    // See lvtmdb::FunctionBase for qualifiedName etc

    lvtshr::AccessSpecifier d_access;

    bool d_isVirtual;
    bool d_isPure;
    bool d_isStatic;
    bool d_isConst;

    TypeObject *d_parent_p;

    std::vector<TypeObject *> d_argumentTypes;

  public:
    // CREATORS
    MethodObject(std::string qualifiedName,
                 std::string name,
                 std::string signature,
                 std::string returnType,
                 std::string templateParameters,
                 lvtshr::AccessSpecifier access,
                 bool isVirtual,
                 bool isPure,
                 bool isStatic,
                 bool isConst,
                 TypeObject *parent);

    ~MethodObject() noexcept override;

    MethodObject(MethodObject&& other) noexcept;

    MethodObject& operator=(MethodObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] lvtshr::AccessSpecifier access() const;

    [[nodiscard]] bool isVirtual() const;

    [[nodiscard]] bool isPure() const;

    [[nodiscard]] bool isStatic() const;

    [[nodiscard]] bool isConst() const;

    [[nodiscard]] TypeObject *parent() const;

    [[nodiscard]] const std::vector<TypeObject *>& argumentTypes() const;

    // MODIFIERS
    void addArgumentType(TypeObject *type);
    // Needs exclusive lock
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_METHODOBJECT
