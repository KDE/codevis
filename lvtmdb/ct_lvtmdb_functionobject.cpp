// ct_lvtmdb_functionobject.cpp                                       -*-C++-*-

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

#include <ct_lvtmdb_functionobject.h>

namespace Codethink::lvtmdb {

FunctionObject::FunctionObject(std::string qualifiedName,
                               std::string name,
                               std::string signature,
                               std::string returnType,
                               std::string templateParameters,
                               NamespaceObject *parent):
    FunctionBase(std::move(qualifiedName),
                 std::move(name),
                 std::move(signature),
                 std::move(returnType),
                 std::move(templateParameters)),
    d_parent_p(parent)
{
}

FunctionObject::~FunctionObject() noexcept = default;

FunctionObject::FunctionObject(FunctionObject&&) noexcept = default;

FunctionObject& FunctionObject::operator=(FunctionObject&&) noexcept = default;

NamespaceObject *FunctionObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

const std::vector<FunctionObject *>& FunctionObject::callers() const
{
    assertReadable();
    return d_callers;
}

const std::vector<FunctionObject *>& FunctionObject::callees() const
{
    assertReadable();
    return d_callees;
}

void FunctionObject::addDependency(FunctionObject *source, FunctionObject *target)
{
    assert(source);
    assert(target);
    addPeerRelationship(source, target, source->d_callees, target->d_callers);
}

} // namespace Codethink::lvtmdb
