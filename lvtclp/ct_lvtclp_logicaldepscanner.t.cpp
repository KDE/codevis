// ct_lvtclp_logicaldepscanner.t.cpp                                  -*-C++-*-

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
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_toolexecutor.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Auto generated by CMake / see main CMakeLists file, and
// the CMakeLists on this folder.
#include <catch2-local-includes.h>
#include <test-project-paths.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

struct FoundCommentTestData {
    struct HashFunc {
        size_t operator()(const FoundCommentTestData& data) const
        {
            return std::hash<std::string>{}(data.filename) ^ std::hash<std::string>{}(data.briefText)
                ^ std::hash<unsigned>{}(data.startLine) ^ std::hash<unsigned>{}(data.endLine);
        }
    };

    std::string filename;
    std::string briefText;
    unsigned startLine;
    unsigned endLine;

    bool operator==(const FoundCommentTestData& other) const
    {
        return this->filename == other.filename && this->briefText == other.briefText
            && this->startLine == other.startLine && this->endLine == other.endLine;
    }

    friend auto operator<<(std::ostream& os, FoundCommentTestData const& self) -> std::ostream&
    {
        return os << self.filename << " comment: '" << self.briefText << "' from line " << self.startLine << " to "
                  << self.endLine;
    }
};

TEST_CASE("Optional comment callbacks")
{
    auto const PREFIX = std::string{TEST_PRJ_PATH};

    auto const prjAPath = PREFIX + "/project_with_includes_outside_src/prjA";
    auto const prjBPath = PREFIX + "/project_with_includes_outside_src/prjB";
    auto cdb = StaticCompilationDatabase{{{"groups/one/oneaaa/oneaaa_comp.cpp", "oneaaa_comp.o"},
                                          {"groups/one/oneaaa/oneaaa_comp.h", "oneaaa_comp.h.o"}},
                                         "placeholder",
                                         {"-I" + prjAPath + "/groups/one/oneaaa/",
                                          "-I" + prjBPath + "/groups/two/twoaaa/",
                                          "-fparse-all-comments",
                                          "-std=c++17"},
                                         prjAPath};
    auto memDb = lvtmdb::ObjectStore{};

    auto foundComments = std::unordered_set<FoundCommentTestData, FoundCommentTestData::HashFunc>{};
    auto saveCommentsCallback =
        [&](const std::string& filename, const std::string& briefText, unsigned startLine, unsigned endLine) {
            foundComments.insert({filename, briefText, startLine, endLine});
        };

    auto executor = ToolExecutor{cdb, 1, [](auto&& _1, auto&& _2) {}, memDb};
    (void) executor.execute(std::make_unique<LogicalDepActionFactory>(
        memDb,
        PREFIX,
        std::filesystem::path{},
        std::vector<std::filesystem::path>{},
        std::vector<std::pair<std::string, std::string>>{},
        [](const std::string&) {},
        std::nullopt,
        false,
        /*enableLakosianRules=*/true,
        saveCommentsCallback));
    REQUIRE(foundComments.size() == 4);
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.h", "klass", 7, 7}) != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.h", "Test only include", 4, 4})
            != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.cpp", "Some other comment...?", 4, 4})
            != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.cpp", "Main include", 1, 1}) != foundComments.end());
}

TEST_CASE("Smoke test partial template specialization - Must not crash")
{
    /*
     * Smoke test for a partial template specialization with nested data type that was crashing while trying
     * to resolve the template specialization arguments. The code has been changed and this test added to ensure
     * we don't have a regression.
     */
    auto const PREFIX = std::string{TEST_PRJ_PATH};

    auto const prjPath = PREFIX + "/templates/";
    auto cdb = StaticCompilationDatabase{{{"templates.m.cpp", "templates.m.o"}},
                                         "placeholder",
                                         {"-I" + prjPath, "-std=c++17"},
                                         prjPath};
    auto memDb = lvtmdb::ObjectStore{};

    auto executor = ToolExecutor{cdb, 1, [](auto&& _1, auto&& _2) {}, memDb};
    (void) executor.execute(std::make_unique<LogicalDepActionFactory>(
        memDb,
        PREFIX,
        std::filesystem::path{},
        std::vector<std::filesystem::path>{},
        std::vector<std::pair<std::string, std::string>>{},
        [](const std::string&) {},
        std::nullopt,
        false,
        /*enableLakosianRules=*/false));
}

TEST_CASE("Test global free functions with same name in different compilation units")
{
    auto const PREFIX = std::string{TEST_PRJ_PATH};

    auto const prjPath = PREFIX + "/cpp_general_test/";
    auto cdb = StaticCompilationDatabase{{
                                             {"hello.m.cpp", "hello.m.o"},
                                             {"other.cpp", "other.o"},
                                             {"third.cpp", "third.o"},
                                         },
                                         "placeholder",
                                         {"-I" + prjPath, "-std=c++17"},
                                         prjPath};
    auto memDb = lvtmdb::ObjectStore{};

    auto executor = ToolExecutor{cdb, 1, [](auto&& _1, auto&& _2) {}, memDb};
    (void) executor.execute(std::make_unique<LogicalDepActionFactory>(
        memDb,
        PREFIX,
        std::filesystem::path{},
        std::vector<std::filesystem::path>{},
        std::vector<std::pair<std::string, std::string>>{},
        [](const std::string&) {},
        std::nullopt,
        false,
        /*enableLakosianRules=*/true));

    memDb.withROLock([&memDb]() {
        auto files = memDb.getAllFiles();
        REQUIRE(files.size() == 3);
        auto fileNameSet = std::unordered_set<std::string>{};
        for (auto const& f : files) {
            auto lock = f->readOnlyLock();
            fileNameSet.insert(f->name());
        }
        REQUIRE(fileNameSet == std::unordered_set<std::string>{"hello.m.cpp", "other.cpp", "third.cpp"});

        auto allGlobalFunctions = std::unordered_map<std::string, FunctionObject *>{};
        for (auto const& f : files) {
            auto fileLock = f->readOnlyLock();
            for (auto const& func : f->globalFunctions()) {
                auto funcLock = func->readOnlyLock();
                allGlobalFunctions[func->qualifiedName()] = func;
            }
        }
        REQUIRE(allGlobalFunctions.size() == 4);

        REQUIRE(allGlobalFunctions.contains("main@hello.m"));
        REQUIRE(allGlobalFunctions.contains("f@hello.m"));
        REQUIRE(allGlobalFunctions.contains("f@other"));
        REQUIRE(allGlobalFunctions.contains("g@third"));

        {
            auto gFuncLock = allGlobalFunctions["main@hello.m"]->readOnlyLock();
            auto mainCallees = allGlobalFunctions["main@hello.m"]->callees();
            REQUIRE(mainCallees.size() == 2);

            auto hasCalleeNamed = [&mainCallees](std::string const& calleeFunctionName) {
                return std::find_if(mainCallees.begin(),
                                    mainCallees.end(),
                                    [&calleeFunctionName](auto *calleeF) {
                                        auto calleeFLock = calleeF->readOnlyLock();
                                        return calleeF->qualifiedName() == calleeFunctionName;
                                    })
                    != mainCallees.end();
            };
            REQUIRE(hasCalleeNamed("f@hello.m"));

            // Note: This assumes that 'g' function is declared in the same component that it's defined.
            REQUIRE(hasCalleeNamed("g@third"));
        }
    });
}
