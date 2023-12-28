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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Auto Add Edge From Components")
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

    clickOn(Sidebar::ManipulationTool::NewComponent{"abcxyz_comp1"});
    clickOn(CurrentGraph{point});
    QTest::qWait(1000);

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{point.x() + 200, point.y() + 200});
    QTest::qWait(1000);
    point = findElementTopLeftPosition("def");

    clickOn(Sidebar::ManipulationTool::NewPackage{"defg"});
    clickOn(CurrentGraph{point});
    QTest::qWait(1000);
    point = findElementTopLeftPosition("def/defg");

    clickOn(Sidebar::ManipulationTool::NewComponent{"defg_comp1"});
    clickOn(CurrentGraph{point});
    QTest::qWait(1000);

    auto element1 = findElementTopLeftPosition("abc/abcxyz/abcxyz_comp1");
    auto element2 = findElementTopLeftPosition("def/defg/defg_comp1");

    clickOn(Sidebar::ManipulationTool::AddDependency{});
    clickOn(CurrentGraph{element1});
    clickOn(CurrentGraph{element2});

    QTest::qWait(100);

    LakosEntity *abc = findElement("abc");
    LakosEntity *def = findElement("def");
    LakosEntity *abcd = findElement("abc/abcxyz");
    LakosEntity *defg = findElement("def/defg");
    LakosEntity *abc_comp1 = findElement("abc/abcxyz/abcxyz_comp1");
    LakosEntity *def_comp1 = findElement("def/defg/defg_comp1");

    REQUIRE(abc->internalNode()->hasProvider(def->internalNode()));
    REQUIRE(abcd->internalNode()->hasProvider(defg->internalNode()));
    REQUIRE(abc_comp1->internalNode()->hasProvider(def_comp1->internalNode()));
    REQUIRE(abc->edgesCollection().size() == 1);
    REQUIRE(abc->edgesCollection()[0]->relations().size() == 1);

    ctrlZ();
    QTest::qWait(100);

    REQUIRE_FALSE(abc->internalNode()->hasProvider(def->internalNode()));
    REQUIRE_FALSE(abcd->internalNode()->hasProvider(defg->internalNode()));
    REQUIRE_FALSE(abc_comp1->internalNode()->hasProvider(def_comp1->internalNode()));

    ctrlShiftZ();
    QTest::qWait(100);

    REQUIRE(abc->internalNode()->hasProvider(def->internalNode()));
    REQUIRE(abcd->internalNode()->hasProvider(defg->internalNode()));
    REQUIRE(abc_comp1->internalNode()->hasProvider(def_comp1->internalNode()));
}