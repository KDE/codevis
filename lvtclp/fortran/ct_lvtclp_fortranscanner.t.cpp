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

#include <catch2-local-includes.h>
#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <fortran/ct_lvtclp_tool.h>
#include <test-project-paths.h>

TEST_CASE("simple fortran project")
{
    using namespace Codethink::lvtclp;
    using namespace Codethink::lvtmdb;

    auto const PREFIX = std::string{TEST_PRJ_PATH};
    auto fileList = std::vector<std::filesystem::path>{{PREFIX + "/fortran_basics/a.f"}};
    auto tool = fortran::Tool{fileList};
    tool.runFull();

    auto locks = std::vector<Lockable::ROLock>{};
    auto l = [&](Lockable::ROLock&& lock) {
        locks.emplace_back(std::move(lock));
    };
    auto& memDb = tool.getObjectStore();
    l(memDb.readOnlyLock());

    // Although only one file is being parsed, we do get information from the others, but
    // don't save their files. Only parsed files are currently added to the database. This
    // may be changed in the future, but for now, this is the expected behavior.
    REQUIRE(memDb.getAllFiles().size() == 1);

    auto *componentA = memDb.getComponent("a");
    REQUIRE(componentA);
    l(componentA->readOnlyLock());

    auto *componentB = memDb.getComponent("b");
    REQUIRE(componentB);
    l(componentB->readOnlyLock());

    auto *package = componentA->package();
    REQUIRE(package);
    l(package->readOnlyLock());

    REQUIRE(componentA->name() == "a");
    REQUIRE(componentB->name() == "b");
    REQUIRE(package->name() == "Fortran");

    auto *funcCal1 = memDb.getFunction(
        /*qualifiedName=*/"cal1",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal1);

    auto *funcCal2 = memDb.getFunction(
        /*qualifiedName=*/"cal2",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal2);

    auto *funcCal3 = memDb.getFunction(
        /*qualifiedName=*/"cal3",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal3);

    auto *funcCalF = memDb.getFunction(
        /*qualifiedName=*/"cal_f",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCalF);

    REQUIRE(funcCal1->callees().size() == 3);
    REQUIRE(funcCal1->callers().size() == 0);

    REQUIRE(funcCal2->callees().size() == 0);
    REQUIRE(funcCal2->callers().size() == 1);

    REQUIRE(funcCal3->callees().size() == 0);
    REQUIRE(funcCal3->callers().size() == 0);

    REQUIRE(funcCalF->callees().size() == 0);
    REQUIRE(funcCalF->callers().size() == 1);
}

TEST_CASE("Mixed fortran and C project")
{
    using namespace Codethink::lvtclp;
    using namespace Codethink::lvtmdb;
    auto const PREFIX = std::string{TEST_PRJ_PATH};

    auto fileList = std::vector<std::filesystem::path>{{PREFIX + "/fortran_c_mixed/a.f"},
                                                       {PREFIX + "/fortran_c_mixed/b.f"},
                                                       {PREFIX + "/fortran_c_mixed/c.c"},
                                                       {PREFIX + "/fortran_c_mixed/main.c"}};
    auto tool = fortran::Tool{fileList};
    tool.runFull();

    auto locks = std::vector<Lockable::ROLock>{};
    auto l = [&](Lockable::ROLock&& lock) {
        locks.emplace_back(std::move(lock));
    };
    auto& memDb = tool.getObjectStore();
    l(memDb.readOnlyLock());

    // '.c' files are ignored
    REQUIRE(memDb.getAllFiles().size() == 2);
}
