// ct_lvtqtc_tool_add_component.t.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_tool_add_component.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvttst_fixture_qt.h>

#include <catch2-local-includes.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

#include <QJsonObject>
#include <QLineEdit>

#include <QtTest/QTest>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Relayout Single Entity")
{
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, gv.scene()).value();
    auto comp1 = nodeStorage.addComponent("pkg_comp1", "pkg_comp1", pkg);
    REQUIRE_FALSE(comp1.has_error());

    auto comp2 = nodeStorage.addComponent("pkg_comp2", "pkg_comp2", pkg);
    REQUIRE_FALSE(comp2.has_error());

    auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, gv.scene()).value();
    REQUIRE(pkg2);

    auto comp3 = nodeStorage.addComponent("pkg_comp3", "pkg_comp3", pkg2);
    REQUIRE_FALSE(comp3.has_error());

    auto comp4 = nodeStorage.addComponent("pkg_comp4", "pkg_comp4", pkg2);
    REQUIRE_FALSE(comp4.has_error());

    LakosEntity *pkgEntity2 = gs->entityByQualifiedName("pkg2");
    LakosEntity *pkgComp1 = gs->entityByQualifiedName("pkg_comp1");
    pkgComp1->setPos(-1, -1);
    LakosEntity *pkgComp2 = gs->entityByQualifiedName("pkg_comp2");
    pkgComp2->setPos(-2, -3);
    LakosEntity *pkgComp3 = gs->entityByQualifiedName("pkg_comp3");
    pkgEntity2->setPos(200, 0);

    Q_EMIT pkgComp1->toggleSelection();
    REQUIRE(gs->selectedItems().count() == 1);

    // clazy:excludeall=lambda-in-connect
    bool valid = false;
    QObject::connect(gs, &GraphicsScene::selectedEntityChanged, gs, [&valid] {
        valid = true;
    });

    // This should trigger the selection via the scene, changing the selected entity,
    // triggering the above lambda.
    Q_EMIT pkgComp3->toggleSelection();
    REQUIRE(valid);

    const size_t original_edges_size = gs->edges().size();
    const size_t original_vertices_size = gs->vertices().size();

    REQUIRE(original_vertices_size != 0);
    // Save the state in json
    const QJsonObject json = gs->toJson();

    gs->clearGraph();

    // Item "Drag to View"
    REQUIRE(gs->items().size() == 1);

    gs->fromJson(json);

    REQUIRE(gs->vertices().size() == original_vertices_size);
    REQUIRE(gs->edges().size() == original_edges_size);
}

TEST_CASE_METHOD(QTApplicationFixture, "Add and remove edges")
{
    auto tmpDir = TmpDir{"add_rm_edges"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto *pkg1 = nodeStorage.addPackage("pkg", "pkg", nullptr, gv.scene()).value();
    REQUIRE(nodeStorage.addComponent("pkg1_comp1", "pkg1_comp1", pkg1).has_value());
    REQUIRE(nodeStorage.addComponent("pkg1_comp2", "pkg1_comp2", pkg1).has_value());

    auto *pkgComp1 = gs->entityByQualifiedName("pkg1_comp1");
    REQUIRE(pkgComp1);
    auto *pkgComp2 = gs->entityByQualifiedName("pkg1_comp2");
    REQUIRE(pkgComp2);

    REQUIRE_FALSE(pkgComp1->hasRelationshipWith(pkgComp2));
    (void) gs->addPackageDependencyRelation(pkgComp1, pkgComp2);
    REQUIRE(pkgComp1->hasRelationshipWith(pkgComp2));
    gs->removeEdge(*pkgComp1, *pkgComp2);
    REQUIRE_FALSE(pkgComp1->hasRelationshipWith(pkgComp2));
}

TEST_CASE_METHOD(QTApplicationFixture, "Single level")
{
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    std::ignore = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    std::ignore = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    std::ignore = nodeStorage.addPackage("ccc", "ccc", nullptr, gv.scene()).value();

    gs->calculateLevels();
    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");
    auto *ccc = gs->entityByQualifiedName("ccc");
    REQUIRE(aaa);
    REQUIRE(bbb);
    REQUIRE(ccc);

    REQUIRE(aaa->lakosianLevel() == 1);
    REQUIRE(bbb->lakosianLevel() == 1);
    REQUIRE(ccc->lakosianLevel() == 1);
}

TEST_CASE_METHOD(QTApplicationFixture, "Basic children levels")
{
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto a = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    auto b = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    auto c = nodeStorage.addPackage("ccc", "ccc", nullptr, gv.scene()).value();

    std::ignore = nodeStorage.addPhysicalDependency(a, b, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, c, true);

    gs->calculateLevels();

    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");
    auto *ccc = gs->entityByQualifiedName("ccc");

    REQUIRE(aaa->lakosianLevel() == 2);
    REQUIRE(bbb->lakosianLevel() == 1);
    REQUIRE(ccc->lakosianLevel() == 0);
}

TEST_CASE_METHOD(QTApplicationFixture, "Children levels multilevel ddd")
{
    /**
     * ccc  eee
     * ^      ^
     * |    ddd
     * |    ^ ^
     * |   /  |
     * bbb   /
     * ^    /
     * aaa
     */

    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto a = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    auto b = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    auto c = nodeStorage.addPackage("ccc", "ccc", nullptr, gv.scene()).value();
    auto d = nodeStorage.addPackage("ddd", "ddd", nullptr, gv.scene()).value();
    auto e = nodeStorage.addPackage("eee", "eee", nullptr, gv.scene()).value();

    std::ignore = nodeStorage.addPhysicalDependency(a, b, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, c, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(a, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(d, e, true);

    gs->calculateLevels();

    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");
    auto *ccc = gs->entityByQualifiedName("ccc");
    auto *ddd = gs->entityByQualifiedName("ddd");
    auto *eee = gs->entityByQualifiedName("eee");

    REQUIRE(aaa->lakosianLevel() == 3);
    REQUIRE(bbb->lakosianLevel() == 2);
    REQUIRE(ccc->lakosianLevel() == 0);
    REQUIRE(ddd->lakosianLevel() == 1);
    REQUIRE(eee->lakosianLevel() == 0);
}

TEST_CASE_METHOD(QTApplicationFixture, "Children simple cycle")
{
    /**
     * aaa <--> bbb
     */
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto *a = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    auto *b = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    std::ignore = nodeStorage.addPhysicalDependency(a, b, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, a, true);

    gs->calculateLevels();
    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");

    REQUIRE(aaa);
    REQUIRE(bbb);
    REQUIRE(aaa->lakosianLevel() == 0);
    REQUIRE(bbb->lakosianLevel() == 0);
}

TEST_CASE_METHOD(QTApplicationFixture, "Children level with cycle no clear startpoint")
{
    /**
     * ccc   eee            // LVL 0
     * ^      ^
     * |    ddd ----        // LVL 1
     * |    ^ ^     \
     * |   /  |     |
     * bbb   /     /        // LVL 2
     * ^    /     /
     * aaa    <--           // LVL 3
     */

    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto a = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    auto b = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    auto c = nodeStorage.addPackage("ccc", "ccc", nullptr, gv.scene()).value();
    auto d = nodeStorage.addPackage("ddd", "ddd", nullptr, gv.scene()).value();
    auto e = nodeStorage.addPackage("eee", "eee", nullptr, gv.scene()).value();

    std::ignore = nodeStorage.addPhysicalDependency(a, b, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, c, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(a, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(d, e, true);
    std::ignore = nodeStorage.addPhysicalDependency(d, a, true);

    gs->calculateLevels();

    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");
    auto *ccc = gs->entityByQualifiedName("ccc");
    auto *ddd = gs->entityByQualifiedName("ddd");
    auto *eee = gs->entityByQualifiedName("eee");

    REQUIRE(aaa->lakosianLevel() == 3);
    REQUIRE(bbb->lakosianLevel() == 2);
    REQUIRE(ccc->lakosianLevel() == 0);
    REQUIRE(ddd->lakosianLevel() == 1);
    REQUIRE(eee->lakosianLevel() == 0);
}

TEST_CASE_METHOD(QTApplicationFixture, "Children level with cycle clear level 0")
{
    /**
     * ccc  eee
     * ^      ^
     * |    ddd ----
     * |    ^ ^     \
     * |   /  |     |
     * bbb   /     /
     * ^    /     /
     * aaa    <--
     * ^
     * xxx
     */
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto x = nodeStorage.addPackage("xxx", "xxx", nullptr, gv.scene()).value();
    auto a = nodeStorage.addPackage("aaa", "aaa", nullptr, gv.scene()).value();
    auto b = nodeStorage.addPackage("bbb", "bbb", nullptr, gv.scene()).value();
    auto c = nodeStorage.addPackage("ccc", "ccc", nullptr, gv.scene()).value();
    auto d = nodeStorage.addPackage("ddd", "ddd", nullptr, gv.scene()).value();
    auto e = nodeStorage.addPackage("eee", "eee", nullptr, gv.scene()).value();

    std::ignore = nodeStorage.addPhysicalDependency(x, a, true);
    std::ignore = nodeStorage.addPhysicalDependency(a, b, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, c, true);
    std::ignore = nodeStorage.addPhysicalDependency(b, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(a, d, true);
    std::ignore = nodeStorage.addPhysicalDependency(d, e, true);
    std::ignore = nodeStorage.addPhysicalDependency(d, a, true);

    gs->calculateLevels();

    auto *xxx = gs->entityByQualifiedName("xxx");
    auto *aaa = gs->entityByQualifiedName("aaa");
    auto *bbb = gs->entityByQualifiedName("bbb");
    auto *ccc = gs->entityByQualifiedName("ccc");
    auto *ddd = gs->entityByQualifiedName("ddd");
    auto *eee = gs->entityByQualifiedName("eee");

    REQUIRE(xxx->lakosianLevel() == 4);
    REQUIRE(aaa->lakosianLevel() == 3);
    REQUIRE(bbb->lakosianLevel() == 2);
    REQUIRE(ccc->lakosianLevel() == 0);
    REQUIRE(ddd->lakosianLevel() == 1);
    REQUIRE(eee->lakosianLevel() == 0);
}
