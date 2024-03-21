// ct_lvtmdl_fieldstreemodel.cpp                                         -*-C++-*-

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

#include <ct_lvtmdl_fieldstreemodel.h>

#include <ct_lvtmdl_modelhelpers.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_typenode.h>
#include <ct_lvtshr_stringhelpers.h>

namespace Codethink::lvtmdl {

FieldsTreeModel::FieldsTreeModel() = default;
FieldsTreeModel::~FieldsTreeModel() = default;

void FieldsTreeModel::refreshData(LakosianNodes selectedNodes)
{
    clear();

    if (selectedNodes.empty()) {
        return;
    }

    using namespace Codethink::lvtldr;
    using namespace Codethink::lvtshr;

    QStandardItem *root = invisibleRootItem();

    auto createItemFromClassNode = [](LakosianNode *node) {
        auto *item = new QStandardItem();
        item->setText(QString::fromStdString(node->name()));
        item->setEditable(false);
        item->setIcon(QIcon(":/icons/class"));
        return item;
    };

    auto createItemFromString = [](std::string name) {
        auto *item = new QStandardItem();
        item->setText(QString::fromStdString(name));
        item->setEditable(false);
        return item;
    };

    for (const auto& lakosianNode : selectedNodes) {
        if (lakosianNode->type() == DiagramType::ClassType) {
            const auto classNode = dynamic_cast<TypeNode *>(lakosianNode);
            auto classItem = createItemFromClassNode(classNode);

            auto fields = classNode->getFields();
            for (auto name : fields.fieldNames) {
                classItem->appendRow(createItemFromString(name));
            }

            root->appendRow(classItem);
        }
    }
}

} // end namespace Codethink::lvtmdl
