// ct_lvtmdl_usesintheimpltablemodel.cpp                                         -*-C++-*-

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

#include <ct_lvtldr_typenode.h>
#include <ct_lvtmdl_usesintheimpltablemodel.h>

namespace Codethink::lvtmdl {

UsesInTheImplTableModel::UsesInTheImplTableModel() = default;
UsesInTheImplTableModel::~UsesInTheImplTableModel() = default;

void UsesInTheImplTableModel::refreshData(const LakosianNodes& selectedNodes)
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

    for (const auto& lakosianNode : selectedNodes) {
        if (lakosianNode->type() == DiagramType::ClassType) {
            const auto classNode = dynamic_cast<TypeNode *>(lakosianNode);
            auto classItem = createItemFromClassNode(classNode);

            for (auto *node : classNode->usesInTheImplementation()) {
                auto innerItem = createItemFromClassNode(node);
                classItem->appendRow(innerItem);
            }
            root->appendRow(classItem);
        }
    }
}

} // end namespace Codethink::lvtmdl
