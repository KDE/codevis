// ct_lvtqtc_undo_load_entity.t.cpp                               -*-C++-*-

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
#include <ct_lvtqtc_undo_load_entity.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Undo/Redo Load Components")
{
    auto tmpDir = TmpDir{"undo_redo_load_comps"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *aa = nodeStorage.addPackage("aa", "aa", a).value();
    auto *aaa = nodeStorage.addComponent("aaa", "aaa", aa).value();

    GraphicsViewWrapperForTesting gv{nodeStorage};
    // TODO: Update this call.
    // gv.updatePackageGraph(QString::fromStdString(a->qualifiedName()));
    gv.show();

    auto *scene = qobject_cast<GraphicsScene *>(gv.scene());
    auto *aa_node = scene->entityByQualifiedName(aa->qualifiedName());
    REQUIRE(aa_node);

    auto undoRedo = UndoManager{};
    undoRedo.addUndoCommand(new UndoLoadEntity(qobject_cast<GraphicsScene *>(gv.scene()),
                                               aa->uid(),
                                               GraphicsScene::UnloadDepth::Entity,
                                               QtcUtil::UndoActionType::e_Remove));

    aa_node = scene->entityByQualifiedName(aa->qualifiedName());
    REQUIRE_FALSE(aa_node);

    undoRedo.undo();

    aa_node = scene->entityByQualifiedName(aa->qualifiedName());
    REQUIRE(aa_node);

    undoRedo.redo();

    aa_node = scene->entityByQualifiedName(aa->qualifiedName());
    REQUIRE_FALSE(aa_node);

    undoRedo.undo();
    aa_node = scene->entityByQualifiedName(aa->qualifiedName());
    REQUIRE(aa_node);

    auto *aaa_node = scene->entityByQualifiedName(aaa->qualifiedName());
    REQUIRE_FALSE(aaa_node);

    undoRedo.addUndoCommand(new UndoLoadEntity(qobject_cast<GraphicsScene *>(gv.scene()),
                                               aa->uid(),
                                               GraphicsScene::UnloadDepth::Children,
                                               QtcUtil::UndoActionType::e_Add));

    aaa_node = scene->entityByQualifiedName(aaa->qualifiedName());
    REQUIRE(aaa_node);

    undoRedo.undo();

    aaa_node = scene->entityByQualifiedName(aaa->qualifiedName());
    REQUIRE_FALSE(aaa_node);

    undoRedo.redo();

    aaa_node = scene->entityByQualifiedName(aaa->qualifiedName());
    REQUIRE(aaa_node);
}
