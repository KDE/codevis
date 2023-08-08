// ct_lvtclp_pysicaldepscanner.t.cpp                                  -*-C++-*-

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

#include <ct_lvtclp_physicaldepscanner.h>
#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_toolexecutor.h>

#include <memory>
#include <unordered_set>

#include <catch2/catch.hpp>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

struct FoundIncludeTestData {
    struct HashFunc {
        size_t operator()(const FoundIncludeTestData& data) const
        {
            return std::hash<std::string>{}(data.sourceFile) ^ std::hash<std::string>{}(data.includedFile)
                ^ std::hash<unsigned>{}(data.lineNo);
        }
    };

    std::string sourceFile;
    std::string includedFile;
    unsigned lineNo;

    bool operator==(const FoundIncludeTestData& other) const
    {
        return this->sourceFile == other.sourceFile && this->includedFile == other.includedFile
            && this->lineNo == other.lineNo;
    }

    friend auto operator<<(std::ostream& os, FoundIncludeTestData const& self) -> std::ostream&
    {
        return os << self.sourceFile << " includes " << self.includedFile << " on line " << self.lineNo;
    }
};

TEST_CASE("Optional include location callbacks")
{
    auto *TEST_PRJ_PATH = std::getenv("TEST_PRJ_PATH");
    if (TEST_PRJ_PATH == nullptr) {
        WARN("TEST_PRJ_PATH not set. Please set it to <your_srcdir>/lvtclp/systemtests/");
        REQUIRE(false);
    }
    auto const PREFIX = std::string{TEST_PRJ_PATH};

    auto const prjAPath = PREFIX + "/project_with_includes_outside_src/prjA";
    auto const prjBPath = PREFIX + "/project_with_includes_outside_src/prjB";
    auto cdb =
        StaticCompilationDatabase{{{"groups/one/oneaaa/oneaaa_comp.cpp", "oneaaa_comp.o"}},
                                  "placeholder",
                                  {"-I" + prjAPath + "/groups/one/oneaaa/", "-I" + prjBPath + "/groups/two/twoaaa/"},
                                  prjAPath};
    auto memDb = lvtmdb::ObjectStore{};

    auto executor = ToolExecutor{cdb, 1, [](auto&& _1, auto&& _2) {}, memDb};

    auto foundIncludes = std::unordered_set<FoundIncludeTestData, FoundIncludeTestData::HashFunc>{};
    auto headerLocationCallback = [&](std::string const& sourceFile, std::string const& includedFile, unsigned lineNo) {
        foundIncludes.insert(FoundIncludeTestData{sourceFile, includedFile, lineNo});
    };
    auto err = executor.execute(std::make_unique<DepScanActionFactory>(
        memDb,
        PREFIX,
        std::vector<std::filesystem::path>{},
        std::vector<std::pair<std::string, std::string>>{},
        [](auto&& _) {},
        std::vector<std::string>{},
        headerLocationCallback));

    REQUIRE(!err);
    REQUIRE(foundIncludes.size() == 2);
    REQUIRE(foundIncludes.find(FoundIncludeTestData{"oneaaa_comp.h", "twoaaa_comp.h", 5}) != foundIncludes.end());
    REQUIRE(foundIncludes.find(FoundIncludeTestData{"oneaaa_comp.cpp", "oneaaa_comp.h", 2}) != foundIncludes.end());
}
