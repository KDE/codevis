// mainwindow_auto_add_edges_from_packages.t.cpp                                      -*-C++-*-

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
TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Auto Add Edges From Packages")
{
    Preferences::self()->useDefaults(true);

    clickOn(Menubar::File::NewProject{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{100, 400});
    QTest::qWait(100);

    clickOn(Sidebar::VisualizationTool::ResetZoom{});

    auto point = findElementTopLeftPosition("abc");
    clickOn(Sidebar::ManipulationTool::NewPackage{"abcxyz"});
    clickOn(CurrentGraph{point});
    QTest::qWait(400);

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{800, 400});
    QTest::qWait(100);

    point = findElementTopLeftPosition("def");

    clickOn(Sidebar::ManipulationTool::NewPackage{"defg"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    auto element1 = findElementTopLeftPosition("abc/abcxyz");
    clickOn(CurrentGraph{element1});

    auto element2 = findElementTopLeftPosition("def/defg");
    clickOn(CurrentGraph{element2});

    QTest::qWait(100);

    LakosEntity *abc = findElement("abc");
    LakosEntity *def = findElement("def");
    LakosEntity *abcd = findElement("abc/abcxyz");
    LakosEntity *defg = findElement("def/defg");

    REQUIRE(abc->internalNode()->hasProvider(def->internalNode()));
    REQUIRE(abcd->internalNode()->hasProvider(defg->internalNode()));

    REQUIRE(abc->edgesCollection().size() == 1);
    REQUIRE(abc->edgesCollection()[0]->relations().size() == 1);

    ctrlZ();
    QTest::qWait(100);

    REQUIRE_FALSE(abcd->internalNode()->hasProvider(defg->internalNode()));
    REQUIRE_FALSE(abc->internalNode()->hasProvider(def->internalNode()));

    ctrlShiftZ();
    QTest::qWait(100);

    REQUIRE(abc->internalNode()->hasProvider(def->internalNode()));
    REQUIRE(abcd->internalNode()->hasProvider(defg->internalNode()));
}
