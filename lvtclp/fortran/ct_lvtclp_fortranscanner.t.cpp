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
#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_tool.h>
#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <fortran/ct_lvtclp_fortran_c_interop.h>
#include <fortran/ct_lvtclp_tool.h>
#include <test-project-paths.h>

#include <memory>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

TEST_CASE("Simple fortran project")
{
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
    REQUIRE(package->name() == "fortran_basics");

    auto *funcCal1 = memDb.getFunction(
        /*qualifiedName=*/"cal1",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal1);
    l(funcCal1->readOnlyLock());

    auto *funcCal2 = memDb.getFunction(
        /*qualifiedName=*/"cal2",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal2);
    l(funcCal2->readOnlyLock());

    auto *funcCal3 = memDb.getFunction(
        /*qualifiedName=*/"cal3",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCal3);
    l(funcCal3->readOnlyLock());

    auto *funcCalF = memDb.getFunction(
        /*qualifiedName=*/"cal_f",
        /*signature=*/"",
        /*templateParameters=*/"",
        /*returnType=*/"");
    REQUIRE(funcCalF);
    l(funcCalF->readOnlyLock());

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
    auto const PREFIX = std::string{TEST_PRJ_PATH} + "/fortran_c_mixed";
    auto sharedMemDb = std::make_shared<ObjectStore>();

    auto fileList = std::vector<std::filesystem::path>{{PREFIX + "/mixedprj/a.f"},
                                                       {PREFIX + "/mixedprj/b.f"},
                                                       // C files will be ignored by Fortran parser.
                                                       {PREFIX + "/mixedprj/c.c"},
                                                       {PREFIX + "/mixedprj/main.c"}};
    auto fortranTool = fortran::Tool{fileList};
    fortranTool.setSharedMemDb(sharedMemDb);

    auto staticCompilationDb =
        StaticCompilationDatabase{{{PREFIX + "/mixedprj/c.c", "c.o"}, {PREFIX + "/mixedprj/main.c", "main.o"}},
                                  "placeholder",
                                  {"-I" + PREFIX + "/mixedprj/", "-std=c++17"},
                                  PREFIX};
    auto cTool = Tool(
        /*sourcePath=*/PREFIX,
        /*db=*/staticCompilationDb,
        /*databasePath=*/"unused");
    cTool.setSharedMemDb(sharedMemDb);

    REQUIRE(cTool.runFull());
    REQUIRE(fortranTool.runFull());

    auto getAllFunctionsForComponent = [](auto *component) {
        auto allFuncs = std::map<std::string, FunctionObject *>{};
        for (auto *file : component->files()) {
            auto _fileLock = file->readOnlyLock();
            for (auto *func : file->globalFunctions()) {
                auto _funcLock = func->readOnlyLock();
                allFuncs[func->qualifiedName()] = func;
            }
        }
        return allFuncs;
    };

    {
        auto locks = std::vector<Lockable::ROLock>{};
        auto l = [&](Lockable::ROLock&& lock) {
            locks.emplace_back(std::move(lock));
        };
        l(sharedMemDb->readOnlyLock());

        // All fortran files + C files
        REQUIRE(sharedMemDb->getAllFiles().size() == 5);

        auto cComponent = sharedMemDb->getComponent("mixedprj/c");
        REQUIRE(cComponent);
        l(cComponent->readOnlyLock());
        auto cFuncs = getAllFunctionsForComponent(cComponent);
        REQUIRE(cFuncs.size() == 2);
        REQUIRE(cFuncs.at("cal_c"));
        // c_func_ has the "_" suffix AND it is defined within C code.
        REQUIRE(cFuncs.at("c_func_"));

        auto otherComponent = sharedMemDb->getComponent("mixedprj/other");
        REQUIRE(otherComponent);
        l(otherComponent->readOnlyLock());
        auto otherFuncs = getAllFunctionsForComponent(otherComponent);

        // NOTE: 'other_func' is a function declared in 'other.h', but it is never defined.
        //       'cal1_' is a also never defined, but since it has the '_' suffix, it is assumed to
        //       be defined in Fortran, thus it is persisted on the 'other' component.
        REQUIRE(otherFuncs.size() == 1);
        REQUIRE(otherFuncs.at("cal1_"));
        l(otherFuncs.at("cal1_")->readOnlyLock());

        // Before Fortran <-> C interop solver run, there should be 0 callees.
        REQUIRE(otherFuncs.at("cal1_")->callees().empty());
    }

    Codethink::lvtclp::fortran::solveFortranToCInteropDeps(*sharedMemDb);

    {
        auto locks = std::vector<Lockable::ROLock>{};
        auto l = [&](Lockable::ROLock&& lock) {
            locks.emplace_back(std::move(lock));
        };
        l(sharedMemDb->readOnlyLock());

        auto otherComponent = sharedMemDb->getComponent("mixedprj/other");
        REQUIRE(otherComponent);
        l(otherComponent->readOnlyLock());
        auto otherFuncs = getAllFunctionsForComponent(otherComponent);
        REQUIRE(otherFuncs.size() == 1);
        REQUIRE(otherFuncs.at("cal1_"));
        l(otherFuncs.at("cal1_")->readOnlyLock());

        // After Fortran <-> C interop solver run, the fortran dependency is resolved,
        // so there is one callee dependency
        REQUIRE(otherFuncs.at("cal1_")->callees().size() == 1);
        auto *fortranCal1 = otherFuncs.at("cal1_")->callees()[0];
        l(fortranCal1->readOnlyLock());
        REQUIRE(fortranCal1->qualifiedName() == "cal1");
    }
}
