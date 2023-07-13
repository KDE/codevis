// ct_lvtclp_testsourcefile.t.cpp                                      -*-C++-*-

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

// This file is for tests specific to adding SourceFiles to the database

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>

#include <ct_lvtclp_testutil.h>

#include <catch2/catch.hpp>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("Source file")
{
    // test ClpUtil::writeSourceFile
    const static char *empty = "";
    Codethink::lvtmdb::ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, empty, "test/SourceFile.cpp"));

    FileObject *source = nullptr;
    session.withROLock([&] {
        source = session.getFile("test/SourceFile.cpp");
    });

    REQUIRE(source);
    source->withROLock([&] {
        REQUIRE(source->name() == "SourceFile.cpp");
        REQUIRE(!source->isHeader());
    });
}
