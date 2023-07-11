// ct_lvtqtc_undo_add_package.t.cpp                               -*-C++-*-

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
#include <catch2/catch.hpp>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_undo_add_package.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Undo/Redo add component")
{
    auto tmpDir = TmpDir{"undo_redo_add_comp"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *aa = nodeStorage.addPackage("aa", "aa", a).value();

    // This GraphicsView is in the heap so that we can control it's lifetime
    auto *gv = new GraphicsViewWrapperForTesting{nodeStorage};
    gv->updatePackageGraph(QString::fromStdString(a->qualifiedName()));
    gv->show();

    auto undoRedo = UndoManager{};
    undoRedo.addUndoCommand(new UndoAddPackage(qobject_cast<GraphicsScene *>(gv->scene()),
                                               {10, 10},
                                               "aa",
                                               "aa",
                                               "a",
                                               QtcUtil::UndoActionType::e_Add,
                                               nodeStorage));

    auto aaaUid = aa->uid();

    aa->setNotes("Some test notes");
    REQUIRE(nodeStorage.findById(aaaUid));
    REQUIRE(gv->hasEntityWithId(aaaUid));

    undoRedo.undo();

    // Component is removed from both nodeStorage and the GraphicsScene
    REQUIRE(nodeStorage.findById(aaaUid) == nullptr);
    REQUIRE_FALSE(gv->hasEntityWithId(aaaUid));

    undoRedo.redo();

    // Component is added back to nodeStorage and GraphicsScene
    aa = nodeStorage.findByQualifiedName(DiagramType::PackageType, "aa");
    REQUIRE(aa);
    aaaUid = aa->uid();
    REQUIRE(nodeStorage.findById(aaaUid));
    REQUIRE(gv->hasEntityWithId(aaaUid));
    REQUIRE(aa->notes() == "Some test notes");

    // Undo/Redo must work even if the GraphicsScene gets deleted
    delete gv;

    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "aa"));
    undoRedo.undo();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "aa") == nullptr);
    undoRedo.redo();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "aa"));
}
