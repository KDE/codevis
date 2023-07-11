// ct_lvtmdb_functionobject.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_FUNCTIONOBJECT
#define INCLUDED_CT_LVTMDB_FUNCTIONOBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_functionbase.h>

#include <string>

namespace Codethink::lvtmdb {

// Forward Declarations
class NamespaceObject;

// =====================
// class FunctionObject
// =====================

class LVTMDB_EXPORT FunctionObject : public FunctionBase {
    // See locking discipline in superclass. That applies here.

  private:
    // See lvtmdb::FunctionBase for qualified name etc

    NamespaceObject *d_parent_p;
    // Immediate parent namespace

  public:
    // CREATORS
    FunctionObject(std::string qualifiedName,
                   std::string name,
                   std::string signature,
                   std::string returnType,
                   std::string templateParameters,
                   NamespaceObject *parent);

    ~FunctionObject() noexcept override;

    FunctionObject(FunctionObject&& other) noexcept;

    FunctionObject& operator=(FunctionObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] NamespaceObject *parent() const;
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_FUNCTIONOBJECT
