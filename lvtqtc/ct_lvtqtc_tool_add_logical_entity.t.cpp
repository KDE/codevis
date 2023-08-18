// ct_lvtqtc_tool_add_logical_entity.t.cpp                               -*-C++-*-

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

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_logicalentity.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_tool_add_logical_entity.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <QComboBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtprj;

TEST_CASE_METHOD(QTApplicationFixture, "Add logical entity")
{
    auto tmpDir = TmpDir{"add_log_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto projectFileForTesting = ProjectFileForTesting{};
    auto graphicsView = GraphicsView{nodeStorage, projectFileForTesting};
    graphicsView.setColorManagement(std::make_shared<ColorManagement>(false));
    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr).value();
    nodeStorage.addComponent("pkgcmp", "pkg/pkgcmp", pkg).expect("Unexpected error trying to create component");

    auto findItemOnGV = [&](const std::string& name) {
        auto allItems = graphicsView.allItemsByType<LakosEntity>();
        auto *item = (*std::find_if(allItems.begin(), allItems.end(), [&name](auto *e) {
            return e->name() == name;
        }));
        assert(item);
        return item;
    };

    auto createUDTUsingTool = [&](LakosEntity *parent, const std::string& name, const std::string& kind) {
        auto tool = ToolAddLogicalEntity{&graphicsView, nodeStorage};
        auto *inputDialog = tool.inputDialog();
        inputDialog->overrideExec([inputDialog, name, kind] {
            inputDialog->findChild<QLineEdit *>("name")->setText(QString::fromStdString(name));
            inputDialog->findChild<QComboBox *>("kind")->setCurrentText(QString::fromStdString(kind));
            return true;
        });

        auto pos = graphicsView.mapFromScene(parent->getTextPos());
        auto e = QMouseEvent{QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier};
        tool.mousePressEvent(&e);
    };

    auto *pkgcmp = findItemOnGV("pkgcmp");
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "pkgcmp::Klass") == nullptr);
    REQUIRE(graphicsView.allItemsByType<LogicalEntity>().empty());
    REQUIRE(pkgcmp->lakosEntities().empty());
    createUDTUsingTool(pkgcmp, "Klass", "Class");
    REQUIRE(pkgcmp->lakosEntities().size() == 1);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "pkgcmp::Klass"));
    REQUIRE(graphicsView.allItemsByType<LogicalEntity>().size() == 1);

    auto *klass = findItemOnGV("Klass");
    REQUIRE(klass->lakosEntities().empty());
    createUDTUsingTool(findItemOnGV("Klass"), "InnerKlass", "Class");
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "Klass::InnerKlass"));
    REQUIRE(graphicsView.allItemsByType<LogicalEntity>().size() == 2);
    REQUIRE(klass->lakosEntities().size() == 1);
}
