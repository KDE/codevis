// ct_lvtmdl_modelhelpers.t.cpp                                        -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtshr_graphenums.h>

#include <catch2/catch.hpp>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

using namespace Codethink::lvtmdl;
using namespace Codethink::lvtldr;

TEST_CASE("Model helpers test")
{
    auto tmpDir = TmpDir{"model_helpers_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    Codethink::lvtldr::LakosianNode *tmz = ns.addPackage("tmz", "tmz", nullptr, std::any()).value();
    Codethink::lvtldr::LakosianNode *tmz_a = ns.addPackage("tmz_a", "tmz_a", tmz, std::any()).value();
    Codethink::lvtldr::LakosianNode *tmp_a_cmp = ns.addComponent("cmp", "tmz/tmz_a/cmp", tmz_a).value();
    Codethink::lvtldr::LakosianNode *cmp_klass =
        ns.addLogicalEntity("klass", "tmz/tmz_a/cmp/klass", tmp_a_cmp, Codethink::lvtshr::UDTKind::Class).value();

    {
        auto *item = ModelUtil::createTreeItemFromLakosianNode(*tmz);
        REQUIRE(item->text().toStdString() == tmz->name());
        REQUIRE(item->data(ModelRoles::e_Id) == tmz->id());
        REQUIRE(item->data(ModelRoles::e_QualifiedName).toString().toStdString() == tmz->qualifiedName());
        REQUIRE(item->data(ModelRoles::e_IsBranch) == true);
        REQUIRE(item->data(ModelRoles::e_NodeType) == NodeType::Enum::e_Package);
        REQUIRE(item->data(ModelRoles::e_RecursiveLakosian) == false);
        // If no information is provided, item will not load children, but will have 1 dummy child
        REQUIRE(item->data(ModelRoles::e_ChildItemsLoaded) == false);
        REQUIRE(item->rowCount() == 1);
        auto *dummy_child = item->child(0);
        REQUIRE(dummy_child->text().isEmpty());
    }

    auto *item = ModelUtil::createTreeItemFromLakosianNode(*tmz, [](LakosianNode const&) {
        return true;
    });
    REQUIRE(item->text().toStdString() == tmz->name());
    REQUIRE(item->data(ModelRoles::e_Id) == tmz->id());
    REQUIRE(item->data(ModelRoles::e_QualifiedName).toString().toStdString() == tmz->qualifiedName());
    REQUIRE(item->data(ModelRoles::e_IsBranch) == true);
    REQUIRE(item->data(ModelRoles::e_NodeType) == NodeType::Enum::e_Package);
    REQUIRE(item->data(ModelRoles::e_RecursiveLakosian) == false);
    REQUIRE(item->data(ModelRoles::e_ChildItemsLoaded) == true);
    REQUIRE(item->rowCount() == 1);

    auto *tmz_a_item = item->child(0);
    REQUIRE(tmz_a_item->text().toStdString() == tmz_a->name());
    REQUIRE(tmz_a_item->data(ModelRoles::e_Id) == tmz_a->id());
    REQUIRE(tmz_a_item->data(ModelRoles::e_QualifiedName).toString().toStdString() == tmz_a->qualifiedName());
    REQUIRE(tmz_a_item->data(ModelRoles::e_IsBranch) == true);
    REQUIRE(tmz_a_item->data(ModelRoles::e_NodeType) == NodeType::Enum::e_Package);
    REQUIRE(tmz_a_item->data(ModelRoles::e_RecursiveLakosian) == false);
    REQUIRE(tmz_a_item->rowCount() == 1);

    auto *cmp_item = tmz_a_item->child(0);
    REQUIRE(cmp_item->text().toStdString() == tmp_a_cmp->name());
    REQUIRE(cmp_item->data(ModelRoles::e_Id) == tmp_a_cmp->id());
    REQUIRE(cmp_item->data(ModelRoles::e_QualifiedName).toString().toStdString() == tmp_a_cmp->qualifiedName());
    REQUIRE(cmp_item->data(ModelRoles::e_IsBranch) == true);
    REQUIRE(cmp_item->data(ModelRoles::e_NodeType) == NodeType::Enum::e_Component);
    REQUIRE(cmp_item->data(ModelRoles::e_RecursiveLakosian) == false);
    REQUIRE(cmp_item->rowCount() == 1);

    auto *cmp_klass_item = cmp_item->child(0);
    REQUIRE(cmp_klass_item->text().toStdString() == cmp_klass->name());
    REQUIRE(cmp_klass_item->data(ModelRoles::e_Id) == cmp_klass->id());
    REQUIRE(cmp_klass_item->data(ModelRoles::e_QualifiedName).toString().toStdString() == cmp_klass->qualifiedName());
    REQUIRE(cmp_klass_item->data(ModelRoles::e_IsBranch) == false);
    REQUIRE(cmp_klass_item->data(ModelRoles::e_NodeType) == NodeType::Enum::e_Class);
    REQUIRE(cmp_klass_item->data(ModelRoles::e_RecursiveLakosian) == true);
    REQUIRE(cmp_klass_item->rowCount() == 0);
}
