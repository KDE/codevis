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

#include <ct_lvtclp_tool.h>

#include <ct_lvtclp_testutil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <fstream>
#include <initializer_list>

#include <catch2-local-includes.h>

#include <autogen-test-variables.h>

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

static void checkDatabaseOriginal(ObjectStore& session)
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

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

static void checkDatabaseModifiedFile(ObjectStore& session, bool printToConsole = false)
{
    const std::initializer_list<ModelUtil::SourceFileModel> files = {
        {"groups/one/onedep/onedep_dep.cpp",
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

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

static void checkDatabaseNewFiles(ObjectStore& session)
// check the database after onedep_newfile has been added
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

    ModelUtil::checkSourceFiles(session, files);
    ModelUtil::checkComponents(session, comps);
    ModelUtil::checkPackages(session, pkgs);
    ModelUtil::checkUDTs(session, udts);
}

static void runTool(Tool& tool, const bool madeChanges, const unsigned numRuns = 1, bool verbose = false)
{
    tool.setPrintToConsole(verbose);
    for (unsigned i = 0; i < numRuns; i++) {
        REQUIRE(tool.runFull());
        REQUIRE(tool.lastRunMadeChanges() == madeChanges);
    }
}

struct ToolTestFixture {
    ToolTestFixture(): topLevel(std::filesystem::temp_directory_path() / "ct_lvtclp_tool_test")
    {
        createTestEnv(topLevel);
    }

    ~ToolTestFixture()
    {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    std::filesystem::path topLevel;
};

TEST_CASE_METHOD(ToolTestFixture, "Tool")
{
    constexpr unsigned NUM_RERUNS = 2;

    {
        StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                        {"groups/one/onedep/onedep_dep.cpp", "build/onedep_dep.o"}},
                                       "placeholder",
                                       {"-Igroups/one/onetop", "-Igroups/one/onedep"},
                                       topLevel);

        Tool tool(topLevel, cmds, topLevel / "database");
        ObjectStore& session = tool.getObjectStore();

        // initial parse
        runTool(tool, true);
        checkDatabaseOriginal(session);

        // re-run incrementally
        runTool(tool, false, NUM_RERUNS);
        checkDatabaseOriginal(session);

        // modify a file, adding a class
        std::filesystem::path fileToChange = topLevel / "groups/one/onetop/onetop_top.h";
        {
            // open for appending
            std::ofstream of(fileToChange.string(), std::ios_base::out | std::ios_base::app);
            REQUIRE(!of.fail());
            of << "namespace onetop { class NewClass {}; }" << std::endl;
        }
        runTool(tool, true);
        checkDatabaseModifiedFile(session);

        // re-run incrementally
        runTool(tool, false, NUM_RERUNS);
        checkDatabaseModifiedFile(session);

        // change modified file back again, run again, and re-run
        createTestEnv(topLevel);
        runTool(tool, true);
        checkDatabaseOriginal(session);
        runTool(tool, false, NUM_RERUNS);
        checkDatabaseOriginal(session);
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

        Tool toolPlus(topLevel, cmdsPlus, topLevel / "database");

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
        checkDatabaseNewFiles(session);
        runTool(toolPlus, false, NUM_RERUNS);
        checkDatabaseNewFiles(session);

        // delete the new component and re-run
        createTestEnv(topLevel);
        runTool(toolPlus, true);
        checkDatabaseOriginal(session);
        runTool(toolPlus, false, NUM_RERUNS);
        checkDatabaseOriginal(session);
    }
}

TEST_CASE("Run Tool on example project")
{
    StaticCompilationDatabase cmds({{"hello.m.cpp", "hello.m.o"}}, "placeholder", {}, PREFIX + "/hello_world/");
    Tool tool(PREFIX + "/hello_world/", cmds, PREFIX + "/hello_world/database");
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
    auto sourcePath = PREFIX + "/package_circular_dependency/";

    auto files = std::initializer_list<std::pair<std::string, std::string>>{
        {"groups/one/onepkg/ct_onepkg_circle.cpp", "ct_onepkg_circle.o"},
        {"groups/one/onepkg/ct_onepkg_thing.cpp", "ct_onepkg_thing.o"}};

    StaticCompilationDatabase cmds(
        files,
        "placeholder",
        {"-I" + sourcePath + "/groups/one/onepkg/", "-I" + sourcePath + "/groups/two/twodmo/"},
        sourcePath);
    Tool tool(sourcePath, cmds, sourcePath + "database");
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
    auto const prjAPath = PREFIX + "/project_with_includes_outside_src/prjA";
    auto const prjBPath = PREFIX + "/project_with_includes_outside_src/prjB";
    StaticCompilationDatabase cmds({{"groups/one/oneaaa/oneaaa_comp.cpp", "oneaaa_comp.o"}},
                                   "placeholder",
                                   {"-I" + prjAPath + "/groups/one/oneaaa/", "-I" + prjBPath + "/groups/two/twoaaa/"},
                                   prjAPath);
    Tool tool(prjAPath, cmds, prjAPath + "/database");
    ObjectStore& session = tool.getObjectStore();

    REQUIRE(tool.runFull());
    session.withROLock([&] {
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
    auto const prjAPath = PREFIX + "/test_only_dependencies/prjA";
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
    Tool tool(prjAPath, cmds, prjAPath + "/database");

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