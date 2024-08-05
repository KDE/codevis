// ct_lvtclp_tool.t.cpp                                               -*-C++-*-

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

#include <ct_lvtclp_cpp_tool.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvttst_tmpdir.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>

#include <QtGlobal>

#include <catch2-local-includes.h>

#include <autogen-test-variables.h>
#include <test-project-paths.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

static void createTop(const std::filesystem::path& topLevel)
{
    bool created = std::filesystem::create_directories(topLevel / "groups/one/onetop");
    REQUIRE(created);

    created = Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.h", R"(
// onetop_top.h

namespace onetop {

class Base
{
};

class Top : public Base {
  public:
    int method();
};

})");
    REQUIRE(created);

    created = Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.cpp", R"(
// onetop_top.cpp

#include <onetop_top.h>

namespace onetop {

int Top::method()
{
    return 2;
}

})");

    REQUIRE(created);
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

static void createTestEnv(const std::filesystem::path& topLevel)
{
    if (std::filesystem::exists(topLevel / "groups")) {
        REQUIRE(std::filesystem::remove_all(topLevel / "groups"));
    }

    createTop(topLevel);
    createDep(topLevel);
}

namespace DatabaseOriginal {
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
                                                                  {"onetop::Base", "onetop::Top"},
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
    {"groups/one/onetop", "groups/one", {"onetop::Base", "onetop::Top"}, {"groups/one/onetop/onetop_top"}, {}}};

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
        "onetop::Base",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {},
        {},
        {},
    },
    {
        "onetop::Top",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {"onetop::Base"},
        {"onetop::Top::method"},
        {},
    },
};
}; // namespace DatabaseOriginal

namespace DatabaseModifiedFile {
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
                                                                 // NewClass added:
                                                                 {"groups/one/onetop/onetop_top.h",
                                                                  true,
                                                                  "groups/one/onetop",
                                                                  "groups/one/onetop/onetop_top",
                                                                  {"onetop"},
                                                                  {"onetop::Base", "onetop::Top", "onetop::NewClass"},
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
    {"groups/one/onetop",
     "groups/one",
     {"onetop::Base", "onetop::Top", "onetop::NewClass"},
     {"groups/one/onetop/onetop_top"},
     {}}};

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
        "onetop::Base",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {},
        {},
        {},
    },
    {
        "onetop::Top",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {"onetop::Base"},
        {"onetop::Top::method"},
        {},
    },
    {
        "onetop::NewClass",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {},
        {},
        {},
    },
};
}; // namespace DatabaseModifiedFile

namespace DatabaseNewFile {
// check the database after onedep_newfile has been added
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
                                                                 {"groups/one/onedep/onedep_newfile.cpp",
                                                                  false,
                                                                  "groups/one/onedep",
                                                                  "groups/one/onedep/onedep_newfile",
                                                                  {},
                                                                  {},
                                                                  {"groups/one/onedep/onedep_newfile.h"}},
                                                                 {"groups/one/onedep/onedep_newfile.h",
                                                                  true,
                                                                  "groups/one/onedep",
                                                                  "groups/one/onedep/onedep_newfile",
                                                                  {"onedep"},
                                                                  {"onedep::NewFile"},
                                                                  {}},
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
                                                                  {"onetop::Base", "onetop::Top"},
                                                                  {}}};

const std::initializer_list<ModelUtil::ComponentModel> comps = {
    {"groups/one/onedep/onedep_dep",
     "onedep_dep",
     {"groups/one/onedep/onedep_dep.h", "groups/one/onedep/onedep_dep.cpp"},
     {"groups/one/onetop/onetop_top"}},
    {"groups/one/onedep/onedep_newfile",
     "onedep_newfile",
     {"groups/one/onedep/onedep_newfile.h", "groups/one/onedep/onedep_newfile.cpp"},
     {}},
    {"groups/one/onetop/onetop_top",
     "onetop_top",
     {"groups/one/onetop/onetop_top.h", "groups/one/onetop/onetop_top.cpp"},
     {}},
};

const std::initializer_list<ModelUtil::PackageModel> pkgs = {
    {"groups/one/onedep",
     "groups/one",
     {"onedep::Dep", "onedep::NewFile"},
     {"groups/one/onedep/onedep_dep", "groups/one/onedep/onedep_newfile"},
     {"groups/one/onetop"}},
    {"groups/one/onetop", "groups/one", {"onetop::Base", "onetop::Top"}, {"groups/one/onetop/onetop_top"}, {}}};

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
        "onedep::NewFile",
        "onedep",
        "groups/one/onedep",
        {},
        {},
        {},
        {},
        {},
    },
    {
        "onetop::Base",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {},
        {},
        {},
    },
    {
        "onetop::Top",
        "onetop",
        "groups/one/onetop",
        {},
        {},
        {"onetop::Base"},
        {"onetop::Top::method"},
        {},
    },
};
}; // namespace DatabaseNewFile

static void runTool(CppTool& tool, const bool madeChanges, const unsigned numRuns = 1)
{
    for (unsigned i = 0; i < numRuns; i++) {
        REQUIRE(tool.runFull());
        REQUIRE(tool.lastRunMadeChanges() == madeChanges);
    }
}

struct CppToolTestFixture {
    CppToolTestFixture():
        topLevel(std::filesystem::weakly_canonical(std::filesystem::temp_directory_path() / "ct_lvtclp_tool_test")
                     .generic_string())
    {
        createTestEnv(topLevel);
    }

    ~CppToolTestFixture()
    {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    std::filesystem::path topLevel;
};

TEST_CASE_METHOD(CppToolTestFixture, "Tool")
{
    constexpr unsigned NUM_RERUNS = 2;

    {
        StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                        {"groups/one/onedep/onedep_dep.cpp", "build/onedep_dep.o"}},
                                       "placeholder",
                                       {"-Igroups/one/onetop", "-Igroups/one/onedep"},
                                       topLevel);

        const CppToolConstants constants{
            .prefix = topLevel,
            .buildPath = std::filesystem::weakly_canonical(std::filesystem::current_path()).generic_string(),
            .databasePath = (topLevel / "database").generic_string(),
            .nonLakosianDirs = {},
            .thirdPartyDirs = {},
            .ignoreGlobs = {},
            .userProvidedExtraCompileCommandsArgs = {},
            .numThreads = 1,
            .enableLakosianRules = true,
            .printToConsole = false};

        CppTool tool(constants, cmds);
        ObjectStore& session = tool.getObjectStore();

        // initial parse
        runTool(tool, true);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));

        // re-run incrementally
        runTool(tool, false, NUM_RERUNS);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));

        // modify a file, adding a class
        std::filesystem::path fileToChange = topLevel / "groups/one/onetop/onetop_top.h";
        {
            // open for appending
            std::ofstream of(fileToChange.string(), std::ios_base::out | std::ios_base::app);
            REQUIRE(!of.fail());
            of << "namespace onetop { class NewClass {}; }" << std::endl;
        }
        runTool(tool, true);
        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseModifiedFile::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseModifiedFile::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseModifiedFile::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseModifiedFile::udts));

        // re-run incrementally
        runTool(tool, false, NUM_RERUNS);
        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseModifiedFile::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseModifiedFile::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseModifiedFile::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseModifiedFile::udts));

        // change modified file back again, run again, and re-run
        createTestEnv(topLevel);
        runTool(tool, true);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));

        runTool(tool, false, NUM_RERUNS);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));
    }

    // add a new component (already in the above compilation database)
    {
        StaticCompilationDatabase cmdsPlus(
            // cmds plus the new component
            {{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
             {"groups/one/onedep/onedep_dep.cpp", "build/onedep_dep.o"},
             {"groups/one/onedep/onedep_newfile.cpp", "build/onedep_newfile.o"}},
            "placeholder",
            {"-Igroups/one/onetop", "-Igroups/one/onedep"},
            topLevel.string());

        const CppToolConstants constants{
            .prefix = topLevel,
            .buildPath = std::filesystem::weakly_canonical(std::filesystem::current_path()).generic_string(),
            .databasePath = (topLevel / "database").generic_string(),
            .nonLakosianDirs = {},
            .thirdPartyDirs = {},
            .ignoreGlobs = {},
            .userProvidedExtraCompileCommandsArgs = {},
            .numThreads = 1,
            .enableLakosianRules = true,
            .printToConsole = false};

        CppTool toolPlus(constants, cmdsPlus);

        Test_Util::createFile(topLevel / "groups/one/onedep/onedep_newfile.h", R"(
    // onedep_newfile.h
    namespace onedep {
    class NewFile {};
    }
    )");
        Test_Util::createFile(topLevel / "groups/one/onedep/onedep_newfile.cpp", R"(
    // onedep_newfile.cpp
#include <onedep_newfile.h>
    )");
        ObjectStore& session = toolPlus.getObjectStore();
        runTool(toolPlus, true);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseNewFile::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseNewFile::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseNewFile::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseNewFile::udts));

        runTool(toolPlus, false, NUM_RERUNS);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseNewFile::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseNewFile::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseNewFile::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseNewFile::udts));

        // delete the new component and re-run
        createTestEnv(topLevel);
        runTool(toolPlus, true);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));

        runTool(toolPlus, false, NUM_RERUNS);

        REQUIRE(ModelUtil::checkSourceFiles(session, DatabaseOriginal::files));
        REQUIRE(ModelUtil::checkComponents(session, DatabaseOriginal::comps));
        REQUIRE(ModelUtil::checkPackages(session, DatabaseOriginal::pkgs));
        REQUIRE(ModelUtil::checkUDTs(session, DatabaseOriginal::udts));
    }
}

TEST_CASE("Run Tool on example project")
{
    std::string projectPrefix = std::filesystem::weakly_canonical(PREFIX + "/hello_world/").generic_string();
    StaticCompilationDatabase cmds({{"hello.m.cpp", "hello.m.o"}}, "placeholder", {}, projectPrefix);

    const CppToolConstants constants{.prefix = projectPrefix,
                                     .buildPath = std::filesystem::current_path(),
                                     .databasePath = (projectPrefix + "/database"),
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);

    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());

    FileObject *file = nullptr;
    session.withROLock([&] {
        file = session.getFile("hello.m.cpp");
    });
    REQUIRE(file);
}

TEST_CASE("Run Tool on example project incrementally")
{
    auto sourcePath = std::filesystem::weakly_canonical(PREFIX + "/package_circular_dependency/").generic_string();

    auto files = std::initializer_list<std::pair<std::string, std::string>>{
        {"groups/one/onepkg/ct_onepkg_circle.cpp", "ct_onepkg_circle.o"},
        {"groups/one/onepkg/ct_onepkg_thing.cpp", "ct_onepkg_thing.o"}};

    StaticCompilationDatabase cmds(
        files,
        "placeholder",
        {"-I" + sourcePath + "/groups/one/onepkg/", "-I" + sourcePath + "/groups/two/twodmo/"},
        sourcePath);

    const CppToolConstants constants{.prefix = sourcePath,
                                     .buildPath = std::filesystem::current_path().generic_string(),
                                     .databasePath = sourcePath + "database",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);
    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runPhysical());
    session.withROLock([&] {
        REQUIRE(session.getAllFiles().size() == 5);
        REQUIRE(session.getAllPackages().size() == 2);
        auto *e = session.getPackage("groups/one/onepkg");
        e->withROLock([&]() {
            REQUIRE(e->components().size() == 2);
            for (auto&& c : e->components()) {
                c->withROLock([&]() {
                    if (c->qualifiedName() == "groups/one/onepkg/ct_onepkg_circle") {
                        REQUIRE(c->forwardDependencies().size() == 2);
                    } else if (c->qualifiedName() == "groups/one/onepkg/ct_onepkg_thing") {
                        REQUIRE(c->forwardDependencies().empty());
                    } else {
                        FAIL("Unexpected component: " + c->qualifiedName());
                    }
                });
            }
        });
    });

    // There was a bug where a logical parse after a only-physical parse would remove physical connections from
    // components. This test has been added to avoid regression.
    REQUIRE(tool.runFull(true));
    session.withROLock([&] {
        REQUIRE(session.getAllFiles().size() == 5);
        REQUIRE(session.getAllPackages().size() == 2);
        auto *e = session.getPackage("groups/one/onepkg");
        e->withROLock([&]() {
            REQUIRE(e->components().size() == 2);
            for (auto&& c : e->components()) {
                c->withROLock([&]() {
                    if (c->qualifiedName() == "groups/one/onepkg/ct_onepkg_circle") {
                        REQUIRE(c->forwardDependencies().size() == 2);
                    } else if (c->qualifiedName() == "groups/one/onepkg/ct_onepkg_thing") {
                        REQUIRE(c->forwardDependencies().empty());
                    } else {
                        FAIL("Unexpected component: " + c->qualifiedName());
                    }
                });
            }
        });
    });
}

TEST_CASE("Run Tool on project including other lakosian project")
{
    auto const prjAPath =
        std::filesystem::weakly_canonical(PREFIX + "/project_with_includes_outside_src/prjA").generic_string();
    auto const prjBPath =
        std::filesystem::weakly_canonical(PREFIX + "/project_with_includes_outside_src/prjB").generic_string();

    StaticCompilationDatabase cmds({{"groups/one/oneaaa/oneaaa_comp.cpp", "oneaaa_comp.o"}},
                                   "placeholder",
                                   {"-I" + prjAPath + "/groups/one/oneaaa/", "-I" + prjBPath + "/groups/two/twoaaa/"},
                                   prjAPath);

    const CppToolConstants constants{.prefix = prjAPath,
                                     .buildPath = std::filesystem::current_path().generic_string(),
                                     .databasePath = prjAPath + "/database",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);
    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());
    session.withROLock([&] {
        std::cout << "All Files" << std::endl;
        for (auto& file : session.getAllFiles()) {
            auto lock = file->readOnlyLock();
            std::cout << file->qualifiedName() << std::endl;
        }

        std::cout << "\n All Components" << std::endl;
        for (const auto& [_, comp] : session.components()) {
            std::ignore = _;
            auto lock = comp->readOnlyLock();
            std::cout << comp->qualifiedName() << std::endl;
        }

        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_comp.cpp"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_comp.h"));
        REQUIRE(session.getComponent("groups/one/oneaaa/oneaaa_comp"));
        REQUIRE(session.getComponent("groups/two/twoaaa/twoaaa_comp"));
    });
}

struct HashFunc {
    size_t operator()(const std::tuple<std::string, unsigned>& data) const
    {
        return std::hash<std::string>{}(std::get<0>(data)) ^ std::hash<unsigned>{}(std::get<1>(data));
    }
};

TEST_CASE("Run Tool store test-only dependencies")
{
    auto const prjAPath = std::filesystem::weakly_canonical(PREFIX + "/test_only_dependencies/prjA").generic_string();
    StaticCompilationDatabase cmds(
        {
            {"groups/one/oneaaa/oneaaa_comp.cpp", "oneaaa_comp.o"},
            {"groups/one/oneaaa/oneaaa_comp.t.cpp", "oneaaa_comp.t.o"},
            {"groups/one/oneaaa/oneaaa_othercomp.cpp", "oneaaa_othercomp.o"},
            {"groups/one/oneaaa/oneaaa_othercomp2.cpp", "oneaaa_othercomp2.o"},
        },
        "placeholder",
        {"-I" + prjAPath + "/groups/one/oneaaa/", "-fparse-all-comments"},
        prjAPath);

    const CppToolConstants constants{.prefix = prjAPath,
                                     .buildPath = std::filesystem::current_path().generic_string(),
                                     .databasePath = prjAPath + "/database",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);

    using Filename = std::string;
    using LineNo = unsigned;
    using FileAndLine = std::tuple<Filename, LineNo>;
    auto includesInFile = std::unordered_map<Filename, std::unordered_set<FileAndLine, HashFunc>>{};
    tool.setHeaderLocationCallback(
        [&includesInFile](Filename const& sourceFile, Filename const& includedFile, LineNo lineNo) {
            if (includesInFile.find(sourceFile) == includesInFile.end()) {
                includesInFile[sourceFile] = std::unordered_set<FileAndLine, HashFunc>{};
            }
            includesInFile[sourceFile].insert(std::make_tuple(includedFile, lineNo));
        });

    auto testOnlyDepComments = std::unordered_map<Filename, std::unordered_set<LineNo>>{};
    tool.setHandleCppCommentsCallback([&testOnlyDepComments](Filename const& filename,
                                                             std::string const& briefText,
                                                             LineNo startLine,
                                                             LineNo endLine) {
        if (briefText != "Test only dependency") {
            return;
        }

        if (testOnlyDepComments.find(filename) == testOnlyDepComments.end()) {
            testOnlyDepComments[filename] = std::unordered_set<LineNo>{};
        }
        testOnlyDepComments[filename].insert(startLine);
    });

    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());
    session.withROLock([&] {
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_comp.cpp"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_comp.h"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_othercomp.cpp"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_othercomp.h"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_othercomp2.cpp"));
        REQUIRE(session.getFile("groups/one/oneaaa/oneaaa_othercomp2.h"));

        REQUIRE(session.getComponent("groups/one/oneaaa/oneaaa_comp"));
        REQUIRE(session.getComponent("groups/one/oneaaa/oneaaa_comp.t"));
        REQUIRE(session.getComponent("groups/one/oneaaa/oneaaa_othercomp"));
        REQUIRE(session.getComponent("groups/one/oneaaa/oneaaa_othercomp2"));
    });

    auto fileHasIncludeAtLine = [&includesInFile](Filename const& file, Filename const& included_file, LineNo line) {
        auto& set = includesInFile.at(file);
        return set.find(std::make_tuple(included_file, line)) != set.end();
    };
    REQUIRE(fileHasIncludeAtLine("oneaaa_othercomp2.cpp", "oneaaa_othercomp2.h", 1));
    REQUIRE(fileHasIncludeAtLine("oneaaa_othercomp.cpp", "oneaaa_othercomp.h", 1));
    REQUIRE(fileHasIncludeAtLine("oneaaa_comp.t.cpp", "oneaaa_comp.h", 1));
    REQUIRE(fileHasIncludeAtLine("oneaaa_comp.t.cpp", "oneaaa_othercomp.h", 2));
    REQUIRE(fileHasIncludeAtLine("oneaaa_comp.t.cpp", "oneaaa_othercomp2.h", 3));
    REQUIRE(fileHasIncludeAtLine("oneaaa_comp.cpp", "oneaaa_comp.h", 1));
    REQUIRE(fileHasIncludeAtLine("oneaaa_comp.cpp", "oneaaa_othercomp2.h", 2));

    auto fileHasTestOnlyCommentAtLine = [&testOnlyDepComments](Filename const& file, LineNo line) {
        auto& set = testOnlyDepComments.at(file);
        return set.find(line) != set.end();
    };
    REQUIRE(fileHasTestOnlyCommentAtLine("oneaaa_comp.cpp", 2));
}

TEST_CASE("Test run tool with non-lakosian rules")
{
    auto const PREFIX = std::filesystem::path{TEST_PRJ_PATH};
    auto const prjPath = std::filesystem::canonical((PREFIX / "cpp_nonlakosian_test")).generic_string() + "/";
    std::cout << "Setting up the generic location:\n\t" << prjPath << std::endl;

    auto tmpdir = TmpDir{"cpp_nonlakosian_test_builddir"};
    auto res = tmpdir.createTextFile("compile_commands.json",
    "[\n"
    "{\n"
    "  \"directory\": \"" + prjPath + "\",\n"
    "  \"command\": \"/usr/bin/c++ -I" + prjPath + " -o lib1.cpp.o -c " + prjPath + "mylibs/lib1/lib1.cpp\",\n"
    "  \"file\": \"" + prjPath + "mylibs/lib1/lib1.cpp\",\n"
    "  \"output\": \"CMakeFiles/someprog.dir/mylibs/lib1/lib1.cpp.o\"\n"
    "},\n"
    "{\n"
    "  \"directory\": \"" + prjPath + "\",\n"
    "  \"command\": \"/usr/bin/c++  -I" + prjPath + "  -o lib2.cpp.o -c " + prjPath + "mylibs/lib2/lib2.cpp\",\n"
    "  \"file\": \"" + prjPath + "mylibs/lib2/lib2.cpp\",\n"
    "  \"output\": \"CMakeFiles/someprog.dir/mylibs/lib2/lib2.cpp.o\"\n"
    "},\n"
    "{\n"
    "  \"directory\": \"" + prjPath + "\",\n"
    "  \"command\": \"/usr/bin/c++  -I" + prjPath + "  -o main.cpp.o -c " + prjPath + "main.cpp\",\n"
    "  \"file\": \"" + prjPath + "main.cpp\",\n"
    "  \"output\": \"CMakeFiles/someprog.dir/main.cpp.o\"\n"
    "}\n"
    "]"
    );

    REQUIRE(std::filesystem::exists(res));

    std::cout << "Setting up project path " << prjPath << std::endl;
    std::cout << "And..." << std::filesystem::path(prjPath).generic_string();

    const CppToolConstants constants{.prefix = prjPath,
                                     .buildPath = {},
                                     .databasePath = prjPath + "database/",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {"-iquote" + prjPath + "hidden_folder/"},
                                     .numThreads = 1,
                                     .enableLakosianRules = false,
                                     .printToConsole = false};

    auto tool =
        CppTool(constants, std::vector<std::filesystem::path>{tmpdir.path().string() + "/compile_commands.json"});

    tool.runPhysical();
    ObjectStore& memDb = tool.getObjectStore();

    auto locks = std::vector<Lockable::ROLock>{};
    auto addLock = [&locks](auto *rawDbObj) {
        locks.push_back(rawDbObj->readOnlyLock());
    };

    addLock(&memDb);

    std::cout << "Found Files:" << std::endl;
    for (auto& file : memDb.getAllFiles()) {
        auto lock = file->readOnlyLock();
        std::cout << "{\n\t" << file->qualifiedName() << "\n\t" << file->name() << "\n}" << std::endl;
    }

    // Note: 6 files within the project, and 1 extra file hidden, added with `userProvidedExtraCompileCommandsArgs`
    REQUIRE(memDb.getAllFiles().size() == 7);

    // There's no "non-lakosian" pseudo-package
    auto *rootPkg = memDb.getPackage("cpp_nonlakosian_test");
    auto *myLibsPkg = memDb.getPackage("mylibs");
    auto *lib1Pkg = memDb.getPackage("lib1");
    auto *lib1UtilsPkg = memDb.getPackage("utils");
    auto *lib2Pkg = memDb.getPackage("lib2");
    addLock(rootPkg);
    addLock(myLibsPkg);
    addLock(lib1Pkg);
    addLock(lib1UtilsPkg);
    addLock(lib2Pkg);

    REQUIRE(rootPkg->parent() == nullptr);
    // Non-lakosian parser accept mixed components and packages within a package
    REQUIRE(rootPkg->children().size() == 2);
    REQUIRE(rootPkg->components().size() == 1);
    {
        REQUIRE(myLibsPkg->children().size() == 2);
        REQUIRE(myLibsPkg->components().empty());
        {
            REQUIRE(lib1Pkg->children().size() == 1);
            REQUIRE(lib1Pkg->components().size() == 1);
            // Non-lakosian parser accepts nested packages, beyond pkg groups and pkgs
            {
                REQUIRE(lib1UtilsPkg->children().size() == 0);
                REQUIRE(lib1UtilsPkg->components().size() == 1);
                auto *lib1UtilsComponent = lib1UtilsPkg->components()[0];
                lib1UtilsComponent->withROLock([&]() {
                    REQUIRE(lib1UtilsComponent->qualifiedName() == "lib1_utils");
                });
            }
            auto *lib1Component = lib1Pkg->components()[0];
            lib1Component->withROLock([&]() {
                REQUIRE(lib1Component->qualifiedName() == "lib1");
            });
        }
        {
            REQUIRE(lib2Pkg->children().size() == 0);
            REQUIRE(lib2Pkg->components().size() == 1);
            auto *lib2Component = lib2Pkg->components()[0];
            lib2Component->withROLock([&]() {
                REQUIRE(lib2Component->qualifiedName() == "lib2");
            });
        }
    }
}

TEST_CASE("forward declaration test")
{
    StaticCompilationDatabase cmds({{"main.cpp", "main.o"},
                                    {"ForwardDeclared.cpp", "ForwardDeclared.o"},
                                    {"ClassDefinition.cpp", "ClassDefinition.o"}},
                                   "placeholder",
                                   {},
                                   PREFIX + "/forward_declaration/");

    const CppToolConstants constants{.prefix = PREFIX + "/forward_declaration/",
                                     .buildPath = std::filesystem::current_path(),
                                     .databasePath = PREFIX + "/forward_declaration/database",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);
    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());

    TypeObject *classDeclaration = nullptr;
    session.withROLock([&] {
        classDeclaration = session.getType("ClassDefinition");
    });

    REQUIRE(classDeclaration);

    classDeclaration->withROLock([classDeclaration] {
        auto files = classDeclaration->files();
        REQUIRE(files.size() == 1);

        auto file = files[0];
        file->withROLock([file] {
            REQUIRE(file->name() == "ClassDefinition.h");
        });
    });
}

#ifndef Q_OS_WINDOWS
// cstddef and stddef.h files are a pain to get it right
// because they depend on specific, compile-defined, paths
// the code currently tries to find that to be able to feed
// clang the correct information
// No need to run this on windows, this test Unix specific code
TEST_CASE("cstddef test")
{
    StaticCompilationDatabase cmds({{"hello.m.cpp", "hello.m.o"}}, "placeholder", {}, PREFIX + "/cstddef_test/");
    const CppToolConstants constants{.prefix = PREFIX + "/cstddef_test/",
                                     .buildPath = std::filesystem::current_path(),
                                     .databasePath = PREFIX + "/cstddef_test/database",
                                     .nonLakosianDirs = {},
                                     .thirdPartyDirs = {},
                                     .ignoreGlobs = {},
                                     .userProvidedExtraCompileCommandsArgs = {},
                                     .numThreads = 1,
                                     .enableLakosianRules = true,
                                     .printToConsole = false};

    CppTool tool(constants, cmds);

    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());

    FileObject *file = nullptr;
    session.withROLock([&] {
        file = session.getFile("hello.m.cpp");
    });
    REQUIRE(file);
}
#endif
