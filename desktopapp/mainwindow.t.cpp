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

// TODO [#616]: Review test flaky on Windows
TEST_CASE_METHOD(CodeVisApplicationTestFixture, "Correctly Show Relationship After Actions")
{
    clickOn(Menubar::File::NewProject{});

    clickOn(Sidebar::ManipulationTool::NewPackage{"abc"});
    clickOn(CurrentGraph{0, 0});
    QTest::qWait(100);

    clickOn(Sidebar::ManipulationTool::NewPackage{"def"});
    clickOn(CurrentGraph{300, 500});
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
        for (auto *item : view->items()) {
            if (auto relation = dynamic_cast<LakosRelation *>(item)) {
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
    scene->collapseToplevelEntities();
    REQUIRE(relation);
    REQUIRE(relation->isVisible());
}
