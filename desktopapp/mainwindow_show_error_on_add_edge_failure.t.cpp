// mainwindow_auto_add_edges_from_components.t.cpp                                      -*-C++-*-

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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Show Error On Add Edge Failure")
{
    Preferences::self()->useDefaults(true);

    clickOn(Menubar::File::NewProject{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{100, 400});
    QTest::qWait(1000);

    clickOn(Sidebar::VisualizationTool::ResetZoom{});

    auto point = findElementTopLeftPosition("abc");
    clickOn(Sidebar::ManipulationTool::NewPackage{"abcxyz"});
    clickOn(CurrentGraph{point});
    QTest::qWait(1000);

    point = findElementTopLeftPosition("abc/abcxyz");

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{450, 450});
    QTest::qWait(1000);
    point = findElementTopLeftPosition("def");

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    auto element1 = findElementTopLeftPosition("abc/abcxyz");

    clickOn(CurrentGraph{element1});
    auto element2 = findElementTopLeftPosition("def");

    clickOn(CurrentGraph{element2});
    REQUIRE(window().currentMessage() == "Cannot create dependency on different hierarchy levels");
}
