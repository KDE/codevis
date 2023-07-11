// ct_lvtmdb_variableobject.cpp                                          -*-C++-*-

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

#include <ct_lvtmdb_variableobject.h>

namespace Codethink::lvtmdb {

VariableObject::VariableObject(
    std::string qualifiedName, std::string name, std::string signature, bool isGlobal, NamespaceObject *parent):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_signature(std::move(signature)),
    d_isGlobal(isGlobal),
    d_parent_p(parent)
{
}

VariableObject::~VariableObject() noexcept = default;

VariableObject::VariableObject(VariableObject&&) noexcept = default;

VariableObject& VariableObject::operator=(VariableObject&&) noexcept = default;

const std::string& VariableObject::signature() const
{
    assertReadable();
    return d_signature;
}

bool VariableObject::isGlobal() const
{
    assertReadable();
    return d_isGlobal;
}

NamespaceObject *VariableObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

} // namespace Codethink::lvtmdb
