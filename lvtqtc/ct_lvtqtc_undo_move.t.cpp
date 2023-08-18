// ct_lvtqtc_undo_move.t.cpp                              -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_undo_move.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <QtTest/QTest>
#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Undo/Redo move")
{
    auto tmpDir = TmpDir{"undo_redo_move"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *aa = nodeStorage.addPackage("aa", "aa", a).value();
    auto *ab = nodeStorage.addPackage("ab", "ab", a).value();
    (void) ab;

    // This GraphicsView is in the heap so that we can control it's lifetime
    auto *gv = new GraphicsViewWrapperForTesting{nodeStorage};
    gv->updatePackageGraph(QString::fromStdString(a->qualifiedName()));
    gv->show();
    auto *scene = qobject_cast<GraphicsScene *>(gv->scene());

    LakosEntity *entityAA = scene->entityByQualifiedName("aa");
    auto undoRedo = UndoManager{};
    undoRedo.addUndoCommand(
        new UndoMove(qobject_cast<GraphicsScene *>(gv->scene()), aa->qualifiedName(), QPoint(10, 10), QPoint(20, 20)));

    undoRedo.undo();
    QTest::qWait(1000);
    REQUIRE(entityAA->pos() == QPoint(10, 10));
    undoRedo.redo();
    QTest::qWait(1000);

    REQUIRE(entityAA->pos() == QPoint(20, 20));

    // Calling undo/redo after gv dies won't crash
    delete gv;
    undoRedo.undo();
    undoRedo.redo();
}
