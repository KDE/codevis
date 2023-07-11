// ct_lvtclp_filesystemscanner.t.cpp                                 -*-C++-*-

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

#include <ct_lvtclp_filesystemscanner.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_tool.h>

#include <ct_lvtmdb_objectstore.h>

#include <QtGlobal>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <initializer_list>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_repositoryobject.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

using namespace Codethink;

using Codethink::lvtclp::FilesystemScanner;
using Codethink::lvtclp::StaticCompilationDatabase;
using Codethink::lvtclp::Test_Util;
using Codethink::lvtclp::Tool;

PyDefaultGilReleasedContext _;

const auto messageCallback = [](const std::string&, long) {};

static void createComponent(const std::filesystem::path& dir, const std::string& name, const std::string& prefix = "")
{
    for (const char *ext : {".h", ".cpp", ".t.cpp"}) {
        std::string pkgname = dir.filename().string();
        std::string fileName;
        fileName.append(pkgname).append("_").append(name).append(ext);
        if (!prefix.empty()) {
            std::string newFileName;
            newFileName.append(prefix).append("_").append(fileName);
            fileName = std::move(newFileName);
        }

        REQUIRE(Test_Util::createFile(dir / fileName));
    }
}

static void createTestEnv(const std::filesystem::path& topLevel)
{
    if (std::filesystem::exists(topLevel)) {
        std::filesystem::remove_all(topLevel);
    }

    REQUIRE(std::filesystem::create_directories(topLevel / "groups/one/onepkg"));
    REQUIRE(std::filesystem::create_directories(topLevel / "groups/two/twofoo"));
    REQUIRE(std::filesystem::create_directories(topLevel / "groups/two/twobar"));
    REQUIRE(std::filesystem::create_directories(topLevel / "unrelated"));
    REQUIRE(std::filesystem::create_directories(topLevel / "groups/one/doc"));
    REQUIRE(std::filesystem::create_directories(topLevel / "groups/pkgnme"));
    REQUIRE(std::filesystem::create_directories(topLevel / "standalones/s_pkgtst"));

    createComponent(topLevel / "groups/one/onepkg", "foo");
    createComponent(topLevel / "groups/one/onepkg", "bar");
    createComponent(topLevel / "groups/two/twofoo", "component");
    createComponent(topLevel / "groups/two/twobar", "component");
    createComponent(topLevel / "standalones/s_pkgtst", "somecmp");

    REQUIRE(Test_Util::createFile(topLevel / "unrelated/test.cpp"));
}

static bool compareResultList(std::vector<std::string> expected, std::vector<std::string> result)
{
    if (expected.size() != result.size()) {
        return false;
    }

    std::sort(expected.begin(), expected.end());
    std::sort(result.begin(), result.end());
    return expected == result;
}

struct FilesystemScannerFixture {
    FilesystemScannerFixture(): topLevel(std::filesystem::temp_directory_path() / "lvtclp_filesystemscanner_test")
    {
        if (std::filesystem::exists(topLevel)) {
            REQUIRE(std::filesystem::remove_all(topLevel));
        }
    }

    ~FilesystemScannerFixture()
    {
        if (std::filesystem::exists(topLevel)) {
            REQUIRE(std::filesystem::remove_all(topLevel));
        }
    }

    std::filesystem::path topLevel;
};

TEST_CASE_METHOD(FilesystemScannerFixture, "Filesystem scanner")
{
    createTestEnv(topLevel);

    Codethink::lvtclp::StaticCompilationDatabase cdb(
        {
            {(topLevel / "groups/one/onepkg/onepkg_foo.cpp").string(), ""},
            {(topLevel / "groups/one/onepkg/onepkg_bar.cpp").string(), ""},
            {(topLevel / "groups/two/twofoo/twofoo_component.cpp").string(), ""},
            {(topLevel / "groups/two/twobar/twobar_component.cpp").string(), ""},
            {(topLevel / "standalones/s_pkgtst/s_pkgtst_somecmp.cpp").string(), ""},
        },
        "",
        {},
        topLevel);

    // Test we can parse okay when everything is new to the database
    {
        std::filesystem::remove("test.db");

        lvtmdb::ObjectStore memDb;
        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        lvtmdb::SociWriter writer;
        writer.createOrOpen("test.db");
        memDb.writeToDatabase(writer);

        // The old code tested if things are saved in the database. this new
        // one takes a different approach (because checking if something is )
        // in the database will need direct access to soci or to a underlying
        // database package - but we have the SociReader, that should read
        // everything from a database to an ObjectStore. so, clear the original
        // store, and reload from disk, test if things still exist there.
        memDb.withRWLock([&] {
            memDb.clear();
        });

        REQUIRE(memDb.packages().empty());
        REQUIRE(memDb.components().empty());

        lvtmdb::SociReader reader;
        auto ret = memDb.readFromDatabase(reader, "test.db");
        REQUIRE(ret.has_value());

        auto lock = memDb.rwLock();
        (void) lock;

        // package groups
        auto *one = memDb.getPackage("groups/one");
        REQUIRE(one);
        auto *two = memDb.getPackage("groups/two");
        REQUIRE(two);
        auto *standalonepkg = memDb.getPackage("standalones/s_pkgtst");
        REQUIRE(standalonepkg);

        // packages
        auto *onepkg = memDb.getPackage("groups/one/onepkg");
        REQUIRE(onepkg);
        auto onepkg_lock = onepkg->readOnlyLock();
        (void) onepkg_lock;
        REQUIRE(onepkg->parent());
        auto parent_lock = onepkg->parent()->readOnlyLock();
        (void) parent_lock;
        REQUIRE(onepkg->parent()->qualifiedName() == "groups/one");

        auto *twofoo = memDb.getPackage("groups/two/twofoo");
        REQUIRE(twofoo);

        auto two_lock = twofoo->readOnlyLock();
        (void) two_lock;
        REQUIRE(twofoo->parent());

        auto two_foo_parent_lock = twofoo->parent()->readOnlyLock();
        (void) two_foo_parent_lock;
        REQUIRE(twofoo->parent()->qualifiedName() == "groups/two");

        auto *twobar = memDb.getPackage("groups/two/twobar");
        REQUIRE(twobar);

        auto two_bar_lock = twobar->readOnlyLock();
        (void) two_bar_lock;
        REQUIRE(twobar->parent());

        auto two_bar_parent_lock = twobar->parent()->readOnlyLock();
        (void) two_bar_parent_lock;
        REQUIRE(twobar->parent()->qualifiedName() == "groups/two");

        // files
        auto *foo_comp = memDb.getComponent("groups/one/onepkg/onepkg_foo");
        auto *bar_comp = memDb.getComponent("groups/one/onepkg/onepkg_bar");
        auto *other_comp = memDb.getComponent("groups/two/twofoo/twofoo_component");
        auto *twobar_comp = memDb.getComponent("groups/two/twobar/twobar_component");
        auto *standalone_comp = memDb.getComponent("standalones/s_pkgtst/s_pkgtst_somecmp");
        auto *null_pkg = memDb.getComponent("unrelated");
        auto *null_pkg_2 = memDb.getComponent("doc");
        auto *null_pkg_3 = memDb.getComponent("pkgname");

        REQUIRE(foo_comp);
        REQUIRE(bar_comp);
        REQUIRE(other_comp);
        REQUIRE(twobar_comp);
        REQUIRE(standalone_comp);
        REQUIRE_FALSE(null_pkg);
        REQUIRE_FALSE(null_pkg_2);
        REQUIRE_FALSE(null_pkg_3);

        auto checkComp = [](lvtmdb::ComponentObject *comp, lvtmdb::PackageObject *const supposedParent) {
            auto lock = comp->readOnlyLock();
            (void) lock;
            REQUIRE(comp->package() == supposedParent);
        };

        checkComp(foo_comp, onepkg);
        checkComp(bar_comp, onepkg);
        checkComp(other_comp, twofoo);
        checkComp(twobar_comp, twobar);
        checkComp(standalone_comp, standalonepkg);

        // IncrementalResult
        REQUIRE(compareResultList({"groups/two/twofoo/twofoo_component.cpp",
                                   "groups/two/twofoo/twofoo_component.h",
                                   "groups/two/twobar/twobar_component.h",
                                   "groups/two/twobar/twobar_component.cpp",
                                   "groups/one/onepkg/onepkg_foo.h",
                                   "groups/one/onepkg/onepkg_bar.h",
                                   "groups/one/onepkg/onepkg_foo.cpp",
                                   "groups/one/onepkg/onepkg_bar.cpp",
                                   "standalones/s_pkgtst/s_pkgtst_somecmp.cpp",
                                   "standalones/s_pkgtst/s_pkgtst_somecmp.h"},
                                  res.newFiles));
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(res.deletedFiles.empty());

        REQUIRE(compareResultList({"groups/one",
                                   "groups/two",
                                   "groups/one/onepkg",
                                   "groups/two/twofoo",
                                   "groups/two/twobar",
                                   "standalones/s_pkgtst"},
                                  res.newPkgs));
        REQUIRE(res.deletedPkgs.empty());
    }

    // Test nothing is added when reparsing the same files with an already
    // populated database
    {
        lvtmdb::ObjectStore memDb;

        lvtmdb::SociReader reader;
        auto ret = memDb.readFromDatabase(reader, "test.db");
        REQUIRE(ret.has_value());

        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        // nothing changed:
        REQUIRE(res.newFiles.empty());
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(res.deletedFiles.empty());
        REQUIRE(res.newPkgs.empty());
        REQUIRE(res.deletedPkgs.empty());
    }

    // Test a deleted file is picked up
    {
        const char *file = "groups/one/onepkg/onepkg_foo.cpp";

        createTestEnv(topLevel);

        // delete a file
        REQUIRE(std::filesystem::remove(topLevel / file));

        lvtmdb::ObjectStore memDb;
        lvtmdb::SociReader reader;
        auto ret = memDb.readFromDatabase(reader, "test.db");
        REQUIRE(ret.has_value());

        // scan
        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        {
            // file was deleted
            REQUIRE(compareResultList({file}, res.deletedFiles));

            // nothing else changed
            REQUIRE(res.newFiles.empty());
            REQUIRE(res.modifiedFiles.empty());
            REQUIRE(res.newPkgs.empty());
            REQUIRE(res.deletedPkgs.empty());

            // In the next step we add the deleted file back again and see if the
            // scanner finds it. The scanner spots new files by comparing with the
            // database so we first need to delete the deleted file from the database
            for (auto deletedFile : res.deletedFiles) {
                memDb.withRWLock([&] {
                    auto *memFile = memDb.getFile(deletedFile);
                    std::set<intptr_t> removed;
                    memDb.removeFile(memFile, removed);
                });
            }
        }

        // add deleted file back again
        REQUIRE(Test_Util::createFile(topLevel / file));

        // scan (testing re-using a scanner instance)
        res = scanner.scanCompilationDb();

        // file was added back
        REQUIRE(compareResultList({file}, res.newFiles));

        // nothing else changed
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(res.deletedFiles.empty());
        REQUIRE(res.newPkgs.empty());
        REQUIRE(res.deletedPkgs.empty());
    }

    // Test that we detect file modification
    {
        const char *file = "groups/two/twobar/twobar_component.cpp";

        createTestEnv(topLevel);

        // modify file
        std::ofstream ofile((topLevel / file).c_str());
        ofile << "namespace Codethink { }" << std::endl;
        ofile.close();

        // scan
        lvtmdb::ObjectStore memDb;
        lvtmdb::SociReader reader;
        auto ret = memDb.readFromDatabase(reader, "test.db");
        REQUIRE(ret.has_value());

        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        // file was modified
        REQUIRE(compareResultList({file}, res.modifiedFiles));

        // nothing else changed
        REQUIRE(res.newFiles.empty());
        REQUIRE(res.deletedFiles.empty());
        REQUIRE(res.newPkgs.empty());
        REQUIRE(res.deletedPkgs.empty());

        // change file back again
        REQUIRE(std::filesystem::remove(topLevel / file));
        REQUIRE(Test_Util::createFile(topLevel / file));

        // re-scan
        res = scanner.scanCompilationDb();

        // file was modified
        REQUIRE(compareResultList({file}, res.modifiedFiles));

        // nothing else changed
        REQUIRE(res.newFiles.empty());
        REQUIRE(res.deletedFiles.empty());
        REQUIRE(res.newPkgs.empty());
        REQUIRE(res.deletedPkgs.empty());
    }

    // delete a package and add it back again
    {
        createTestEnv(topLevel);
        // scan
        lvtmdb::ObjectStore memDb;

        // Scan original files.
        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        REQUIRE(compareResultList({"groups/two",
                                   "groups/one",
                                   "groups/one/onepkg",
                                   "groups/two/twofoo",
                                   "groups/two/twobar",
                                   "standalones/s_pkgtst"},
                                  res.newPkgs));

        // delete a package
        REQUIRE(std::filesystem::remove_all(topLevel / "groups/one/onepkg"));

        res = scanner.scanCompilationDb();
        REQUIRE(res.newFiles.empty());
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(compareResultList({"groups/one/onepkg/onepkg_foo.h",
                                   "groups/one/onepkg/onepkg_bar.h",
                                   "groups/one/onepkg/onepkg_foo.cpp",
                                   "groups/one/onepkg/onepkg_bar.cpp"},
                                  res.deletedFiles));

        REQUIRE(res.newPkgs.empty());
        REQUIRE(compareResultList({"groups/one/onepkg", "groups/one"}, res.deletedPkgs));

        for (auto file : res.deletedFiles) {
            lvtmdb::FileObject *memFile = nullptr;
            memDb.withROLock([&] {
                memFile = memDb.getFile(file);
            });
            std::set<intptr_t> removed;
            memDb.removeFile(memFile, removed);
        }
        for (auto pkg : res.deletedPkgs) {
            lvtmdb::PackageObject *memPkg = nullptr;

            memDb.withROLock([&] {
                memPkg = memDb.getPackage(pkg);
            });
            std::set<intptr_t> removed;
            memDb.removePackage(memPkg, removed);
        }

        // add package back again
        createTestEnv(topLevel);
        // re-scan
        res = scanner.scanCompilationDb();

        REQUIRE(compareResultList({"groups/one/onepkg/onepkg_foo.h",
                                   "groups/one/onepkg/onepkg_bar.h",
                                   "groups/one/onepkg/onepkg_foo.cpp",
                                   "groups/one/onepkg/onepkg_bar.cpp"},
                                  res.newFiles));
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(res.deletedFiles.empty());

        for (const auto& pkg : res.newPkgs) {
            std::cout << "New packages added " << pkg << "\n";
        }

        REQUIRE(compareResultList({"groups/one", "groups/one/onepkg"}, res.newPkgs));
        REQUIRE(res.deletedPkgs.empty());
    }

    // delete a package group and add it back again
    {
        std::cout << "---------------------\n";
        createTestEnv(topLevel);

        // scan
        lvtmdb::ObjectStore memDb;
        FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
        FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

        // delete a package group
        REQUIRE(std::filesystem::remove_all(topLevel / "groups/two"));

        res = scanner.scanCompilationDb();

        REQUIRE(res.newFiles.empty());
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(compareResultList({"groups/two/twofoo/twofoo_component.cpp",
                                   "groups/two/twofoo/twofoo_component.h",
                                   "groups/two/twobar/twobar_component.h",
                                   "groups/two/twobar/twobar_component.cpp"},
                                  res.deletedFiles));

        REQUIRE(res.newPkgs.empty());
        REQUIRE(compareResultList({"groups/two", "groups/two/twofoo", "groups/two/twobar"}, res.deletedPkgs));

        // delete package from the database so the next step works okay
        for (auto file : res.deletedFiles) {
            lvtmdb::FileObject *memFile = nullptr;
            memDb.withROLock([&] {
                memFile = memDb.getFile(file);
            });
            std::set<intptr_t> removed;
            memDb.removeFile(memFile, removed);
        }
        for (auto pkg : res.deletedPkgs) {
            lvtmdb::PackageObject *memPkg = nullptr;

            memDb.withROLock([&] {
                memPkg = memDb.getPackage(pkg);
            });
            std::set<intptr_t> removed;
            memDb.removePackage(memPkg, removed);
        }

        // add package group back again
        createTestEnv(topLevel);

        // re-scan
        res = scanner.scanCompilationDb();

        REQUIRE(compareResultList({"groups/two/twofoo/twofoo_component.cpp",
                                   "groups/two/twofoo/twofoo_component.h",
                                   "groups/two/twobar/twobar_component.h",
                                   "groups/two/twobar/twobar_component.cpp"},
                                  res.newFiles));
        REQUIRE(res.modifiedFiles.empty());
        REQUIRE(res.deletedFiles.empty());

        REQUIRE(compareResultList({"groups/two", "groups/two/twofoo", "groups/two/twobar"}, res.newPkgs));
        REQUIRE(res.deletedPkgs.empty());
    }
}

TEST_CASE_METHOD(FilesystemScannerFixture, "Non Lakosian")
{
    // create a partially lakosian hierarchy:
    // groups/one/onepkg/onepkg_foo.{cpp,h}
    // groups/one/onepkg/onepkg_bar.{cpp,h}
    // thirdparty/nonlakosian.cpp
    // include/nonlakosian.hpp
    REQUIRE(std::filesystem::create_directories(topLevel / "groups/one/onepkg"));
    REQUIRE(std::filesystem::create_directories(topLevel / "thirdparty"));
    REQUIRE(std::filesystem::create_directories(topLevel / "include"));

    createComponent(topLevel / "groups/one/onepkg", "foo");
    createComponent(topLevel / "groups/one/onepkg", "bar");

    REQUIRE(Test_Util::createFile(topLevel / "thirdparty/nonlakosian.cpp"));
    REQUIRE(Test_Util::createFile(topLevel / "include/nonlakosian.hpp"));

    Codethink::lvtclp::StaticCompilationDatabase cdb(
        {
            {(topLevel / "groups/one/onepkg/onepkg_foo.cpp").string(), ""},
            {(topLevel / "groups/one/onepkg/onepkg_bar.cpp").string(), ""},
            {(topLevel / "thirdparty/nonlakosian.cpp").string(), ""},
        },
        "compiler",
        {"--unrelated", "-I../include", "-I/not/a/path"},
        topLevel);

    // scan
    lvtmdb::ObjectStore memDb;

    FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {topLevel / "thirdparty"}, {});
    FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();

    // Dump to disk.
    lvtmdb::SociWriter writer;
    std::filesystem::remove("test.db");
    writer.createOrOpen("test.db");
    memDb.writeToDatabase(writer);

    // Clear the current memory
    memDb.withRWLock([&] {
        memDb.clear();
    });

    // Reload from disk.
    lvtmdb::SociReader reader;
    auto ret = memDb.readFromDatabase(reader, "test.db");
    REQUIRE(ret.has_value());

    // package groups
    auto memDblock = memDb.readOnlyLock();
    (void) memDblock;

    auto *one = memDb.getPackage("groups/one");
    REQUIRE(one);

    auto *nonLakosian = memDb.getPackage(lvtclp::ClpUtil::NON_LAKOSIAN_GROUP_NAME);
    REQUIRE(nonLakosian);

    // packages
    auto *onepkg = memDb.getPackage("groups/one/onepkg");
    REQUIRE(onepkg);
    auto lock = onepkg->readOnlyLock();
    (void) lock;
    REQUIRE(onepkg->parent());
    REQUIRE(onepkg->parent() == one);

    auto *thirdparty = memDb.getPackage("thirdparty");
    REQUIRE(thirdparty);
    auto thirdparty_lock = thirdparty->readOnlyLock();
    (void) thirdparty_lock;
    REQUIRE(thirdparty->parent() == nonLakosian);

    // lakosian files
    for (const auto& [_, comp] : memDb.components()) {
        comp->withROLock([&comp = comp] {
            std::cout << "Component name: " << comp->qualifiedName() << "\n";
        });
    }
    REQUIRE(memDb.getComponent("groups/one/onepkg/onepkg_foo"));
    REQUIRE(memDb.getComponent("groups/one/onepkg/onepkg_bar"));

    // non-lakosian files
    lvtmdb::FileObject *source = memDb.getFile("thirdparty/nonlakosian.cpp");
    REQUIRE(source);

    source->withROLock([&] {
        REQUIRE(source->package() == thirdparty);
    });

    REQUIRE(memDb.getFile("include/nonlakosian.hpp"));

    // currently the header would be in a package called "include" inside
    // "non-lakosian group". Ideally it would be in "thirdparty". Not testing
    // this now in case it is fixed later
}

class FSNonLakosianFixture {
  public:
    // public data for Catch2 magic
    const std::filesystem::path d_topLevel;
    const std::filesystem::path d_submodules;
    const std::filesystem::path d_configurationParser;
    const std::filesystem::path d_main;
    const std::filesystem::path d_qsettings;
    const std::filesystem::path d_dumpQsettings;
    const std::filesystem::path d_lvtclp;
    const std::filesystem::path d_filesystemScanner;

    FSNonLakosianFixture():
        d_topLevel(std::filesystem::temp_directory_path() / "lvtclp_testfilesystemscanner"),
        d_submodules(d_topLevel / "submodules"),
        d_configurationParser(d_submodules / "configuration-parser"),
        d_main(d_configurationParser / "main.cpp"),
        d_qsettings(d_configurationParser / "qsettings"),
        d_dumpQsettings(d_qsettings / "dump_qsettings.cpp"),
        d_lvtclp(d_topLevel / "lvtclp"),
        d_filesystemScanner(d_lvtclp / "lvtclp_filesystemScanner.cpp")
    {
        if (std::filesystem::exists(d_topLevel)) {
            REQUIRE(std::filesystem::remove_all(d_topLevel));
        }

        REQUIRE(std::filesystem::create_directories(d_configurationParser));
        REQUIRE(Test_Util::createFile(d_main));

        REQUIRE(std::filesystem::create_directories(d_qsettings));
        REQUIRE(Test_Util::createFile(d_dumpQsettings));

        REQUIRE(std::filesystem::create_directories(d_lvtclp));
        REQUIRE(Test_Util::createFile(d_filesystemScanner));
    }

    ~FSNonLakosianFixture()
    {
        REQUIRE(std::filesystem::remove_all(d_topLevel));
    }
};

TEST_CASE_METHOD(FSNonLakosianFixture, "Non-lakosian extra levels of hierarchy")
{
    StaticCompilationDatabase cmds(
        {{d_main.string(), ""}, {d_dumpQsettings.string(), ""}, {d_filesystemScanner.string(), ""}},
        "placeholder-g++",
        {},
        d_topLevel);

    Tool tool(d_topLevel, cmds, ":memory:");
    // regression test: filsystem scanner should not crash
    REQUIRE(tool.runPhysical());
}

TEST_CASE_METHOD(FilesystemScannerFixture, "Semantic packing")
{
    createTestEnv(topLevel);
    auto semRulesPath = topLevel / "semrules";
    REQUIRE(std::filesystem::create_directories(semRulesPath));

    auto filePath = semRulesPath / "myrules.py";
    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    }

    auto scriptStream = std::ofstream(filePath.string());
    const std::string SCRIPT_CONTENTS =
        "\ndef accept(path):"
        "\n    return True"
        "\n"
        "\ndef process(path, addPkg):"
        "\n    if 'groups' in path:"
        "\n        addPkg('MyPkg', 'MyPkgGrp', 'MyRepo', None)"
        "\n        return 'MyPkg'"
        "\n    else:"
        "\n        addPkg('MyStandalone', None, 'MyRepo', None)"
        "\n        return 'MyStandalone'"
        "\n";
    scriptStream << SCRIPT_CONTENTS;
    scriptStream.close();

    REQUIRE(std::ifstream{filePath.string()}.good());
    REQUIRE(qputenv("SEMRULES_PATH", semRulesPath.string().c_str()));

    Codethink::lvtclp::StaticCompilationDatabase cdb(
        {
            {(topLevel / "groups/one/onepkg/onepkg_foo.cpp").string(), ""},
            {(topLevel / "groups/one/onepkg/onepkg_bar.cpp").string(), ""},
            {(topLevel / "groups/two/twofoo/twofoo_component.cpp").string(), ""},
            {(topLevel / "groups/two/twobar/twobar_component.cpp").string(), ""},
            {(topLevel / "standalones/s_pkgtst/s_pkgtst_somecmp.cpp").string(), ""},
        },
        "",
        {},
        topLevel);

    lvtmdb::ObjectStore memDb;

    FilesystemScanner scanner(memDb, topLevel, cdb, messageCallback, false, {}, {});
    FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();
    auto lock = memDb.readOnlyLock();

    lvtmdb::RepositoryObject *repo = memDb.getRepository("MyRepo");

    REQUIRE(repo);
    repo->withROLock([&] {
        REQUIRE(repo->children().size() == 3);
    });

    // lakosian files
    for (const auto& [_, pkg] : memDb.packages()) {
        pkg->withROLock([&pkg = pkg] {
            std::cout << "Component name: " << pkg->qualifiedName() << "\n";
        });
    }

    lvtmdb::PackageObject *myPkg = memDb.getPackage("MyPkg");

    myPkg->withROLock([&] {
        REQUIRE(myPkg->qualifiedName() == "MyPkg");
        REQUIRE(myPkg->components().size() == 4);

        for (auto *comp : myPkg->components()) {
            comp->withROLock([&] {
                auto qname = comp->qualifiedName();
                INFO(qname);
                REQUIRE((qname == "groups/one/onepkg/onepkg_foo" || qname == "groups/one/onepkg/onepkg_bar"
                         || qname == "groups/two/twofoo/twofoo_component"
                         || qname == "groups/two/twobar/twobar_component"
                         || qname == "standalones/s_pkgtst/s_pkgtst_somecmp"));
            });
        }
    });

    lvtmdb::PackageObject *myStandalones = memDb.getPackage("MyStandalone");
    myStandalones->withROLock([&] {
        REQUIRE(myStandalones->qualifiedName() == "MyStandalone");
        REQUIRE(myStandalones->components().size() == 1);
        for (lvtmdb::ComponentObject *comp : myStandalones->components()) {
            comp->withROLock([&] {
                auto qname = comp->qualifiedName();
                INFO(qname);
                REQUIRE((qname == "standalones/s_pkgtst/s_pkgtst_somecmp"));
            });
        }
    });

    REQUIRE(qunsetenv("SEMRULES_PATH"));
}
