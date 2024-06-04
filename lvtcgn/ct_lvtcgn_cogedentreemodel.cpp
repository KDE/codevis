// ct_lvtcgn_codegentreemodel.cpp                                       -*-C++-*-

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

#include <ct_lvtcgn_cogedentreemodel.h>
#include <ct_lvtcgn_generatecode.h>

Q_DECLARE_METATYPE(Codethink::lvtcgn::mdl::IPhysicalEntityInfo *)

namespace Codethink::lvtcgn::gui {

using namespace Codethink::lvtcgn::mdl;

CodeGenerationEntitiesTreeModel::CodeGenerationEntitiesTreeModel(ICodeGenerationDataProvider& dataProvider,
                                                                 QObject *parent):
    QStandardItemModel(parent), dataProvider(dataProvider)
{
    setHorizontalHeaderLabels({tr("Generation selection")});
    refreshContents();

    connect(this, &CodeGenerationEntitiesTreeModel::itemChanged, [this](QStandardItem *item) {
        auto& info = *item->data(CodeGenerationDataRole::InfoReferenceRole).value<IPhysicalEntityInfo *>();
        info.setSelectedForCodeGeneration(item->checkState() == Qt::Checked
                                          || item->checkState() == Qt::PartiallyChecked);

        if (itemChangedCascadeUpdate) {
            itemChangedCascadeUpdate = false;
            updateAllChildrenState(item);
            updateParentState(item);
            itemChangedCascadeUpdate = true;
        }
    });
}

void CodeGenerationEntitiesTreeModel::updateAllChildrenState(QStandardItem *item)
{
    if (item->checkState() == Qt::PartiallyChecked) {
        return;
    }

    for (int i = 0; i < item->rowCount(); ++i) {
        item->child(i)->setCheckState(item->checkState());
        updateAllChildrenState(item->child(i));
    }
}

void CodeGenerationEntitiesTreeModel::updateParentState(QStandardItem *item)
{
    auto *parentItem = item->parent();
    if (!parentItem) {
        return;
    }

    auto allChildrenHaveSameState = true;
    auto referenceState = parentItem->child(0)->checkState();
    for (int i = 0; i < parentItem->rowCount(); ++i) {
        if (parentItem->child(i)->checkState() != referenceState) {
            allChildrenHaveSameState = false;
            break;
        }
    }

    parentItem->setCheckState(allChildrenHaveSameState ? referenceState : Qt::PartiallyChecked);
    updateParentState(parentItem);
}

void CodeGenerationEntitiesTreeModel::populateItemAndChildren(IPhysicalEntityInfo *info, QStandardItem *parent)
{
    auto *item = new QStandardItem(info->name());
    item->setData(info->name(), CodeGenerationDataRole::EntityNameRole);
    item->setData(QVariant::fromValue(&info), CodeGenerationDataRole::InfoReferenceRole);
    item->setCheckable(true);
    item->setCheckState(info->selectedForCodeGeneration() ? Qt::Checked : Qt::Unchecked);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    if (parent == nullptr) {
        appendRow(item);
    } else {
        parent->appendRow(item);
    }

    auto children = info->children();
    std::sort(children.begin(), children.end(), [](IPhysicalEntityInfo *l, IPhysicalEntityInfo *r) {
        return l->name() < r->name();
    });
    for (const auto& childInfo : children) {
        populateItemAndChildren(childInfo, item);
    }
}

void CodeGenerationEntitiesTreeModel::refreshContents()
{
    auto children = dataProvider.topLevelEntities();
    std::sort(children.begin(), children.end(), [](IPhysicalEntityInfo *l, IPhysicalEntityInfo *r) {
        if (l->name() == "non-lakosian group") {
            return false;
        }
        if (r->name() == "non-lakosian group") {
            return true;
        }
        return l->name() < r->name();
    });

    for (const auto& rootInfo : children) {
        populateItemAndChildren(rootInfo, nullptr);
    }
}

void CodeGenerationEntitiesTreeModel::recursiveExec(std::function<RecursiveExec(QStandardItem *)> f)
{
    std::function<RecursiveExec(QStandardItem * item)> recursiveSearchAndExec = [&](QStandardItem *item) {
        if (f(item) == RecursiveExec::StopSearch) {
            return RecursiveExec::StopSearch;
        }

        for (auto i = 0; i < item->rowCount(); ++i) {
            auto *childItem = item->child(i);
            if (recursiveSearchAndExec(childItem) == RecursiveExec::StopSearch) {
                return RecursiveExec::StopSearch;
            }
        }
        return RecursiveExec::ContinueSearch;
    };

    for (auto i = 0; i < rowCount(); ++i) {
        if (recursiveSearchAndExec(item(i)) == RecursiveExec::StopSearch) {
            return;
        }
    }
}

} // namespace Codethink::lvtcgn::gui
