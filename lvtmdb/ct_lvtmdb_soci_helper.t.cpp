// ct_lvtmdb_soci_helper.t.cpp                                        -*-C++-*-

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

#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_helper.h>
#include <ct_lvtmdb_soci_writer.h>

#include <filesystem>
#include <iostream>
#include <unordered_set>

#include <catch2-local-includes.h>

namespace fs = std::filesystem;

using namespace Codethink::lvtmdb;

struct DBTestResult {
    struct HashFunc {
        size_t operator()(const DBTestResult& data) const
        {
            return data.id;
        }
    };

    int id;
    int version;
    long long parentId;
    std::string name;
    std::string qualifiedName;

    bool operator==(const DBTestResult& other) const
    {
        return this->id == other.id && this->version == other.version && this->parentId == other.parentId
            && this->name == other.name && this->qualifiedName == other.qualifiedName;
    }
};

struct RawDBColsHashFunc2 {
    size_t operator()(const std::any& data) const
    {
        return std::any_cast<int>(data);
    }
};

TEST_CASE("Run single query helper")
{
    fs::path project_path = fs::temp_directory_path();
    auto tmpDBPath = project_path / "database.db";

    {
        ObjectStore store1;
        NamespaceObject *store1_parent = nullptr;
        NamespaceObject *store1_b = nullptr;
        NamespaceObject *store1_c = nullptr;
        store1.withRWLock([&] {
            store1_parent = store1.getOrAddNamespace("parent", "parent", nullptr);
            store1_b = store1.getOrAddNamespace("b", "b", store1_parent);
            store1_c = store1.getOrAddNamespace("c", "c", store1_parent);
            CHECK(store1.namespaces().size() == 3);
        });
        std::error_code ec;
        std::filesystem::remove(tmpDBPath, ec);
        SociWriter writer;
        CHECK(writer.createOrOpen(tmpDBPath.string()));
        store1.writeToDatabase(writer);
    }

    soci::session db;
    db.open(*soci::factory_sqlite3(), tmpDBPath.string());
    auto result = SociHelper::runSingleQuery(db, "SELECT * FROM namespace_declaration");
    auto obtainedData = std::unordered_set<DBTestResult, DBTestResult::HashFunc>{};
    for (auto&& rowdata : result) {
        obtainedData.insert({std::any_cast<int>(rowdata[0].value()),
                             std::any_cast<int>(rowdata[1].value()),
                             rowdata[2].has_value() ? std::any_cast<long long>(rowdata[2].value()) : -1,
                             std::any_cast<std::string>(rowdata[3].value()),
                             std::any_cast<std::string>(rowdata[4].value())});
    }

    auto expectedData = std::unordered_set<DBTestResult, DBTestResult::HashFunc>{{1, 0, -1, "parent", "parent"},
                                                                                 {2, 0, 1, "b", "b"},
                                                                                 {3, 0, 1, "c", "c"}};
    REQUIRE(obtainedData == expectedData);
}
