// ct_lvtqtc_tool_add_package.t.cpp                               -*-C++-*-

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
#include <catch2/catch.hpp>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_tool_add_package.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <QLineEdit>
#include <QMouseEvent>
#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtprj;
using namespace Codethink::lvtldr;

TEST_CASE_METHOD(QTApplicationFixture, "Add package")
{
    auto tmpDir = TmpDir{"add_pkg_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto projectFileForTesting = ProjectFileForTesting{};
    auto graphicsView = GraphicsView{nodeStorage, projectFileForTesting};
    graphicsView.setColorManagement(std::make_shared<ColorManagement>(false));
    auto addPkgTool = ToolAddPackage{&graphicsView, nodeStorage};

    auto e = QMouseEvent{QEvent::MouseButtonPress, QPointF{0, 0}, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier};

    // Setup the internal dialog on the mouse event to create
    // a name called "pkg".
    // multiple uses should just change the text on nameLineEdit.
    auto *inputDialog = addPkgTool.inputDialog();
    inputDialog->overrideExec([inputDialog] {
        auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
        REQUIRE(nameLineEdit->text() == "");
        nameLineEdit->setText("pkg");
        return true;
    });

    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "pkg") == nullptr);
    addPkgTool.mouseReleaseEvent(&e);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "pkg") != nullptr);
}

TEST_CASE_METHOD(QTApplicationFixture, "Lakosian Name Verification Test")
{
    const QString header = QObject::tr("BDE Guidelines Enforced: ");

    bool hasParent = false;
    std::string parentName;
    std::string name;

    // No name, error - package name is too short.
    auto result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what == header + QObject::tr("top level packages must be three letters long."));

    // No name, error - package name is too long.
    name = "abcdehkjerht";
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what == header + QObject::tr("top level packages must be three letters long."));

    // three letter name for toplevel package (without parent) == success.
    name = "abc";
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == false);

    hasParent = true;
    parentName = "abc";
    name = "";

    // Full name for the package name is parentName + name. the name needs to be at least 1 letter long, no more than 5
    // letters.
    result = LakosianNameRules::checkName(hasParent, "_def", parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what == header + QObject::tr("Package names should not contain underscore"));

    // Full name for the package name is parentName + name. the name needs to be at least 1 letter long, no more than 5
    // letters.
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what
            == header + QObject::tr("Name too short, must be at least four letters (parent package(3) + name(1-5))"));

    name = "abcdefghi";
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what
            == header + QObject::tr("Name too long, must be at most eight letters (parent package(3) + name(1-5))"));

    // names must always start with the parent's name. soo if I have a parent `bal`, `balb` is an accepted child.
    name = "defl";
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == true);
    REQUIRE(result.error().what
            == header
                + QObject::tr("the package name must start with the parent's name (%1)")
                      .arg(QString::fromStdString(parentName)));

    // package name follows the convention. full package: abc/abcde
    name = "abcde";
    result = LakosianNameRules::checkName(hasParent, name, parentName);
    REQUIRE(result.has_error() == false);
}
