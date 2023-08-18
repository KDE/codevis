// ct_lvtqtc_undo_notes.t.cpp                              -*-C++-*-

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

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_undo_notes.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Undo/Redo Set Notes")
{
    auto tmpDir = TmpDir{"undo_redo_set_notes"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a").value();

    // This GraphicsView is in the heap so that we can control it's lifetime
    auto *gv = new GraphicsViewWrapperForTesting{nodeStorage};
    gv->updatePackageGraph(QString::fromStdString(a->qualifiedName()));
    gv->show();

    auto undoRedo = UndoManager{};

    // this undo command does not trigger a change on the first execution, so we need to update the node too.
    REQUIRE(a->notes().empty());
    a->setNotes("def");
    REQUIRE(a->notes() == "def");

    undoRedo.addUndoCommand(
        new UndoNotes(a->qualifiedName(), a->type(), "abc", "def", dynamic_cast<GraphicsScene *>(gv->scene())));

    undoRedo.undo();
    REQUIRE(a->notes() == "abc");
    undoRedo.redo();
    REQUIRE(a->notes() == "def");
}
