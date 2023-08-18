// ct_lvtldr_alloweddependencyloader.t.cpp                            -*-C++-*-

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

#include <catch2-local-includes.h>

#include <ct_lvtldr_alloweddependencyloader.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>

#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_writer.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

using namespace Codethink::lvtldr;
using Codethink::lvtmdb::ObjectStore;
using Codethink::lvtmdb::SociWriter;

TEST_CASE("Load Allowed Dependencies")
{
    auto tmpDir = TmpDir{"load_allowed_deps"};
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(tmpDir.path() / "codedb.db");

    auto *grp = nodeStorage.addPackage("grp", "groups/grp", nullptr).value();
    auto *pkg = nodeStorage.addPackage("pkg", "groups/grp/pkg", grp).value();
    nodeStorage.addComponent("component", "groups/pkg/component", pkg).expect("");
    auto *pkb = nodeStorage.addPackage("pkb", "groups/grp/pkb", grp).value();
    nodeStorage.addComponent("component2", "groups/pkg/component2", pkb).expect("");

    auto srcDir = TmpDir{"src"};
    (void) srcDir.createTextFile("groups/grp/pkg/package/pkg.dep", "pkb");
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(pkg)->hasAllowedDependency(pkb));
    auto result = loadAllowedDependenciesFromDepFile(nodeStorage, srcDir.path());
    if (result.has_error()) {
        FAIL("Enum code: " << static_cast<int>(result.error().kind));
    }
    REQUIRE(dynamic_cast<PackageNode *>(pkg)->hasAllowedDependency(pkb));
}

TEST_CASE("Load Allowed Dependencies for different pkg groups")
{
    auto tmpDir = TmpDir{"load_allowed_diff_pkggrp"};
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(tmpDir.path() / "codedb.db");

    auto *grpa = nodeStorage.addPackage("grpa", "groups/grpa", nullptr).value();
    auto *pkga = nodeStorage.addPackage("pkga", "groups/grpa/pkga", grpa).value();
    nodeStorage.addComponent("component", "groups/grpa/pkga/component", pkga).expect("");
    auto *grpb = nodeStorage.addPackage("grpb", "groups/grpb", nullptr).value();
    auto *pkgb = nodeStorage.addPackage("pkgb", "groups/grpb/pkgb", grpb).value();
    nodeStorage.addComponent("component", "groups/grpb/pkgb/component", pkgb).expect("");
    auto *pkgstd = nodeStorage.addPackage("pkgstd", "standalones/pkgstd", nullptr).value();
    nodeStorage.addComponent("component", "standalones/pkgstd/component", pkgstd).expect("");

    auto srcDir = TmpDir{"src"};
    (void) srcDir.createTextFile("groups/grpa/group/grpa.dep", "grpb\npkgstd");
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(grpa)->hasAllowedDependency(grpb));
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(grpa)->hasAllowedDependency(pkgstd));
    auto result = loadAllowedDependenciesFromDepFile(nodeStorage, srcDir.path());
    if (result.has_error()) {
        FAIL("Enum code: " << static_cast<int>(result.error().kind));
    }
    REQUIRE(dynamic_cast<PackageNode *>(grpa)->hasAllowedDependency(grpb));
    REQUIRE(dynamic_cast<PackageNode *>(grpa)->hasAllowedDependency(pkgstd));

    REQUIRE_FALSE(dynamic_cast<PackageNode *>(grpb)->hasAllowedDependency(grpa));
}
