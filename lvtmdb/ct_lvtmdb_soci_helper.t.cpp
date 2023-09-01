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
#include <optional>

#include <catch2-local-includes.h>

namespace fs = std::filesystem;

using namespace Codethink::lvtmdb;

template<typename T>
struct is_optional_t {
    static constexpr bool value = false;
};
template<typename U>
struct is_optional_t<std::optional<U>> {
    static constexpr bool value = true;
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

    auto checkItem = [&result](auto row, auto col, auto expectedValue) {
        if constexpr (is_optional_t<decltype(expectedValue)>::value) {
            auto& [maybeData, isNull] = result[row][col];
            if (expectedValue.has_value()) {
                REQUIRE(std::any_cast<typename decltype(expectedValue)::value_type>(maybeData)
                        == expectedValue.value());
            } else {
                REQUIRE(isNull);
            }
        } else {
            auto& [maybeData, _] = result[row][col];
            REQUIRE(std::any_cast<decltype(expectedValue)>(maybeData) == expectedValue);
        }
    };
    auto checkRow = [&](int id,
                        int version,
                        std::optional<long long> parent_id,
                        std::string const& name,
                        std::string const& qualified_name) {
        static auto constexpr ID = 0;
        static auto constexpr VERSION = 1;
        static auto constexpr PARENT_ID = 2;
        static auto constexpr NAME = 3;
        static auto constexpr QUALIFIED_NAME = 4;

        checkItem(id - 1, ID, id);
        checkItem(id - 1, VERSION, version);
        checkItem(id - 1, PARENT_ID, parent_id);
        checkItem(id - 1, NAME, name);
        checkItem(id - 1, QUALIFIED_NAME, qualified_name);
    };

    REQUIRE(result.size() == 3);
    checkRow(1, 0, std::nullopt, "parent", "parent");
    checkRow(2, 0, 1, "b", "b");
    checkRow(3, 0, 1, "c", "c");
}
