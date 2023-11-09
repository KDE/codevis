// ct_lvtqtw_tabwidget.t.cpp                           -*-C++-*-

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

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtc_testing_utils.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtw_graphtabelement.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtw_tabwidget.h>
#include <ct_lvttst_tmpdir.h>

#include <ct_lvtshr_graphenums.h>

#include <QJsonDocument>
#include <catch2-local-includes.h>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtqtw;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtmdl;
using namespace Codethink::lvtprj;
using namespace Codethink::lvtldr;

TEST_CASE_METHOD(QTApplicationFixture, "Basic tab widget workflow")
{
    auto tmpDir = TmpDir{"basic_tab_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto colorManagement = std::make_shared<ColorManagement>(false);
    auto projectFile = ProjectFileForTesting{};
    auto tab = TabWidget{nodeStorage, projectFile, colorManagement};

    // By default, one tab is created
    REQUIRE(tab.count() == 1);

    // If there's only one tab, and it is closed, a new tab will be created
    tab.closeTab(0);
    REQUIRE(tab.count() == 1);
    REQUIRE(tab.currentIndex() == 0);

    // More tabs can be added and removed
    tab.openNewGraphTab();
    REQUIRE(tab.count() == 2);
    REQUIRE(tab.currentIndex() == 1);
    tab.closeTab(0);
    REQUIRE(tab.count() == 1);
    REQUIRE(tab.currentIndex() == 0);
    REQUIRE(tab.tabText(tab.currentIndex()).toStdString() == "Unnamed 1");

    (void) nodeStorage.addPackage("aaa", "aaa", nullptr);
    (void) nodeStorage.addPackage("bbb", "bbb", nullptr);

    // Now we need to load the graph, and send the data to the graphics scene.
    // it does not load directly anymore.
    tab.openNewGraphTab(QStringLiteral("aaa"));
    REQUIRE(tab.count() == 2);
    REQUIRE(tab.currentIndex() == 1);

    // Wrong tabs are accepted (won't crash), but the tab contents will be empty
    // TODO: Fix this test.
    tab.openNewGraphTab("zzz");
    REQUIRE(tab.count() == 3);
    REQUIRE(tab.currentIndex() == 2);

    tab.replaceGraphAt(1, QStringLiteral("aaa"));
    REQUIRE(tab.count() == 3);
    REQUIRE(tab.currentIndex() == 2);
}

TEST_CASE_METHOD(QTApplicationFixture, "Basic Bookmark Workflow")
{
    auto tmpDir = TmpDir{"basic_tab_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    (void) nodeStorage.addPackage("aaa", "aaa", nullptr);
    (void) nodeStorage.addPackage("bbb", "bbb", nullptr);

    auto colorManagement = std::make_shared<ColorManagement>(false);
    auto projectFile = ProjectFileForTesting{};
    auto tab = TabWidget{nodeStorage, projectFile, colorManagement};

    tab.show();

    // TODO: Fix this test.
    // tab.setCurrentGraphTab(TabWidget::GraphInfo{"aaa", NodeType::e_Package});
    tab.saveBookmark("Bookmark1", 0, Codethink::lvtprj::ProjectFile::Bookmark);
    REQUIRE(tab.tabText(0) == "Bookmark1");

    tab.closeTab(0);
    REQUIRE(tab.tabText(0) == "Unnamed 0");

    tab.loadBookmark(projectFile.getBookmark("Bookmark1"), Codethink::lvtshr::HistoryType::NoHistory);
    REQUIRE(tab.tabText(0) == "Bookmark1");
}
