// ct_lvtmdb_functionbase.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_FUNCTIONBASE
#define INCLUDED_CT_LVTMDB_FUNCTIONBASE

//@PURPOSE: Share code for method and function declarations

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>

#include <string>

namespace Codethink::lvtmdb {

// Forward Declarations
class NamespaceObject;

// =====================
// class FunctionBase
// =====================

class LVTMDB_EXPORT FunctionBase : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    std::string d_signature;
    // Type signature of this function

    std::string d_returnType;
    // The return type of the function

    std::string d_templateParameters;
    // Optional template parameters for the function

  public:
    // CREATORS
    FunctionBase(std::string qualifiedName,
                 std::string name,
                 std::string signature,
                 std::string returnType,
                 std::string templateParameters);

    ~FunctionBase() noexcept override;

    FunctionBase(FunctionBase&& other) noexcept;

    FunctionBase& operator=(FunctionBase&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] const std::string& signature() const;

    [[nodiscard]] const std::string& returnType() const;

    [[nodiscard]] const std::string& templateParameters() const;

    [[nodiscard]] std::string storageKey() const;
    // Functions with the same qualifiedName can be overloaded. Use this
    // key to get a unique string

    // CLASS METHODS
    static std::string getStorageKey(const std::string& qualifiedName,
                                     const std::string& signature,
                                     const std::string& templateParameters,
                                     const std::string& returnType);
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_FUNCTIONBASE
