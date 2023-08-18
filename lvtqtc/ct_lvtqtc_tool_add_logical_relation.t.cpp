// ct_lvtqtc_tool_add_logical_relation.t.cpp                               -*-C++-*-

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
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_tool_add_logical_relation.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

template<typename TOOL_TYPE>
void runTestOnTool(QTApplicationFixture *self)
{
    auto tmpDir = TmpDir{"add_log_relation"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    GraphicsViewWrapperForTesting gv{nodeStorage};
    gv.setFixedWidth(800);
    gv.setFixedHeight(600);
    auto *pkg = nodeStorage.addPackage("pkg", "pkg").value();
    auto *abc = nodeStorage.addComponent("abc", "pkg/abc", pkg).value();
    auto *Polygon = nodeStorage.addLogicalEntity("Polygon", "abc::Polygon", abc, UDTKind::Class).value();
    auto *Shape = nodeStorage.addLogicalEntity("Shape", "abc::Shape", abc, UDTKind::Class).value();
    auto *def = nodeStorage.addComponent("def", "pkg/def", pkg).value();
    auto *Square = nodeStorage.addLogicalEntity("Square", "def::Square", def, UDTKind::Class).value();
    auto *utl = nodeStorage.addPackage("utl", "utl").value();
    auto *utl_stuff = nodeStorage.addComponent("stuff", "utl/stuff", utl).value();
    auto *Util = nodeStorage.addLogicalEntity("Util", "stuff::Util", utl_stuff, UDTKind::Class).value();
    gv.show();
    qobject_cast<GraphicsScene *>(gv.scene())->runLayoutAlgorithm();

    auto tool = TOOL_TYPE{&gv, nodeStorage};
    tool.setProperty("debug", true);
    std::string lastErrorMsg;
    QObject::connect(&tool, &TOOL_TYPE::sendMessage, &tool, [&lastErrorMsg](const QString& s) {
        lastErrorMsg = s.toStdString();
        qDebug() << QString::fromStdString(lastErrorMsg);
    });

    // Using the tool an arbitrary place won't do anything (including no crashing)
    tool.activate();
    REQUIRE_FALSE(mousePressAt(tool, {500, 100}));
    mouseReleaseAt(tool, {500, 100});
    tool.deactivate();

    gv.moveEntityTo(Polygon->uid(), {0, 0});
    gv.moveEntityTo(Shape->uid(), {100, 100});

    // Basic tool usage
    tool.activate();
    REQUIRE_FALSE(Polygon->hasProvider(Shape));
    mousePressAt(tool, gv.getEntityPosition(Polygon->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Polygon->uid()));

    mousePressAt(tool, gv.getEntityPosition(Shape->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Shape->uid()));
    REQUIRE(Polygon->hasProvider(Shape));
    REQUIRE(lastErrorMsg.empty());

    // Add a relation between different packages
    nodeStorage.addPhysicalDependency(pkg, utl).expect("Unexpected error");
    nodeStorage.addPhysicalDependency(abc, utl_stuff).expect("Unexpected error");

    REQUIRE_FALSE(Shape->hasProvider(Util));
    REQUIRE_FALSE(gv.hasRelationWithId(Shape->uid(), Util->uid()));
    mousePressAt(tool, gv.getEntityPosition(Shape->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Shape->uid()));
    mousePressAt(tool, gv.getEntityPosition(Util->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Util->uid()));
    REQUIRE(Shape->hasProvider(Util));
    REQUIRE(gv.hasRelationWithId(Shape->uid(), Util->uid()));

    REQUIRE_FALSE(abc->hasProvider(def));
    REQUIRE_FALSE(Shape->hasProvider(Square));
    REQUIRE_FALSE(gv.hasRelationWithId(Shape->uid(), Square->uid()));
    mousePressAt(tool, gv.getEntityPosition(Shape->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Shape->uid()));
    mousePressAt(tool, gv.getEntityPosition(Square->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(Square->uid()));
    REQUIRE(abc->hasProvider(def));
    REQUIRE(Shape->hasProvider(Square));
    REQUIRE(gv.hasRelationWithId(Shape->uid(), Square->uid()));
}

TEST_CASE_METHOD(QTApplicationFixture, "Add logical relation")
{
    runTestOnTool<ToolAddUsesInTheInterface>(this);
    runTestOnTool<ToolAddUsesInTheImplementation>(this);
    runTestOnTool<ToolAddIsARelation>(this);
}
