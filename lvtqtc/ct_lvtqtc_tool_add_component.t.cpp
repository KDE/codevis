// ct_lvtqtc_tool_add_component.t.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_tool_add_component.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvttst_fixture_qt.h>

#include <catch2-local-includes.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <QLineEdit>
#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Add component")
{
    auto tmpDir = TmpDir{"add_component"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, gv.scene()).value();

    auto tool = ToolAddComponent{&gv, nodeStorage};
    auto *inputDialog = tool.inputDialog();

    inputDialog->overrideExec([inputDialog] {
        auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
        REQUIRE(nameLineEdit->text() == "pkg_");
        nameLineEdit->setText("pkg_comp1");
        return true;
    });

    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "pkg/pkg_comp1") == nullptr);
    REQUIRE(gs->allEntities().size() == 1);

    mousePressAt(tool, gv.getEntityPosition(pkg->uid()));

    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "pkg/pkg_comp1") != nullptr);
    REQUIRE(gs->allEntities().size() == 2);
}
