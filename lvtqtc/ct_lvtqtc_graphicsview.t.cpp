// ct_lvtqtc_graphicsview.t.cpp                           -*-C++-*-

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

#include <ct_lvtqtc_graphicsview.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_typenode.h>
#include <ct_lvtshr_graphenums.h>

#include <QApplication>

#include <catch2-local-includes.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtprj;
using namespace Codethink::lvtldr;

using UDTKind = Codethink::lvtshr::UDTKind;

static auto const projectFileForTesting = ProjectFileForTesting{};

TEST_CASE_METHOD(QTApplicationFixture, "Basic vertex addition test")
{
    auto tmpDir = TmpDir{"basic_vertex_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *packageNode = nodeStorage.addPackage("some package", "some package qn").value();
    auto *componentNode = nodeStorage.addComponent("some component", "some component qn", packageNode).value();
    auto *classNode =
        nodeStorage.addLogicalEntity("some class", "some class qn", componentNode, UDTKind::Class).value();

    GraphicsView view{nodeStorage, projectFileForTesting};
    auto c = std::make_shared<ColorManagement>();
    view.setColorManagement(c);
    view.resize(400, 300);
    auto *scene = dynamic_cast<GraphicsScene *>(view.scene());

    // We have the Text saying "Drop Elements" now.
    auto previous_size = view.items().size();
    REQUIRE(previous_size == 1);

    auto *some_class = scene->addUdtVertex(classNode, false, nullptr, Codethink::lvtshr::LoaderInfo{});
    scene->addItem(some_class);
    REQUIRE(view.items().size() > previous_size);

    previous_size = view.items().size();
    auto *some_component = scene->addCompVertex(componentNode, false, nullptr, Codethink::lvtshr::LoaderInfo{});
    scene->addItem(some_component);
    REQUIRE(view.items().size() > previous_size);

    previous_size = view.items().size();
    auto *some_package = scene->addPkgVertex(packageNode, false, nullptr, Codethink::lvtshr::LoaderInfo{});
    scene->addItem(some_package);
    REQUIRE(view.items().size() > previous_size);

    // Select one entity as "being viewed"
    // scene->setMainEntity(some_class);

    // All items are initially positioned on origin ...
    REQUIRE(some_class->pos() == QPointF{0, 0});
    REQUIRE(some_component->pos() == QPointF{0, 0});
    REQUIRE(some_package->pos() == QPointF{0, 0});

    // Removed Layout Tests because layouts got moved to
    // the plugin system.
}

TEST_CASE_METHOD(QTApplicationFixture, "Vertex and edges test")
{
    auto tmpDir = TmpDir{"vertex_edges_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    GraphicsView view{nodeStorage, projectFileForTesting};
    auto c = std::make_shared<ColorManagement>();
    view.setColorManagement(c);
    view.resize(400, 300);
    auto *scene = dynamic_cast<GraphicsScene *>(view.scene());

    auto *packageNode = nodeStorage.addPackage("some package", "some package qn").value();
    auto *componentNode = nodeStorage.addComponent("some component", "some component qn", packageNode).value();
    auto create_class = [&](std::string const& name) {
        auto *udtNode = nodeStorage.addLogicalEntity(name, "_" + name, componentNode, UDTKind::Class).value();
        auto *item = scene->addUdtVertex(udtNode, false, nullptr, Codethink::lvtshr::LoaderInfo{});
        scene->addItem(item);
        return item;
    };

    auto create_edge = [&](auto *a, auto *b) {
        auto item = scene->addIsARelation(a, b);
        scene->addItem(item);
        return item;
    };

    auto *class_a = create_class("A");
    auto *class_b = create_class("B");
    auto *class_c = create_class("C");

    auto *connect_a_b = create_edge(class_a, class_b);
    auto *connect_b_c = create_edge(class_b, class_c);

    // Select one entity as "being viewed"
    // scene->setMainEntity(class_a);

    REQUIRE(connect_a_b->from() == class_a);
    REQUIRE(connect_a_b->to() == class_b);
    REQUIRE(connect_b_c->from() == class_b);
    REQUIRE(connect_b_c->to() == class_c);
}

TEST_CASE_METHOD(QTApplicationFixture, "Clear graphics view when nodeStorage is wiped out")
{
    auto tmpDir = TmpDir{"clear_gv_on_ns_empty"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    GraphicsView view{nodeStorage, projectFileForTesting};
    GraphicsView view_second{nodeStorage, projectFileForTesting};

    view.show();
    view_second.show();

    auto c = std::make_shared<ColorManagement>();
    view.setColorManagement(c);
    view_second.setColorManagement(c);

    // Empty views have the text "Drop items to view"
    REQUIRE(view.items().size() == 1);
    REQUIRE(view_second.items().size() == 1);
    REQUIRE(view.scene() != view_second.scene());

    auto *scn = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(view.scene());
    auto result = nodeStorage.addPackage("somepkg", "somepkg", nullptr, scn);

    REQUIRE(view_second.items().size() == 1);
    REQUIRE_FALSE(result.has_error());
    REQUIRE(!view.items().empty());
    nodeStorage.clear();
    REQUIRE(view.items().size() == 1);
}

TEST_CASE_METHOD(QTApplicationFixture, "Component on renamed package")
{
    // There was a bug in which renaming a package would cause it's components to be placed outside of it. This test
    // has been created to avoid regression.
    auto tmpDir = TmpDir{"component_on_renamed_pkg_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);
    GraphicsView view{nodeStorage, projectFileForTesting};
    auto c = std::make_shared<ColorManagement>();
    view.setColorManagement(c);
    auto *scene = dynamic_cast<GraphicsScene *>(view.scene());

    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, view.scene()).value();
    auto *pkgView = scene->findLakosEntityFromUid(pkg->uid());
    REQUIRE(pkgView);
    REQUIRE(pkgView->lakosEntities().empty());
    pkg->setName("abc");
    auto *component = nodeStorage.addComponent("component", "abc/component", pkg).value();
    auto *componentView = scene->findLakosEntityFromUid(component->uid());
    REQUIRE(componentView);

    // Before the issue was solved, the following assert was failing:
    REQUIRE(pkgView->lakosEntities().size() == 1);
}

TEST_CASE_METHOD(QTApplicationFixture, "Multiple views")
{
    auto tmpDir = TmpDir{"multiple_views_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto c = std::make_shared<ColorManagement>();
    GraphicsView view1{nodeStorage, projectFileForTesting};
    view1.setColorManagement(c);
    auto *scene1 = dynamic_cast<GraphicsScene *>(view1.scene());
    GraphicsView view2{nodeStorage, projectFileForTesting};
    view2.setColorManagement(c);
    auto *scene2 = dynamic_cast<GraphicsScene *>(view2.scene());

    auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr, scene1).value();
    auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, scene1).value();

    REQUIRE(scene1->findLakosEntityFromUid(pkg2->uid()));
    REQUIRE_FALSE(scene2->findLakosEntityFromUid(pkg2->uid()));
    scene2->loadEntityByQualifiedName(QString::fromStdString(pkg2->qualifiedName()), {100, 100});
    REQUIRE(scene1->findLakosEntityFromUid(pkg2->uid()));
    REQUIRE(scene2->findLakosEntityFromUid(pkg2->uid()));

    // Adding and removing dependencies shouldn't crash when dealing with multiple views
    REQUIRE_FALSE(nodeStorage.addPhysicalDependency(pkg1, pkg2).has_error());
    nodeStorage.removePhysicalDependency(pkg1, pkg2).expect("");
}

TEST_CASE_METHOD(QTApplicationFixture, "Load Class Test")
{
    auto tmpDir = TmpDir{"load_class_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *mainPackage = nodeStorage.addPackage("somepkg", "somepkg").value();
    auto *mainComponent = nodeStorage.addComponent("somecomp", "somecomp", mainPackage).value();
    auto *class1 = nodeStorage.addLogicalEntity("class1", "class1", mainComponent, UDTKind::Class).value();
    auto *class2 = nodeStorage.addLogicalEntity("class2", "class2", mainComponent, UDTKind::Class).value();
    auto *innerClass1 = nodeStorage.addLogicalEntity("innerclass1", "innerclass1", class1, UDTKind::Class).value();
    REQUIRE(innerClass1);

    auto relation = nodeStorage.addLogicalRelation(dynamic_cast<Codethink::lvtldr::TypeNode *>(class1),
                                                   dynamic_cast<Codethink::lvtldr::TypeNode *>(class2),
                                                   Codethink::lvtshr::LakosRelationType::IsA);

    REQUIRE(relation);

    GraphicsView view{nodeStorage, projectFileForTesting};
    auto c = std::make_shared<ColorManagement>();
    view.setColorManagement(c);
    view.show();

    auto *scene = dynamic_cast<GraphicsScene *>(view.scene());

    scene->loadEntityByQualifiedName(QString::fromStdString(innerClass1->qualifiedName()), QPoint(10, 10));

    REQUIRE(scene->entityByQualifiedName("class1"));
    REQUIRE_FALSE(scene->entityByQualifiedName("class2"));

    REQUIRE(scene->entityByQualifiedName("somepkg"));
    REQUIRE(scene->entityByQualifiedName("somecomp"));
    REQUIRE(scene->entityByQualifiedName("innerclass1"));
}
