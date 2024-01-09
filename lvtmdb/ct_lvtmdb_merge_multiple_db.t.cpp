// ct_lvtmdb_merge_multiple_db.t.cpp                                               -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_soci_helper.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>

#include <filesystem>
#include <iostream>
#include <unordered_set>

#include <catch2-local-includes.h>

namespace fs = std::filesystem;

using namespace Codethink::lvtmdb;

namespace {
const fs::path projectPath = fs::temp_directory_path();
const auto tmpDBPathA = (projectPath / "database_1.db").string();
const auto tmpDbPathB = (projectPath / "database_2.db").string();
const auto tmpDbPathC = (projectPath / "database_3.db").string();
const auto tmpDbPathD = (projectPath / "database_4.db").string();
const auto resultingDb = (projectPath / "resulting.db").string();
} // namespace

void remove_temporary_files()
{
    for (const auto& file : {tmpDBPathA, tmpDbPathB, tmpDbPathC, tmpDbPathD, resultingDb}) {
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        }
    }
}

void create_temporary_dataases()
{
    // idea: use two different stores, add a bunch of random data at it
    // then save 4 distinct versions of it, two for db1, 2 for db2
    // then merge them and check if the results are sane.
    ObjectStore store;
    auto mtx = store.rwLock();
    auto repo = store.getOrAddRepository("test_repository", "/home/ble/projects/repo");
    auto pkg_a = store.getOrAddPackage("root_pkg_a", "root_a", "/home/ble/projects/repo/root_a", nullptr, repo);
    auto pkg_b = store.getOrAddPackage("root_pkg_b", "root_b", "/home/ble/projects/repo/root_b", nullptr, repo);
    auto pkg_a_pkga =
        store.getOrAddPackage("root_pkg_a/pkga", "pkga", "/home/ble/projects/repo/root_a/pkga", pkg_a, repo);
    auto pkg_a_pkgb =
        store.getOrAddPackage("root_pkg_a/pkgb", "pkgb", "/home/ble/projects/repo/root_a/pkgb", pkg_a, repo);
    auto pkg_b_pkga =
        store.getOrAddPackage("root_pkg_b/pkga", "pkga", "/home/ble/projects/repo/root_b/pkga", pkg_b, repo);
    auto pkg_b_pkgb =
        store.getOrAddPackage("root_pkg_b/pkgb", "pkgb", "/home/ble/projects/repo/root_b/pkgb", pkg_b, repo);

    auto pkg_a_pkga_comp_a = store.getOrAddComponent("root_pkg_a/pkga/comp_a", "comp_a", pkg_a_pkga);
    auto pkg_a_pkga_comp_b = store.getOrAddComponent("root_pkg_a/pkga/comp_b", "comp_b", pkg_a_pkga);
    auto pkg_a_pkgb_comp_a = store.getOrAddComponent("root_pkg_a/pkgb/comp_a", "comp_a", pkg_a_pkgb);
    auto pkg_a_pkgb_comp_b = store.getOrAddComponent("root_pkg_a/pkgb/comp_b", "comp_b", pkg_a_pkgb);
    auto pkg_b_pkga_comp_a = store.getOrAddComponent("root_pkg_b/pkga/comp_a", "comp_a", pkg_b_pkga);
    auto pkg_b_pkga_comp_b = store.getOrAddComponent("root_pkg_b/pkga/comp_b", "comp_b", pkg_b_pkga);
    auto pkg_b_pkgb_comp_a = store.getOrAddComponent("root_pkg_b/pkgb/comp_a", "comp_a", pkg_b_pkgb);
    auto pkg_b_pkgb_comp_b = store.getOrAddComponent("root_pkg_b/pkgb/comp_b", "comp_b", pkg_b_pkgb);

    // Files are just created and we don't need to keep track of them (hopefully)'
    store.getOrAddFile("/home/ble/projects/repo/root_a/pkga/comp_a.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_a_pkga_comp_a);
    store.getOrAddFile("/home/ble/projects/repo/root_a/pkga/comp_b.cpp",
                       "compb.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_a_pkga_comp_b);
    store.getOrAddFile("/home/ble/projects/repo/root_a/pkgb/comp_a.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkgb,
                       pkg_a_pkgb_comp_a);
    store.getOrAddFile("/home/ble/projects/repo/root_a/pkgb/comp_b.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkgb,
                       pkg_a_pkgb_comp_b);
    store.getOrAddFile("/home/ble/projects/repo/root_b/pkga/comp_a.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_b_pkga_comp_a);
    store.getOrAddFile("/home/ble/projects/repo/root_b/pkga/comp_b.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_b_pkga_comp_b);
    store.getOrAddFile("/home/ble/projects/repo/root_b/pkgb/comp_a.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_b_pkgb_comp_a);
    store.getOrAddFile("/home/ble/projects/repo/root_b/pkgb/comp_b.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_a_pkga,
                       pkg_b_pkgb_comp_b);

    { // Save one db.
        SociWriter writer;
        writer.createOrOpen(tmpDBPathA);
        store.writeToDatabase(writer);
    }

    // Add more data on the ObjectStore, save it somewhere else.
    auto repo2 = store.getOrAddRepository("test_another_repo", "/home/ble/projects/repo2");
    auto pkg_repo2 =
        store.getOrAddPackage("repo2_pkg", "repo2_pkg", "/home/ble/projects/repo/repo2pkg", nullptr, repo2);
    auto pkg_repo2_pkga = store.getOrAddPackage("repo2_pkg/pkgabla",
                                                "pkgabla",
                                                "/home/ble/projects/repo/repo2pkg/pkgbla",
                                                pkg_repo2,
                                                repo2);
    auto pkg2_a_pkga_comp_a = store.getOrAddComponent("/repo2_pkg/repo2_pkg/pkgabla/comp_a", "comp_a", pkg_repo2_pkga);
    store.getOrAddFile("/home/ble/projects//repo2_pkg/repo2_pkg/pkgabla/compa.cpp",
                       "compa.cpp",
                       false,
                       {},
                       pkg_repo2_pkga,
                       pkg2_a_pkga_comp_a);

    { // Save second db.
        SociWriter writer;
        writer.createOrOpen(tmpDbPathB);
        store.writeToDatabase(writer);
    }

    ObjectStore
        store2; // smaller store, with a few of the same values - same packages as the first db but has a new component.
    auto lock2 = store2.rwLock();
    auto repo_3 = store2.getOrAddRepository("test_repository", "/home/ble/projects/repo");
    auto pkg3_a = store2.getOrAddPackage("root_pkg_a", "root_a", "/home/ble/projects/repo/root_a", nullptr, repo_3);
    auto pkg3_a_pkga =
        store2.getOrAddPackage("root_pkg_a/pkga", "pkga", "/home/ble/projects/repo/root_a/pkga", pkg3_a, repo_3);
    store2.getOrAddComponent("root_pkg_a/pkga/comp_a", "comp_a", pkg3_a_pkga);
    store2.getOrAddComponent("root_pkg_a/pkga/comp_diff", "comp_diff", pkg3_a_pkga);

    { // Save third db.
        SociWriter writer;
        writer.createOrOpen(tmpDbPathC);
        store.writeToDatabase(writer);
    }

    // Add more data on the ObjectStore, save it somewhere else.
    auto pkg4_repo =
        store2.getOrAddPackage("repo4_pkg", "repo3_pkg", "/home/ble/projects/repo/repo2pkg", nullptr, repo_3);
    auto pkg4_repo2_pkga =
        store2.getOrAddPackage("repo4_pkg", "pkgabla", "/home/ble/projects/repo/repo2pkg", pkg4_repo, repo_3);
    auto pkg4_a_pkga_comp_a = store2.getOrAddComponent("root_pkg_a/pkga/comp_a", "comp_a", pkg4_repo2_pkga);
    store2.getOrAddFile("/home/ble/projects/repo/root_a/pkga/compa.cpp",
                        "compa.cpp",
                        false,
                        {},
                        pkg4_repo2_pkga,
                        pkg4_a_pkga_comp_a);

    { // Save fourth db.
        SociWriter writer;
        writer.createOrOpen(tmpDbPathD);
        store2.writeToDatabase(writer);
    }
}

void merge_in_disk_dbs()
{
    // Now, iterate over all databases, *not* keeping all of them in memory at the same time,
    // Note that I didn't just dumped everything on disk before because this is to make sure
    // merging different in - disk databases will work.'
    for (const auto& db_file : {tmpDBPathA, tmpDbPathB, tmpDbPathC, tmpDbPathD}) {
        SociReader reader;
        ObjectStore readerStore;
        auto res = readerStore.readFromDatabase(reader, db_file);
        if (res.has_error()) {
            std::cout << res.error().what << "\n";
        }
        SociWriter writer;
        writer.createOrOpen(resultingDb);
        readerStore.writeToDatabase(writer);
    }
}

void validate_in_disk_db()
{
    SociReader reader;
    ObjectStore readerStore;
    auto res = readerStore.readFromDatabase(reader, resultingDb);

    if (res.has_error()) {
        std::cout << res.error().what << "\n";
    }

    auto lock = readerStore.readOnlyLock();
    REQUIRE(readerStore.getRepository("test_repository"));
    REQUIRE(readerStore.getRepository("test_another_repo"));

    auto packages = readerStore.getAllPackages();
    auto files = readerStore.getAllFiles();

    REQUIRE(packages.size() == 9);
    REQUIRE(files.size() == 10);
}

TEST_CASE("Merge Multiple In-Disk Databases")
{
    remove_temporary_files();
    create_temporary_dataases();
    merge_in_disk_dbs();
    validate_in_disk_db();
}
