// ct_lvtmdb_fieldobject.cpp                                          -*-C++-*-

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

#include <ct_lvtmdb_fieldobject.h>

namespace Codethink::lvtmdb {

FieldObject::FieldObject(std::string qualifiedName,
                         std::string name,
                         std::string signature,
                         lvtshr::AccessSpecifier access,
                         bool isStatic,
                         TypeObject *parent):
    DatabaseObject(std::move(qualifiedName), std::move(name)),
    d_signature(std::move(signature)),
    d_access(access),
    d_isStatic(isStatic),
    d_parent_p(parent)
{
}

FieldObject::~FieldObject() noexcept = default;

FieldObject::FieldObject(FieldObject&&) noexcept = default;

FieldObject& FieldObject::operator=(FieldObject&&) noexcept = default;

const std::string& FieldObject::signature() const
{
    assertReadable();
    return d_signature;
}

lvtshr::AccessSpecifier FieldObject::access() const
{
    assertReadable();
    return d_access;
}

bool FieldObject::isStatic() const
{
    assertReadable();
    return d_isStatic;
}

TypeObject *FieldObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

const std::vector<TypeObject *>& FieldObject::variableTypes() const
{
    assertReadable();
    return d_variableTypes;
}

void FieldObject::addType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_variableTypes, type);
}

} // namespace Codethink::lvtmdb
