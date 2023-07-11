// ct_lvtqtw_tool_reparent_entity.t.cpp                                                                        -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_tool_reparent_entity.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Reparent entity")
{
    auto tmpDir = TmpDir{"reparent_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    GraphicsViewWrapperForTesting gv{nodeStorage};
    gv.show();

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *b = nodeStorage.addPackage("b", "b").value();
    auto *aa = nodeStorage.addComponent("aa", "aa", a).value();
    auto *bb = nodeStorage.addComponent("bb", "bb", b).value();

    gv.moveEntityTo(a->uid(), {0, 0});
    gv.moveEntityTo(b->uid(), {100, 0});

    auto tool = ToolReparentEntity{&gv, nodeStorage};
    gv.setCurrentTool(&tool);
    tool.activate();

    // Basic tool usage
    REQUIRE(gv.countEntityChildren(a->uid()) == 1);
    REQUIRE(gv.countEntityChildren(b->uid()) == 1);
    mouseMoveTo(tool, gv.getEntityPosition(aa->uid()));
    mousePressAt(tool, gv.getEntityPosition(aa->uid()));
    mouseMoveTo(tool, gv.getEntityPosition(b->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(b->uid()));
    REQUIRE(gv.countEntityChildren(a->uid()) == 0);
    REQUIRE(gv.countEntityChildren(b->uid()) == 2);
    mouseMoveTo(tool, gv.getEntityPosition(bb->uid()));
    mousePressAt(tool, gv.getEntityPosition(bb->uid()));
    mouseMoveTo(tool, gv.getEntityPosition(a->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(a->uid()));
    REQUIRE(gv.countEntityChildren(a->uid()) == 1);
    REQUIRE(gv.countEntityChildren(b->uid()) == 1);
}

TEST_CASE_METHOD(QTApplicationFixture, "Reparent entity multiple GraphicsScenes")
{
    auto tmpDir = TmpDir{"reparent_multiple_gs"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    GraphicsViewWrapperForTesting gv{nodeStorage};
    gv.show();

    auto *b = nodeStorage.addPackage("b", "b").value();
    auto *bb = nodeStorage.addComponent("bb", "bb", b).value();

    // gv2 will only have visibility of 'a' and 'aa'
    GraphicsViewWrapperForTesting gv2{nodeStorage};
    gv2.show();

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *aa = nodeStorage.addComponent("aa", "aa", a).value();

    gv.moveEntityTo(a->uid(), {0, 0});
    gv.moveEntityTo(b->uid(), {100, 0});

    auto tool = ToolReparentEntity{&gv, nodeStorage};
    gv.setCurrentTool(&tool);
    tool.activate();

    // Basic tool usage
    REQUIRE(gv.countEntityChildren(a->uid()) == 1);
    REQUIRE(gv.countEntityChildren(b->uid()) == 1);
    REQUIRE(gv2.countEntityChildren(a->uid()) == 1);
    mouseMoveTo(tool, gv.getEntityPosition(aa->uid()));
    mousePressAt(tool, gv.getEntityPosition(aa->uid()));
    mouseMoveTo(tool, gv.getEntityPosition(b->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(b->uid()));
    REQUIRE(gv2.countEntityChildren(a->uid()) == 0);
    REQUIRE(gv.countEntityChildren(a->uid()) == 0);
    REQUIRE(gv.countEntityChildren(b->uid()) == 2);
    mouseMoveTo(tool, gv.getEntityPosition(bb->uid()));
    mousePressAt(tool, gv.getEntityPosition(bb->uid()));
    mouseMoveTo(tool, gv.getEntityPosition(a->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(a->uid()));
    REQUIRE(gv.countEntityChildren(a->uid()) == 1);
    REQUIRE(gv.countEntityChildren(b->uid()) == 1);

    // Because `bb` was not in the gv2 scene in the first place, he won't be shown after being moved to the `a` package.
    // The user would have to drag `bb` manually. This is the expected behavior.
    REQUIRE(gv2.countEntityChildren(a->uid()) == 0);
}
