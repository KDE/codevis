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

#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_toolexecutor.h>

#include <memory>
#include <unordered_set>

#include <catch2/catch.hpp>

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
    auto *TEST_PRJ_PATH = std::getenv("TEST_PRJ_PATH");
    if (TEST_PRJ_PATH == nullptr) {
        WARN("TEST_PRJ_PATH not set. Please set it to <your_srcdir>/lvtclp/systemtests/");
        REQUIRE(false);
    }
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
        std::vector<std::filesystem::path>{},
        std::vector<std::pair<std::string, std::string>>{},
        [](const std::string&) {},
        std::nullopt,
        false,
        saveCommentsCallback));
    REQUIRE(foundComments.size() == 4);
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.h", "klass", 7, 7}) != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.h", "Test only include", 4, 4})
            != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.cpp", "Some other comment...?", 4, 4})
            != foundComments.end());
    REQUIRE(foundComments.find(FoundCommentTestData{"oneaaa_comp.cpp", "Main include", 1, 1}) != foundComments.end());
}
