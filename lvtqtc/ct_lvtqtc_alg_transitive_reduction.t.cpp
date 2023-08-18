// ct_lvtqtc_alg_transitive_reduction.t.cpp                           -*-C++-*-

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

#include <ct_lvtqtc_alg_transitive_reduction.h>

#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_lakosrelation.h>
#include <ct_lvtqtc_logicalentity.h>
#include <ct_lvtqtc_packagedependency.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtqtc_usesintheimplementation.h>
#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtshr_loaderinfo.h>

#include <catch2-local-includes.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;

using UDTKind = Codethink::lvtshr::UDTKind;

TEST_CASE_METHOD(QTApplicationFixture, "Circular Graph")
{
    auto tmpDir = TmpDir{"circular_graph"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto storage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *pkg = storage.addPackage("pkg", "pkg").value();
    auto *cmp = storage.addComponent("cmp", "cmp", pkg).value();
    for (int i = 1; i <= 5; i++) {
        auto name = "udt" + std::to_string(i);
        auto qualifiedName = "udt" + std::to_string(i);
        storage.addLogicalEntity(name, qualifiedName, cmp, UDTKind::Class)
            .expect("Unexpected error on addLogicalEntity");
    }
    auto *aNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt1");
    auto *bNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt2");
    auto *cNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt3");
    auto *dNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt4");
    auto *eNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt5");

    // name, qualified name, parent name, id, loader info.
    // we are not interested in the loader info, just pass a
    // default constructed one.
    // there's no parent for the entities, too, so an empty string
    // suffices.
    auto info = Codethink::lvtshr::LoaderInfo{};
    auto *a = new LogicalEntity(aNode, info);
    auto *b = new LogicalEntity(bNode, info);
    auto *c = new LogicalEntity(cNode, info);
    auto *d = new LogicalEntity(dNode, info);
    auto *e = new LogicalEntity(eNode, info);

    // The edges that will belong to the edge collection.
    auto *ab = new UsesInTheImplementation(a, b);
    auto *ac = new UsesInTheImplementation(a, c);
    auto *bc = new UsesInTheImplementation(b, c);
    auto *cd = new UsesInTheImplementation(c, d);
    auto *de = new UsesInTheImplementation(d, e); // Circular edge here. E -> B -> C - > D -> E
    auto *eb = new UsesInTheImplementation(e, b);
    auto *bd = new UsesInTheImplementation(b, d);

    std::vector<LakosEntity *> vertices = {a, b, c, d, e};

    // one edge collection per edge.
    auto colAB = std::make_shared<EdgeCollection>();
    colAB->setFrom(a);
    colAB->setTo(b);
    colAB->addRelation(ab);

    auto colAC = std::make_shared<EdgeCollection>();
    colAC->setFrom(a);
    colAC->setTo(c);
    colAC->addRelation(ac);

    auto colBC = std::make_shared<EdgeCollection>();
    colBC->setFrom(b);
    colBC->setTo(c);
    colBC->addRelation(bc);

    auto colBD = std::make_shared<EdgeCollection>();
    colBD->setFrom(b);
    colBD->setTo(d);
    colBD->addRelation(bd);

    auto colCD = std::make_shared<EdgeCollection>();
    colCD->setFrom(c);
    colCD->setTo(d);
    colCD->addRelation(cd);

    auto colEB = std::make_shared<EdgeCollection>();
    colEB->setFrom(e);
    colEB->setTo(b);
    colEB->addRelation(eb);

    auto colDE = std::make_shared<EdgeCollection>();
    colDE->setFrom(d);
    colDE->setTo(e);
    colDE->addRelation(de);

    // feels weird to use a const method to add stuff on the object.
    a->edgesCollection().push_back(colAB);
    a->edgesCollection().push_back(colAC);
    b->edgesCollection().push_back(colBC);
    b->edgesCollection().push_back(colBD);
    c->edgesCollection().push_back(colCD);
    // d has no out edges.
    e->edgesCollection().push_back(colEB);
    e->edgesCollection().push_back(colDE);

    auto *algorithm = new AlgorithmTransitiveReduction();
    algorithm->setVertices(vertices);

    algorithm->run();

    // after updating the algorithm to handle redundant edges,
    // cycles won't create errors anymore..
    assert(!algorithm->hasError());
    algorithm->deleteLater();
}

TEST_CASE_METHOD(QTApplicationFixture, "Small Graph")
{
    auto tmpDir = TmpDir{"small_graph"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto storage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *pkg = storage.addPackage("pkg", "pkg").value();
    auto *cmp = storage.addComponent("cmp", "cmp", pkg).value();
    for (int i = 1; i <= 5; i++) {
        auto name = "udt" + std::to_string(i);
        auto qualifiedName = "udt" + std::to_string(i);
        storage.addLogicalEntity(name, qualifiedName, cmp, UDTKind::Class)
            .expect("Unexpected error on addLogicalEntity");
    }

    auto *aNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt1");
    auto *bNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt2");
    auto *cNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt3");
    auto *dNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt4");
    auto *eNode = storage.findByQualifiedName(Codethink::lvtshr::DiagramType::ClassType, "udt5");

    // we are not interested in the loader info, just pass a
    // default constructed one.
    // there's no parent for the entities, too, so an empty string
    // suffices.
    auto info = Codethink::lvtshr::LoaderInfo{};
    auto *a = new LogicalEntity(aNode, info);
    auto *b = new LogicalEntity(bNode, info);
    auto *c = new LogicalEntity(cNode, info);
    auto *d = new LogicalEntity(dNode, info);
    auto *e = new LogicalEntity(eNode, info);

    // The edges that will belong to the edge collection.
    auto *ab = new UsesInTheImplementation(a, b);
    auto *ac = new UsesInTheImplementation(a, c);
    auto *bc = new UsesInTheImplementation(b, c);
    auto *cd = new UsesInTheImplementation(c, d);
    auto *ed = new UsesInTheImplementation(e, d);
    auto *eb = new UsesInTheImplementation(e, b);
    auto *bd = new UsesInTheImplementation(b, d);

    std::vector<LakosEntity *> vertices = {a, b, c, d, e};

    // one edge collection per edge.
    auto colAB = std::make_shared<EdgeCollection>();
    colAB->setFrom(a);
    colAB->setTo(b);
    colAB->addRelation(ab);

    auto colAC = std::make_shared<EdgeCollection>();
    colAC->setFrom(a);
    colAC->setTo(c);
    colAC->addRelation(ac);

    auto colBC = std::make_shared<EdgeCollection>();
    colBC->setFrom(b);
    colBC->setTo(c);
    colBC->addRelation(bc);

    auto colBD = std::make_shared<EdgeCollection>();
    colBD->setFrom(b);
    colBD->setTo(d);
    colBD->addRelation(bd);

    auto colCD = std::make_shared<EdgeCollection>();
    colCD->setFrom(c);
    colCD->setTo(d);
    colCD->addRelation(cd);

    auto colEB = std::make_shared<EdgeCollection>();
    colEB->setFrom(e);
    colEB->setTo(b);
    colEB->addRelation(eb);

    auto colED = std::make_shared<EdgeCollection>();
    colED->setFrom(e);
    colED->setTo(d);
    colED->addRelation(ed);

    // feels weird to use a const method to add stuff on the object.
    a->edgesCollection().push_back(colAB);
    a->edgesCollection().push_back(colAC);
    b->edgesCollection().push_back(colBC);
    b->edgesCollection().push_back(colBD);
    c->edgesCollection().push_back(colCD);
    // d has no out edges.
    e->edgesCollection().push_back(colEB);
    e->edgesCollection().push_back(colED);

    auto *algorithm = new AlgorithmTransitiveReduction();
    algorithm->setVertices(vertices);

    algorithm->run();

    assert(!algorithm->hasError());

    // we have three nodes with removed edges here.
    assert(algorithm->redundantEdgesByNode().size() == 3);

    algorithm->deleteLater();
}

TEST_CASE_METHOD(QTApplicationFixture, "Complete Graph")
{
    auto tmpDir = TmpDir{"trans_red_complete_graph"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto info = Codethink::lvtshr::LoaderInfo{};
    auto vertices = std::vector<LakosEntity *>{};
    for (auto const& name : {"a", "b", "c"}) {
        auto *pkg = nodeStorage.addPackage(name, name).value();
        vertices.push_back(new PackageEntity{pkg, info});
    }
    std::vector<LakosRelation *> edges = {};
    for (auto *v1 : vertices) {
        for (auto *v2 : vertices) {
            if (v1 == v2) {
                continue;
            }
            nodeStorage.addPhysicalDependency(v1->internalNode(), v2->internalNode()).expect("");

            auto *e = new PackageDependency{v1, v2};
            edges.push_back(e);

            auto ec = std::make_shared<EdgeCollection>();
            ec->setFrom(e->from());
            ec->setTo(e->to());
            ec->addRelation(e);

            e->from()->edgesCollection().push_back(ec);
        }
    }

    auto algorithm = AlgorithmTransitiveReduction{};
    algorithm.setVertices(vertices);
    algorithm.run();
    REQUIRE(!algorithm.hasError());

    auto isRedundantEdge = [&vertices](const std::string& fromName, const std::string& toName) {
        auto v = std::find_if(vertices.cbegin(), vertices.cend(), [&fromName](auto w) {
            return w->name() == fromName;
        });
        REQUIRE(v != vertices.cend());
        for (auto const& e : (*v)->edgesCollection()) {
            if (e->to()->name() == toName) {
                return e->isRedundant();
            }
        }
        assert(false && "not found");
        return false;
    };

    // clang-format off
    INFO("Solution = "
        << (isRedundantEdge("a", "b") ? "1" : "0")
        << ", " << (isRedundantEdge("a", "c") ? "1" : "0")
        << ", " << (isRedundantEdge("b", "a") ? "1" : "0")
        << ", " << (isRedundantEdge("b", "c") ? "1" : "0")
        << ", " << (isRedundantEdge("c", "a") ? "1" : "0")
        << ", " << (isRedundantEdge("c", "b") ? "1" : "0"));
    REQUIRE((
        // Possible solution 1
        (
            isRedundantEdge("a", "b") &&
            !isRedundantEdge("a", "c") &&
            isRedundantEdge("b", "a") &&
            !isRedundantEdge("b", "c") &&
            isRedundantEdge("c", "a") &&
            !isRedundantEdge("c", "b")
        ) ||
        // Possible solution 2
        (
            isRedundantEdge("a", "b") &&
            !isRedundantEdge("a", "c") &&
            isRedundantEdge("b", "a") &&
            !isRedundantEdge("b", "c") &&
            !isRedundantEdge("c", "a") &&
            !isRedundantEdge("c", "b")
        ) ||
        // Possible solution 3
        (
            isRedundantEdge("a", "b") &&
            !isRedundantEdge("a", "c") &&
            !isRedundantEdge("b", "a") &&
            isRedundantEdge("b", "c") &&
            isRedundantEdge("c", "a") &&
            !isRedundantEdge("c", "b")
        ) ||
        // Possible solution 4
        (
            isRedundantEdge("a", "b") &&
            !isRedundantEdge("a", "c") &&
            isRedundantEdge("b", "a") &&
            !isRedundantEdge("b", "c") &&
            !isRedundantEdge("c", "a") &&
            isRedundantEdge("c", "b")
        ) ||
        // Possible solution 5
        (
            !isRedundantEdge("a", "b") &&
            isRedundantEdge("a", "c") &&
            isRedundantEdge("b", "a") &&
            !isRedundantEdge("b", "c") &&
            !isRedundantEdge("c", "a") &&
            isRedundantEdge("c", "b")
        )
    ));
    // clang-format on
}
