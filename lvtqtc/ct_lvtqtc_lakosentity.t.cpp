// ct_lvtqtc_lakosentity.t.cpp                               -*-C++-*-

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
#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtqtc_componententity.h>
#include <ct_lvtqtc_ellipsistextitem.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_packagedependency.h>
#include <ct_lvtqtc_packageentity.h>

#include <ct_lvtshr_loaderinfo.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <preferences.h>

#include <memory>

#include <QDebug>

using namespace Codethink::lvtshr;
using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;

using Codethink::lvtqtc::LakosEntity;
using PackageDependencyType = Codethink::lvtqtc::PackageDependency;

TEST_CASE_METHOD(QTApplicationFixture, "Lakos entity cover string")
{
    // There was a bug where the cover string isn't being updated after the name of the entity is being changed.
    // This test has been added to ensure no regression will happen.
    auto tmpDir = TmpDir{"lakos_entity_cover"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *package = nodeStorage.addPackage("Pkg", "Pkg", nullptr).value();
    auto *component = nodeStorage.addComponent("Comp", "Comp", package).value();
    auto *classEntity =
        nodeStorage.addLogicalEntity("SomeClass", "SomeClass", component, Codethink::lvtshr::UDTKind::Class).value();

    REQUIRE(package->name() == "Pkg");
    package->setName("B");
    REQUIRE(package->name() == "B");

    REQUIRE(component->name() == "Comp");
    component->setName("CompB");
    REQUIRE(component->name() == "CompB");

    REQUIRE(classEntity->name() == "SomeClass");
    classEntity->setName("AnotherClass");
    REQUIRE(classEntity->name() == "AnotherClass");
}

TEST_CASE_METHOD(QTApplicationFixture, "LakosEntity tests")
{
    auto tmpDir = TmpDir{"lakosentity_tests"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *pkgGroupA = nodeStorage.addPackage("A", "A", nullptr).value();
    auto *pkgChildA = nodeStorage.addPackage("PkgChildA", "A/PkgChildA", pkgGroupA).value();

    auto info = LoaderInfo{false, true, true};
    LakosEntity *entityA = new PackageEntity(pkgGroupA, info);
    LakosEntity *childA = new PackageEntity(pkgChildA, info);
    childA->setParentItem(entityA);

    auto *textItem = [entityA]() -> Codethink::lvtqtc::EllipsisTextItem * {
        for (auto *child : entityA->childItems()) {
            auto *castedChildren = dynamic_cast<Codethink::lvtqtc::EllipsisTextItem *>(child);
            if (castedChildren) {
                return castedChildren;
            }
        }
        return nullptr;
    }();

    REQUIRE(textItem);

    entityA->expand(QtcUtil::CreateUndoAction::e_No);
    REQUIRE(entityA->isExpanded());

    Preferences::setLakosEntityNamePos(Qt::TopLeftCorner);
    REQUIRE(qFuzzyCompare(textItem->pos().x(), entityA->boundingRect().left()));
    REQUIRE(textItem->pos().y() < entityA->boundingRect().top());

    Preferences::setLakosEntityNamePos(Qt::TopRightCorner);
    REQUIRE(qFuzzyCompare(textItem->pos().x() + textItem->boundingRect().width(), entityA->boundingRect().right()));
    REQUIRE(textItem->pos().y() < entityA->boundingRect().top());

    Preferences::setLakosEntityNamePos(Qt::BottomRightCorner);
    REQUIRE(qFuzzyCompare(textItem->pos().x() + textItem->boundingRect().width(), entityA->boundingRect().right()));
    REQUIRE(qFuzzyCompare(textItem->pos().y(), entityA->boundingRect().bottom()));

    Preferences::setLakosEntityNamePos(Qt::BottomLeftCorner);
    REQUIRE(qFuzzyCompare(textItem->pos().x(), entityA->boundingRect().left()));
    REQUIRE(qFuzzyCompare(textItem->pos().y(), entityA->boundingRect().bottom()));
}
