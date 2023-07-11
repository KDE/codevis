// ct_lvtmdb_methodobject.cpp                                         -*-C++-*-

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

#include <ct_lvtmdb_methodobject.h>

#include <ct_lvtmdb_util.h>

namespace Codethink::lvtmdb {

MethodObject::MethodObject(std::string qualifiedName,
                           std::string name,
                           std::string signature,
                           std::string returnType,
                           std::string templateParameters,
                           lvtshr::AccessSpecifier access,
                           bool isVirtual,
                           bool isPure,
                           bool isStatic,
                           bool isConst,
                           TypeObject *parent):
    FunctionBase(std::move(qualifiedName),
                 std::move(name),
                 std::move(signature),
                 std::move(returnType),
                 std::move(templateParameters)),
    d_access(access),
    d_isVirtual(isVirtual),
    d_isPure(isPure),
    d_isStatic(isStatic),
    d_isConst(isConst),
    d_parent_p(parent)
{
}

MethodObject::~MethodObject() noexcept = default;

MethodObject::MethodObject(MethodObject&&) noexcept = default;

MethodObject& MethodObject::operator=(MethodObject&&) noexcept = default;

lvtshr::AccessSpecifier MethodObject::access() const
{
    assertReadable();
    return d_access;
}

bool MethodObject::isVirtual() const
{
    assertReadable();
    return d_isVirtual;
}

bool MethodObject::isPure() const
{
    assertReadable();
    return d_isPure;
}

bool MethodObject::isStatic() const
{
    assertReadable();
    return d_isStatic;
}

bool MethodObject::isConst() const
{
    assertReadable();
    return d_isConst;
}

TypeObject *MethodObject::parent() const
{
    assertReadable();
    return d_parent_p;
}

const std::vector<TypeObject *>& MethodObject::argumentTypes() const
{
    assertReadable();
    return d_argumentTypes;
}

void MethodObject::addArgumentType(TypeObject *type)
{
    assertWritable();
    MdbUtil::pushBackUnique(d_argumentTypes, type);
}

} // namespace Codethink::lvtmdb
