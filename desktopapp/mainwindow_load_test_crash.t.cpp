// mainwindow_load_test_crash.t.cpp                                      -*-C++-*-

/* Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
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
    Preferences::self()->useDefaults(true);

    // We need a default tab widget to display elements on the view.
    REQUIRE(hasDefaultTabWidget());

    const bool isOpen = window().openProjectFromPath("testcrash.lks");
    REQUIRE(isOpen);

    QTest::qWait(100);

    clickOn(PackageTreeView::Package{"abc"});
    QTest::qWait(1000);

    // if this passes, we are not crashing.
}
