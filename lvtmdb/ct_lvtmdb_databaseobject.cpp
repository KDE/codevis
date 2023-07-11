// ct_lvtmdb_databaseobject.cpp                                       -*-C++-*-

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

#include <ct_lvtmdb_databaseobject.h>

namespace Codethink::lvtmdb {

DatabaseObject::DatabaseObject(std::string qualifiedName, std::string name):
    d_qualifiedName(std::move(qualifiedName)), d_name(std::move(name))
{
}

DatabaseObject::~DatabaseObject() noexcept = default;

DatabaseObject::DatabaseObject(DatabaseObject&&) noexcept = default;

DatabaseObject& DatabaseObject::operator=(DatabaseObject&&) noexcept = default;

#ifdef LVTMDB_LOCK_DEBUGGING
const std::string& DatabaseObject::debugName() const
{
    // skip locking assert so we can use this to debug locking
    // this is safe because the qualified name isn't modified after
    // construction
    return d_qualifiedName;
}
#endif

const std::string& DatabaseObject::qualifiedName() const
{
    assertReadable();
    return d_qualifiedName;
}

const std::string& DatabaseObject::name() const
{
    assertReadable();
    return d_name;
}

} // namespace Codethink::lvtmdb
