// ct_lvtmdb_errorobject.cpp                                          -*-C++-*-

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

#include <ct_lvtmdb_errorobject.h>

namespace Codethink::lvtmdb {

ErrorObject::ErrorObject(MdbUtil::ErrorKind errorKind,
                         std::string qualifiedName,
                         std::string errorMessage,
                         std::string fileName):
    DatabaseObject(std::move(qualifiedName), {}),
    d_errorKind(errorKind),
    d_errorMessage(std::move(errorMessage)),
    d_fileName(std::move(fileName))
{
}

ErrorObject::~ErrorObject() noexcept = default;

ErrorObject::ErrorObject(ErrorObject&&) noexcept = default;

ErrorObject& ErrorObject::operator=(ErrorObject&&) noexcept = default;

MdbUtil::ErrorKind ErrorObject::errorKind() const
{
    return d_errorKind;
}

const std::string& ErrorObject::errorMessage() const
{
    assertReadable();
    return d_errorMessage;
}

const std::string& ErrorObject::fileName() const
{
    assertReadable();
    return d_fileName;
}

std::string ErrorObject::storageKey() const
{
    assertReadable();
    return getStorageKey(qualifiedName(), d_errorMessage, d_fileName);
}

std::string ErrorObject::getStorageKey(const std::string& qualifiedName,
                                       const std::string& errorMessage,
                                       const std::string& fileName)
{
    return qualifiedName + '\n' + errorMessage + '\n' + fileName;
}

} // namespace Codethink::lvtmdb
