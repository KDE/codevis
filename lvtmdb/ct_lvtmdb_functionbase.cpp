// ct_lvtmdb_functionbase.cpp                                         -*-C++-*-

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

#include <ct_lvtmdb_functionbase.h>

namespace Codethink::lvtmdb {

FunctionBase::FunctionBase(std::string qualifiedName,
                           std::string name,
                           std::string signature,
                           std::string returnType,
                           std::string templateParameters):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_signature(std::move(signature)),
    d_returnType(std::move(returnType)),
    d_templateParameters(std::move(templateParameters))
{
}

FunctionBase::~FunctionBase() noexcept = default;

FunctionBase::FunctionBase(FunctionBase&&) noexcept = default;

FunctionBase& FunctionBase::operator=(FunctionBase&&) noexcept = default;

const std::string& FunctionBase::signature() const
{
    assertReadable();
    return d_signature;
}

const std::string& FunctionBase::returnType() const
{
    assertReadable();
    return d_returnType;
}

const std::string& FunctionBase::templateParameters() const
{
    assertReadable();
    return d_templateParameters;
}

std::string FunctionBase::storageKey() const
{
    assertReadable();
    return getStorageKey(qualifiedName(), d_signature, d_templateParameters, d_returnType);
}

std::string FunctionBase::getStorageKey(const std::string& qualifiedName,
                                        const std::string& signature,
                                        const std::string& templateParameters,
                                        const std::string& returnType)
{
    return qualifiedName + '\n' + signature + '\n' + templateParameters + '\n' + returnType;
}

} // namespace Codethink::lvtmdb
