// ct_lvtmdb.t.cpp                                         -*-C++-*-

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
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>

#include <filesystem>
#include <iostream>

#include <catch2/catch.hpp>

namespace fs = std::filesystem;

using namespace Codethink::lvtmdb;

TEST_CASE("Merge Two Different Databases")
{
    ObjectStore store1;
    ObjectStore store2;

    NamespaceObject *store1_parent = nullptr;
    NamespaceObject *store1_b = nullptr;
    NamespaceObject *store1_c = nullptr;

    NamespaceObject *store2_a = nullptr;
    NamespaceObject *store2_parent = nullptr;
    NamespaceObject *store2_b = nullptr;
    NamespaceObject *store2_c = nullptr;

    // Parent is the first one to be added, so it will probably have id 1
    // on the first database.
    store1.withRWLock([&] {
        store1_parent = store1.getOrAddNamespace("parent", "parent", nullptr);
        store1_b = store1.getOrAddNamespace("b", "b", store1_parent);
        store1_c = store1.getOrAddNamespace("c", "c", store1_parent);

        CHECK(store1.namespaces().size() == 3);
    });

    // parent is the second one to be added, so it will probably have id 2
    // on the second database.
    store2.withRWLock([&] {
        store2_a = store2.getOrAddNamespace("a", "a", store1_parent);

        // and that makes b and c from the store 2 point to a different id for parent,
        // when the db is saved on disk.
        store2_parent = store2.getOrAddNamespace("parent", "parent", nullptr);
        store2_b = store2.getOrAddNamespace("b", "b", store1_parent);
        store2_c = store2.getOrAddNamespace("c", "c", store1_parent);
        CHECK(store2.namespaces().size() == 4);
    });

    fs::path project_path = fs::temp_directory_path();

    std::error_code ec;
    std::filesystem::remove(project_path / "database_1.db", ec);
    std::filesystem::remove(project_path / "database_2.db", ec);
    // We don't care about the error code.

    {
        SociWriter writer;
        auto db_file = project_path / "database_1.db";
        CHECK(writer.createOrOpen(db_file.string()));
        store1.writeToDatabase(writer);
    }
    {
        SociWriter writer2;
        auto db_file = project_path / "database_2.db";
        CHECK(writer2.createOrOpen(db_file.string()));
        store2.writeToDatabase(writer2);
    }

    SociReader reader;
    ObjectStore mergedStore;
    auto res = mergedStore.readFromDatabase(reader, (project_path / "database_1.db").string());
    CHECK(!res.has_error());

    res = mergedStore.readFromDatabase(reader, (project_path / "database_2.db").string());
    CHECK(!res.has_error());

    // We have 3 namespaces on db1, 4 on db2. but they overlap.
    // we should only have 4 namespaces on the merged store.
    mergedStore.withROLock([&] {
        CHECK(mergedStore.namespaces().size() == 4);
    });
}
