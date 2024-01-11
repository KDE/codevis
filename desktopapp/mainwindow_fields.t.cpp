// // mainwindow_fields.t.cpp                                      -*-C++-*-

// /*
//  * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
//  * // SPDX-License-Identifier: Apache-2.0
//  * //
//  * // Licensed under the Apache License, Version 2.0 (the "License");
//  * // you may not use this file except in compliance with the License.
//  * // You may obtain a copy of the License at
//  * //
//  * //     http://www.apache.org/licenses/LICENSE-2.0
//  * //
//  * // Unless required by applicable law or agreed to in writing, software
//  * // distributed under the License is distributed on an "AS IS" BASIS,
//  * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * // See the License for the specific language governing permissions and
//  * // limitations under the License.
//  */

#include <ct_lvtmdl_fieldstreemodel.h>

#include <ct_lvtmdl_modelhelpers.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosrelation.h>

#include <ct_lvttst_fixture_qt.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtldr_typenode.h>

#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvttst_tmpdir.h>

#include <catch2-local-includes.h>
#include <variant>

#include "ct_lvtclr_colormanagement.h"

// in a header
Q_DECLARE_LOGGING_CATEGORY(LogTest)

// in one source file
Q_LOGGING_CATEGORY(LogTest, "log.test")

using namespace Codethink::lvtldr;
using namespace Codethink::lvtmdl;

TEST_CASE("Ensure FieldTreeModel can format itself from fields in a given collection of class nodes")
{
    // Generate empty nodeStorage
    auto tmpDir = TmpDir{"fields_tree_model_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    // Create some nodes.
    Codethink::lvtldr::LakosianNode *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, std::any()).value();
    Codethink::lvtldr::LakosianNode *pkg_cmp = nodeStorage.addComponent("cmp", "pkg/cmp", pkg).value();
    Codethink::lvtldr::LakosianNode *classA =
        nodeStorage.addLogicalEntity("classA", "pkg/cmp/classA", pkg_cmp, Codethink::lvtshr::UDTKind::Class).value();
    Codethink::lvtldr::LakosianNode *classB =
        nodeStorage.addLogicalEntity("classB", "pkg/cmp/classB", pkg_cmp, Codethink::lvtshr::UDTKind::Class).value();

    // Add fields to class nodes.
    auto classATypeNode = dynamic_cast<TypeNode *>(classA);
    auto classBTypeNode = dynamic_cast<TypeNode *>(classB);

    classATypeNode->addFieldName("field1");
    classATypeNode->addFieldName("field2");
    classATypeNode->addFieldName("field3");

    classBTypeNode->addFieldName("field4");
    classBTypeNode->addFieldName("field5");
    classBTypeNode->addFieldName("field6");
    classBTypeNode->addFieldName("field7");

    // Pass class nodes into a FeildTreeModel
    FieldsTreeModel fieldsTreeModel;
    LakosianNodes nodes;
    nodes.push_back(classA);
    nodes.push_back(classB);
    fieldsTreeModel.refreshData(nodes);

    // Check the FeildTreeModel has been set correctly
    auto getTextFrom = [&fieldsTreeModel](QModelIndex const& i) {
        return fieldsTreeModel.itemFromIndex(i)->text().toStdString();
    };

    REQUIRE(fieldsTreeModel.columnCount() == 1);
    REQUIRE(fieldsTreeModel.rowCount() == 2);

    auto classAIndex = fieldsTreeModel.index(0, 0);
    REQUIRE(getTextFrom(classAIndex) == "classA");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 0, classAIndex)) == "field1");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 1, classAIndex)) == "field2");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 2, classAIndex)) == "field3");

    auto classBIndex = fieldsTreeModel.index(1, 0);
    REQUIRE(getTextFrom(classBIndex) == "classB");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 0, classAIndex)) == "field4");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 1, classAIndex)) == "field5");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 2, classAIndex)) == "field6");
    REQUIRE(getTextFrom(fieldsTreeModel.index(0, 2, classAIndex)) == "field7");
}