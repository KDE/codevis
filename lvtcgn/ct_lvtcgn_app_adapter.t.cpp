// ct_lvtcgn_app_adapter.t.cpp                                       -*-C++-*-

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

#include <ct_lvtcgn_app_adapter.h>

#include <catch2-local-includes.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <qcoreapplication.h>
#include <test-project-paths.h>

using namespace Codethink::lvtcgn::app;
using namespace Codethink::lvtcgn::mdl;
using namespace Codethink::lvtldr;

TEST_CASE("Code generation adapter")
{
    int argc = 0;
    char **argv = nullptr;
    QCoreApplication app(argc, argv);

    auto tmpDir = TmpDir{"codegen_adapter"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *pkgA = ns.addPackage("pkgA", "pkgA").value();
    (void) ns.addComponent("componentA", "pkgA::componentA", pkgA);
    auto *pkgB = ns.addPackage("pkgB", "pkgB").value();
    (void) ns.addPackage("pkgBa", "pkgB/pkgBa", pkgB);
    (void) ns.addPackage("non-lakosian group", "non-lakosian group");

    auto dataProvider = NodeStorageDataProvider{ns};
    REQUIRE(dataProvider.numberOfPhysicalEntities() == 5);

    auto pkgs = dataProvider.topLevelEntities();
    REQUIRE(pkgs.size() == 3);

    auto getTopLvlEntity = [&pkgs](auto const& name) -> IPhysicalEntityInfo * {
        return *std::find_if(pkgs.begin(), pkgs.end(), [&name](auto *p) {
            return p->name() == name;
        });
    };

    auto *pkg0 = getTopLvlEntity("non-lakosian group");
    REQUIRE(pkg0->name() == "non-lakosian group");
    // In this particular case, the non-lakosian group is simply a package because it is empty, and falls into
    // this specific rule.
    REQUIRE(pkg0->type() == "Package");
    REQUIRE_FALSE(pkg0->parent());
    REQUIRE(pkg0->children().empty());
    // non-lakosian groups are expected to be unselected for code generation
    REQUIRE(pkg0->selectedForCodeGeneration() == false);

    auto *pkg1 = getTopLvlEntity("pkgA");
    REQUIRE(pkg1->name() == "pkgA");
    REQUIRE(pkg1->type() == "Package");
    REQUIRE_FALSE(pkg1->parent());
    REQUIRE(pkg1->selectedForCodeGeneration() == true);
    REQUIRE(pkg1->children().size() == 1);

    auto *componentA = pkg1->children()[0];
    REQUIRE(componentA->name() == "componentA");
    REQUIRE(componentA->type() == "Component");
    REQUIRE(componentA->parent()->name() == "pkgA");
    REQUIRE(componentA->children().empty());
    REQUIRE(componentA->fwdDependencies().empty());

    auto *pkg2 = getTopLvlEntity("pkgB");
    REQUIRE(pkg2->name() == "pkgB");
    REQUIRE(pkg2->type() == "PackageGroup");
    REQUIRE_FALSE(pkg2->parent());
    REQUIRE(pkg2->children().size() == 1);
    REQUIRE(pkg2->selectedForCodeGeneration() == true);

    auto *pkgBa = pkg2->children()[0];
    REQUIRE(pkgBa->name() == "pkgBa");
    REQUIRE(pkgBa->type() == "Package");
    REQUIRE(pkgBa->parent()->name() == "pkgB");
    REQUIRE(pkgBa->children().empty());
    REQUIRE(pkgBa->selectedForCodeGeneration() == true);
}

TEST_CASE("CMake code generation script")
{
    int argc = 0;
    char **argv = nullptr;
    QCoreApplication app(argc, argv);

    auto cmakeGeneratorPath = std::string(LAKOSDIAGRAM_PYSCRIPTS_PATH) + "/cmake/codegenerator.js";
    SECTION("Basic package project without package groups")
    {
        auto tmpDir = TmpDir{"basic_pkg_no_grp"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkgA = ns.addPackage("pkgA", "pkgA").value();
        auto *componentA = ns.addComponent("componentA", "pkgA/componentA", pkgA).value();
        auto *pkgB = ns.addPackage("pkgB", "pkgB").value();
        auto *componentB = ns.addComponent("componentB", "pkgB/componentB", pkgB).value();
        ns.addPhysicalDependency(pkgA, pkgB).expect("Unexpected error on relationship pkgA->pkgB");
        ns.addPhysicalDependency(componentA, componentB)
            .expect("Unexpected error on relationship componentA->componentB");

        auto outputDir = TmpDir{"cmake_out_dir"};
        auto dataProvider = NodeStorageDataProvider{ns};
        auto result = CodeGeneration::generateCodeFromjS(QString::fromStdString(cmakeGeneratorPath),
                                                         QString::fromStdString(outputDir.path().string()),
                                                         dataProvider);
        if (result.has_error()) {
            FAIL("ERROR MESSAGE: " + result.error().message);
        } else {
            qDebug() << "GenerateCodeFromJS Run Correctly";
        }

        REQUIRE(std::filesystem::exists(outputDir.path() / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgA" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgA" / "componentA.cpp"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgA" / "componentA.h"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgB" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgB" / "componentB.cpp"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgB" / "componentB.h"));
    }

    SECTION("Generate code with package groups")
    {
        auto tmpDir = TmpDir{"codegen_with_grps"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkgGrpA = ns.addPackage("pkgGrpA", "pkgGrpA").value();
        auto *pkgA = ns.addPackage("pkgA", "pkgGrpA/pkgA", pkgGrpA).value();
        auto *componentA = ns.addComponent("componentA", "pkgGrpA/pkgA/componentA", pkgA).value();
        auto *pkgGrpB = ns.addPackage("pkgGrpB", "pkgGrpB").value();
        auto *pkgB = ns.addPackage("pkgB", "pkgGrpB/pkgB", pkgGrpB).value();
        auto *componentB = ns.addComponent("componentB", "pkgGrpB/pkgB/componentB", pkgB).value();

        ns.addPhysicalDependency(pkgGrpA, pkgGrpB).expect("Unexpected error on relationship pkgGrpA->pkgGrpB");
        ns.addPhysicalDependency(pkgA, pkgB).expect("Unexpected error on relationship pkgA->pkgB");
        ns.addPhysicalDependency(componentA, componentB)
            .expect("Unexpected error on relationship componentA->componentB");

        auto outputDir = TmpDir{"cmake_out_dir"};
        auto dataProvider = NodeStorageDataProvider{ns};
        auto result = CodeGeneration::generateCodeFromjS(QString::fromStdString(cmakeGeneratorPath),
                                                         QString::fromStdString(outputDir.path().string()),
                                                         dataProvider);
        if (result.has_error()) {
            FAIL("ERROR MESSAGE: " + result.error().message);
        }

        REQUIRE(std::filesystem::exists(outputDir.path() / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpA" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpA" / "pkgA" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpA" / "pkgA" / "componentA.cpp"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpA" / "pkgA" / "componentA.h"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpB" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpB" / "pkgB" / "CMakeLists.txt"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpB" / "pkgB" / "componentB.cpp"));
        REQUIRE(std::filesystem::exists(outputDir.path() / "pkgGrpB" / "pkgB" / "componentB.h"));
    }
}
