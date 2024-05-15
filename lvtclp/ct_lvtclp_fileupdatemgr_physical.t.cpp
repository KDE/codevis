// ct_lvtclp_fileupdatemgr_physical.t.cpp                             -*-C++-*-

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

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>

#include <ct_lvtclp_cpp_tool.h>
#include <ct_lvtclp_testutil.h>

#include <filesystem>
#include <initializer_list>
#include <vector>

#include <catch2-local-includes.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

static void createTop(const std::filesystem::path& topLevel)
{
    bool created = std::filesystem::create_directories(topLevel / "groups/one/onetop");
    REQUIRE(created);

    Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.h", R"(
// onetop_top.h

namespace onetop {

class Top {
  public:
    int method();
};

})");

    Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.cpp", R"(
// onetop_top.cpp

#include <onetop_top.h>

namespace onetop {

int Top::method()
{
    return 2;
}

})");
}

static void createDep(const std::filesystem::path& topLevel)
{
    assert(std::filesystem::create_directories(topLevel / "groups/one/onedep"));

    Test_Util::createFile(topLevel / "groups/one/onedep/onedep_dep.h", R"(
// onedep_dep.h

#include <onetop_top.h>

namespace onedep {

class Dep {
    onetop::Top top;

    int method();
};

})");
    Test_Util::createFile(topLevel / "groups/one/onedep/onedep_dep.cpp", R"(
// onedep_dep.cpp

#include <onedep_dep.h>

namespace onedep {

int Dep::method()
{
    return top.method();
}

})");
}

struct PhysicalFileUpdateManagerFixture {
    PhysicalFileUpdateManagerFixture():
        topLevel(std::filesystem::temp_directory_path() / "ct_lvtclp_fileupdatemgr_physical_test")
    {
        if (std::filesystem::exists(topLevel)) {
            REQUIRE(std::filesystem::remove_all(topLevel));
        }

        createTop(topLevel);
        createDep(topLevel);
    }

    ~PhysicalFileUpdateManagerFixture()
    {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    std::filesystem::path topLevel;
};

void checkDatabaseOriginal(ObjectStore& session)
// check the database before any files are removed
{
    const std::initializer_list<ModelUtil::SourceFileModel> files = {{"groups/one/onedep/onedep_dep.cpp",
                                                                      false,
                                                                      "groups/one/onedep",
                                                                      "groups/one/onedep/onedep_dep",
                                                                      {"onedep"},
                                                                      {},
                                                                      {"groups/one/onedep/onedep_dep.h"}},
                                                                     {"groups/one/onedep/onedep_dep.h",
                                                                      true,
                                                                      "groups/one/onedep",
                                                                      "groups/one/onedep/onedep_dep",
                                                                      {"onedep"},
                                                                      {"onedep::Dep"},
                                                                      {"groups/one/onetop/onetop_top.h"}},
                                                                     {"groups/one/onetop/onetop_top.cpp",
                                                                      false,
                                                                      "groups/one/onetop",
                                                                      "groups/one/onetop/onetop_top",
                                                                      {"onetop"},
                                                                      {},
                                                                      {"groups/one/onetop/onetop_top.h"}},
                                                                     {"groups/one/onetop/onetop_top.h",
                                                                      true,
                                                                      "groups/one/onetop",
                                                                      "groups/one/onetop/onetop_top",
                                                                      {"onetop"},
                                                                      {"onetop::Top"},
                                                                      {}}};

    const std::initializer_list<ModelUtil::ComponentModel> comps = {
        {"groups/one/onedep/onedep_dep",
         "onedep_dep",
         {"groups/one/onedep/onedep_dep.h", "groups/one/onedep/onedep_dep.cpp"},
         {"groups/one/onetop/onetop_top"}},
        {"groups/one/onetop/onetop_top",
         "onetop_top",
         {"groups/one/onetop/onetop_top.h", "groups/one/onetop/onetop_top.cpp"},
         {}},
    };

    const std::initializer_list<ModelUtil::PackageModel> pkgs = {
        {"groups/one/onedep", "groups/one", {"onedep::Dep"}, {"groups/one/onedep/onedep_dep"}, {"groups/one/onetop"}},
        {"groups/one/onetop", "groups/one", {"onetop::Top"}, {"groups/one/onetop/onetop_top"}, {}}};

    const std::initializer_list<ModelUtil::UDTModel> udts = {
        {
            "onedep::Dep",
            "onedep",
            "groups/one/onedep",
            {"onetop::Top"},
            {},
            {},
            {"onedep::Dep::method"},
            {"onedep::Dep::top"},
        },
        {
            "onetop::Top",
            "onetop",
            "groups/one/onetop",
            {},
            {},
            {},
            {"onetop::Top::method"},
            {},
        },
    };

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

void checkDatabaseDepDeleted(ObjectStore& session)
// check the database after onedep_dep.h is deleted
{
    const std::initializer_list<ModelUtil::SourceFileModel> files = {
        // onedep_dep.h deleted
        // onedep_dep.cpp deleted because it includes onedep_dep.h
        {"groups/one/onetop/onetop_top.cpp",
         false,
         "groups/one/onetop",
         "groups/one/onetop/onetop_top",
         {"onetop"},
         {},
         {"groups/one/onetop/onetop_top.h"}},
        {"groups/one/onetop/onetop_top.h",
         true,
         "groups/one/onetop",
         "groups/one/onetop/onetop_top",
         {"onetop"},
         {"onetop::Top"},
         {}}};
    const std::initializer_list<ModelUtil::ComponentModel> comps = {
        {"groups/one/onetop/onetop_top",
         "onetop_top",
         {"groups/one/onetop/onetop_top.h", "groups/one/onetop/onetop_top.cpp"},
         {}},
    };
    const std::initializer_list<ModelUtil::PackageModel> pkgs = {
        {"groups/one/onedep",
         "groups/one",
         {/*onedep::Dep deleted*/},
         {/*onedep_dep deleted*/},
         {/*depenency deleted*/}},
        {"groups/one/onetop", "groups/one", {"onetop::Top"}, {"groups/one/onetop/onetop_top"}, {}}};
    const std::initializer_list<ModelUtil::UDTModel> udts = {
        // onedep::Dep deleted
        {
            "onetop::Top",
            "onetop",
            "groups/one/onetop",
            {},
            {},
            {},
            {"onetop::Top::method"},
            {},
        },
    };

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

void checkDatabaseTopDeleted(ObjectStore& session)
// check the database after onetop_top.cpp is deleted
{
    const std::initializer_list<ModelUtil::SourceFileModel> files = {{"groups/one/onedep/onedep_dep.cpp",
                                                                      false,
                                                                      "groups/one/onedep",
                                                                      "groups/one/onedep/onedep_dep",
                                                                      {"onedep"},
                                                                      {},
                                                                      {"groups/one/onedep/onedep_dep.h"}},
                                                                     {"groups/one/onedep/onedep_dep.h",
                                                                      true,
                                                                      "groups/one/onedep",
                                                                      "groups/one/onedep/onedep_dep",
                                                                      {"onedep"},
                                                                      {"onedep::Dep"},
                                                                      {"groups/one/onetop/onetop_top.h"}},
                                                                     //      onetop_top.cpp deleted
                                                                     {"groups/one/onetop/onetop_top.h",
                                                                      true,
                                                                      "groups/one/onetop",
                                                                      "groups/one/onetop/onetop_top",
                                                                      {"onetop"},
                                                                      {"onetop::Top"},
                                                                      {}}};

    const std::initializer_list<ModelUtil::ComponentModel> comps = {
        {"groups/one/onedep/onedep_dep",
         "onedep_dep",
         {"groups/one/onedep/onedep_dep.h", "groups/one/onedep/onedep_dep.cpp"},
         {"groups/one/onetop/onetop_top"}},
        {"groups/one/onetop/onetop_top", "onetop_top", {"groups/one/onetop/onetop_top.h"}, {}},
    };

    const std::initializer_list<ModelUtil::PackageModel> pkgs = {
        {"groups/one/onedep", "groups/one", {"onedep::Dep"}, {"groups/one/onedep/onedep_dep"}, {"groups/one/onetop"}},
        {"groups/one/onetop", "groups/one", {"onetop::Top"}, {"groups/one/onetop/onetop_top"}, {}}};

    const std::initializer_list<ModelUtil::UDTModel> udts = {
        {
            "onedep::Dep",
            "onedep",
            "groups/one/onedep",
            {"onetop::Top"},
            {},
            {},
            {"onedep::Dep::method"},
            {"onedep::Dep::top"},
        },
        {
            "onetop::Top",
            "onetop",
            "groups/one/onetop",
            {},
            {},
            {},
            {"onetop::Top::method"},
            {},
        },
    };

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

TEST_CASE_METHOD(PhysicalFileUpdateManagerFixture, "File update manager physical")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/one/onedep/onedep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/one/onedep"},
                                   topLevel);

    INFO("Using top level tmp path = '" << topLevel.string() << "'");
    CppTool tool(topLevel, {}, cmds, ":memory:");
    REQUIRE(tool.runFull());

    ObjectStore& session = tool.getObjectStore();

    // remove onedep_dep.h from the database and check the right things change
    {
        FileObject *depH = nullptr;
        session.withROLock([&] {
            depH = session.getFile("groups/one/onedep/onedep_dep.h");
        });
        REQUIRE(depH);
        session.withRWLock([&] {
            auto removedFiles = std::set<intptr_t>{};
            session.removeFile(depH, removedFiles);
        });
        checkDatabaseDepDeleted(session);
    }

    // reparse the files removed by the fileUpdateMgr and check the database
    // is restored
    {
        session.withRWLock([&] {
            session.clear();
        });
        REQUIRE(tool.runFull());
        checkDatabaseOriginal(session);
    }

    // remove onedep_top.cpp from the database and check the right things change
    {
        FileObject *top = nullptr;
        session.withROLock([&] {
            top = session.getFile("groups/one/onetop/onetop_top.cpp");
        });
        REQUIRE(top);
        session.withRWLock([&] {
            auto removedFiles = std::set<intptr_t>{};
            session.removeFile(top, removedFiles);
        });
        checkDatabaseTopDeleted(session);
    }
}
