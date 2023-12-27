// mainwindow_basic_application_workflow.t.cpp                                      -*-C++-*-

/* Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
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

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosrelation.h>
#include <ct_lvttst_fixture_qt.h>

#include <apptesting_fixture.h>
#include <catch2-local-includes.h>
#include <variant>

#include <preferences.h>

// in a header
Q_DECLARE_LOGGING_CATEGORY(LogTest)

// in one source file
Q_LOGGING_CATEGORY(LogTest, "log.test")

using namespace Codethink::lvtldr;

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Basic application workflow")
{
    Preferences::self()->useDefaults(true);

    REQUIRE(isShowingWelcomePage());
    REQUIRE_FALSE(isShowingGraphPage());
    clickOn(Menubar::File::NewProject{});
    REQUIRE_FALSE(isShowingWelcomePage());
    REQUIRE(isShowingGraphPage());

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{100, 400});
    QTest::qWait(100);

    // position will change because of the layout algorithm,
    // so we need to request from the view.
    clickOn(Sidebar::ManipulationTool::NewPackage{"abcxyz"});
    const auto pos1 = findElementTopLeftPosition("abc");
    clickOn(CurrentGraph{pos1});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewComponent{"abcxyz_comp1"});
    const auto pos2 = findElementTopLeftPosition("abc/abcxyz");
    clickOn(CurrentGraph{pos2});

    // Wait to make sure animations finished
    QTest::qWait(200);

    ctrlZ();
    ctrlShiftZ();

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{600, 400});
    QTest::qWait(100);

    // position will change because of the layout algorithm,
    // so we need to request from the view.
    const auto pos3 = findElementTopLeftPosition("def");

    clickOn(Sidebar::ManipulationTool::NewPackage{"defxyz"});
    clickOn(CurrentGraph{pos3});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewComponent{"defxyz_comp1"});
    const auto pos4 = findElementTopLeftPosition("def/defxyz");
    clickOn(CurrentGraph{pos4});

    // Wait to make sure animations finished
    QTest::qWait(200);

    ctrlZ();
    ctrlShiftZ();

    // Stress test
    ctrlZ();
    ctrlZ();
    ctrlZ();
    ctrlZ();
    ctrlShiftZ();
    ctrlShiftZ();
    ctrlShiftZ();
    ctrlShiftZ();

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    auto posAbc = findElementTopLeftPosition("abc");
    clickOn(CurrentGraph{posAbc});
    auto posDef = findElementTopLeftPosition("def");
    clickOn(CurrentGraph{posDef});

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    auto posAXyz = findElementTopLeftPosition("abc/abcxyz");
    clickOn(CurrentGraph{posAXyz});

    auto posDXyz = findElementTopLeftPosition("def/defxyz");
    clickOn(CurrentGraph{posDXyz});

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    auto element1 = findElementTopLeftPosition("abc/abcxyz/abcxyz_comp1");
    clickOn(CurrentGraph{element1});

    auto element2 = findElementTopLeftPosition("def/defxyz/defxyz_comp1");
    clickOn(CurrentGraph{element2});

    clickOn(Sidebar::ManipulationTool::NewLogicalEntity{"klass", "Class"});
    clickOn(CurrentGraph{element1});

    QTest::qWait(100);
    ctrlZ();
    QTest::qWait(100);
    ctrlShiftZ();
    QTest::qWait(100);

    // Wait just for be human-noticeable
    QTest::qWait(100);
    clickOn(Menubar::File::CloseProject{});
    REQUIRE(isShowingWelcomePage());
    REQUIRE_FALSE(isShowingGraphPage());
}
