// ct_lvtmdb_lockable.cpp                                             -*-C++-*-

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

#include <ct_lvtmdb_lockable.h>

#include <cassert>
#include <iostream>

namespace Codethink::lvtmdb {

std::pair<Lockable::ROLock, Lockable::ROLock> Lockable::roLockTwo(Lockable *a, Lockable *b)
{
    assert(a);
    assert(b);
    if (a == b) {
        std::cerr << "lvtmdb::Lockable::lockTwo with identical arguments" << std::endl;
        // we can't return two distinct locks to one object: that would be a
        // deadlock
        std::abort();
    }

    // order by memory address
    if (a < b) {
        return {a->readOnlyLock(), b->readOnlyLock()};
    }
    return {b->readOnlyLock(), a->readOnlyLock()};
}

std::pair<Lockable::RWLock, Lockable::RWLock> Lockable::rwLockTwo(Lockable *a, Lockable *b)
{
    assert(a);
    assert(b);
    if (a == b) {
        std::cerr << "lvtmdb::Lockable::lockTwo with identical arguments" << std::endl;
        // we can't return two distinct locks to one object: that would be a
        // deadlock
        std::abort();
    }

    // order by memory address
    if (a < b) {
        return {a->rwLock(), b->rwLock()};
    }
    return {b->rwLock(), a->rwLock()};
}

} // namespace Codethink::lvtmdb
