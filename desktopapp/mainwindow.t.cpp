// mainwindow.t.cpp                                      -*-C++-*-

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

// TODO: Resolve the test below after task 651 is fixed.

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Loading Project Leads to crash")
{
    Preferences::loadDefaults();

    // We need a default tab widget to display elements on the view.
    REQUIRE(hasDefaultTabWidget());

    // We need to lock for navigation to add new elements on the view.
    clickOn(Sidebar::ToggleApplicationMode{});

    const bool isOpen = window().openProjectFromPath("testcrash.lks");
    REQUIRE(isOpen);

    QTest::qWait(100);

    clickOn(PackageTreeView::Package{"abc"});
    QTest::qWait(100);

    // if this passes, we are not crashing.
}

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Disable Tools on Lock Navigation")
{
    Preferences::loadDefaults();

    REQUIRE(isShowingWelcomePage());
    clickOn(Menubar::File::NewProject{});
    REQUIRE(isShowingGraphPage());

    // if this passes, we are not crashing.
    // We need to lock for navigation to add new elements on the view.
    clickOn(Sidebar::ToggleApplicationMode{});
    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(Sidebar::ToggleApplicationMode{});

    REQUIRE_FALSE(isAnyToolSelected());
}

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Basic application workflow")
{
    Preferences::loadDefaults();

    REQUIRE(isShowingWelcomePage());
    REQUIRE_FALSE(isShowingGraphPage());
    clickOn(Menubar::File::NewProject{});
    REQUIRE_FALSE(isShowingWelcomePage());
    REQUIRE(isShowingGraphPage());

    // We need to lock for navigation to add new elements on the view.
    clickOn(Sidebar::ToggleApplicationMode{});

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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Auto Add Edges From Packages")
{
    Preferences::loadDefaults();

    clickOn(Menubar::File::NewProject{});
    clickOn(Sidebar::ToggleApplicationMode{});

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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Auto Add Edge From Components")
{
    Preferences::loadDefaults();

    clickOn(Menubar::File::NewProject{});
    clickOn(Sidebar::ToggleApplicationMode{});

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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Show Error On Add Edge Failure")
{
    Preferences::loadDefaults();

    clickOn(Menubar::File::NewProject{});
    clickOn(Sidebar::ToggleApplicationMode{});

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

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Test Auto Save")
{
    // Disable the timer for the test. Sequential auto tests do not handle QTimers well.
    Preferences::loadDefaults();
    Preferences::document()->setAutoSaveBackupIntervalMsecs(0);

    clickOn(Menubar::File::NewProject{});

    // first, let's delete any possible backup file from pre-existing runs.
    auto backupPath = window().projectFile().backupPath();
    std::filesystem::remove(backupPath);
    REQUIRE_FALSE(std::filesystem::exists(backupPath));

    clickOn(Sidebar::ToggleApplicationMode{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{100, 400});

    REQUIRE(std::filesystem::exists(backupPath.string()));

    // let's clean our trash.
    std::filesystem::remove(backupPath);
}

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Auto Add Edge From Classes")
{
    Preferences::loadDefaults();

    clickOn(Menubar::File::NewProject{});
    clickOn(Sidebar::ToggleApplicationMode{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{0, 0});
    QTest::qWait(100);

    auto point = findElementTopLeftPosition("abc");
    clickOn(Sidebar::ManipulationTool::NewPackage{"abcxyz"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    point = findElementTopLeftPosition("abc/abcxyz");

    clickOn(Sidebar::ManipulationTool::NewComponent{"abcxyz_comp1"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    point = findElementTopLeftPosition("abc/abcxyz/abcxyz_comp1");

    clickOn(Sidebar::ManipulationTool::NewLogicalEntity{"first_class", "ClassA"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{500, 400});
    QTest::qWait(100);

    point = findElementTopLeftPosition("def");

    clickOn(Sidebar::ManipulationTool::NewPackage{"defg"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    point = findElementTopLeftPosition("def/defg");

    clickOn(Sidebar::ManipulationTool::NewComponent{"defg_comp1"});
    clickOn(CurrentGraph{point});
    QTest::qWait(1000);

    point = findElementTopLeftPosition("def/defg/defg_comp1");

    clickOn(Sidebar::ManipulationTool::NewLogicalEntity{"second_class", "ClassB"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::AddIsA{});

    auto element1 = findElementTopLeftPosition("abcxyz_comp1::first_class");
    clickOn(CurrentGraph{element1});
    QTest::qWait(100);

    auto element2 = findElementTopLeftPosition("defg_comp1::second_class");
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
    QTest::qWait(100);

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

// TODO [#616]: Review test flaky on Windows
#ifndef _WIN32
TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Correctly Show Relationship After Actions")
{
    Preferences::loadDefaults();

    clickOn(Menubar::File::NewProject{});
    clickOn(Sidebar::ToggleApplicationMode{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{0, 0});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{300, 400});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::AddDependency{});

    auto point = findElementTopLeftPosition("abc");
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    point = findElementTopLeftPosition("def");
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    auto getRelation = [this]() -> LakosRelation * {
        auto *view = qobject_cast<GraphicsView *>(window().findChild<QGraphicsView *>());
        LakosRelation *relation = nullptr;
        for (auto *item : view->items()) {
            relation = dynamic_cast<LakosRelation *>(item);
            if (relation) {
                return relation;
            }
        }
        return nullptr;
    };

    point = findElementTopLeftPosition("abc");
    clickOn(Sidebar::ManipulationTool::NewPackage{"abcxyz"});
    clickOn(CurrentGraph{point});
    QTest::qWait(100);

    auto *entity1 = findElement("abc");

    entity1->shrink(QtcUtil::CreateUndoAction::e_No, std::nullopt, LakosEntity::RelayoutBehavior::e_RequestRelayout);
    QTest::qWait(300);
    auto *relation = getRelation();
    REQUIRE(relation);
    REQUIRE(relation->isVisible());

    entity1->expand(QtcUtil::CreateUndoAction::e_No, std::nullopt, LakosEntity::RelayoutBehavior::e_RequestRelayout);
    QTest::qWait(300);
    relation = getRelation();
    REQUIRE(relation);
    REQUIRE(relation->isVisible());

    auto *view = window().findChild<GraphicsView *>();
    auto *scene = qobject_cast<GraphicsScene *>(view->scene());
    scene->collapseSecondaryEntities();
    REQUIRE(relation);
    REQUIRE(relation->isVisible());
}
#endif

#if 0
// TODO https://bugreports.qt.io/browse/QTBUG-5232

theres no way to test a mouse press event currently in Qt5 using
sane code. let this code here and re-enable it on Qt6.

This is a good reason to start thinking a Qt6.4 port

TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Element Move")
{
    Preferences::loadDefaults();
    clickOn(Menubar::File::NewProject{});

    clickOn(Sidebar::ToggleApplicationMode{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{300, 400});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{100, 400});
    QTest::qWait(100);

    auto point = findElementTopLeftPosition("abc");
    auto element = findElement("abc");
    auto initialPos = element->pos();

    graphPressMoveRelease(point, QPoint(point.x() + 200, point.y()));
    QTest::qWait(100);

    auto resultingPos = element->pos();

    REQUIRE_FALSE(initialPos == resultingPos);
}
#endif
