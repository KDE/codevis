// ct_lvtmdl_packagetreemodel.cpp                                  -*-C++-*-

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

#include <ct_lvtmdl_packagetreemodel.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtldr_repositorynode.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtmdl_modelhelpers.h>

namespace Codethink::lvtmdl {

using namespace lvtshr;
using namespace lvtldr;

// --------------------------------------------
// struct PackageTreeModelPrivate
// --------------------------------------------
struct PackageTreeModel::PackageTreeModelPrivate {
    NodeStorage& nodeStorage;

    explicit PackageTreeModelPrivate(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};
// --------------------------------------------
// class PackageTreeModel
// --------------------------------------------

PackageTreeModel::PackageTreeModel(NodeStorage& nodeStorage):
    d(std::make_unique<PackageTreeModel::PackageTreeModelPrivate>(nodeStorage))
{
    QObject::connect(&nodeStorage, &NodeStorage::nodeAdded, [this](LakosianNode *node, std::any) { // NOLINT
        reload();
        (void) node;
    });

    QObject::connect(&nodeStorage, &NodeStorage::nodeRemoved, [this](LakosianNode *node) {
        reload();
        (void) node;
    });

    QObject::connect(&nodeStorage, &NodeStorage::nodeNameChanged, [this](LakosianNode *node) {
        if (QStandardItem *item = itemForLakosianNode(node)) {
            item->setText(QString::fromStdString(node->name()));
        }
    });
}

QStandardItem *PackageTreeModel::itemForLakosianNode(LakosianNode *node)
{
    std::function<QStandardItem *(QStandardItem *)> recurseFnc = [&](QStandardItem *item) -> QStandardItem * {
        auto itemType =
            NodeType::toDiagramType(static_cast<NodeType::Enum>(item->data(ModelRoles::e_NodeType).toInt()));
        auto itemId = item->data(ModelRoles::e_Id).toLongLong();
        if (node->type() == itemType && node->id() == itemId) {
            return item;
        }

        for (int i = 0; i < item->rowCount(); ++i) {
            if (QStandardItem *foundItem = recurseFnc(item->child(i))) {
                return foundItem;
            }
        }

        return nullptr;
    };

    return recurseFnc(invisibleRootItem());
}

void PackageTreeModel::reload()
{
    QStandardItem *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());

    auto shouldLoadChildren = [](LakosianNode const& node) {
        // Will leave children of components (logical entities) to be lazy loaded
        return node.type() != lvtshr::DiagramType::ComponentType;
    };

    for (auto *topLvlEntity : d->nodeStorage.getTopLevelPackages()) {
        auto *package = dynamic_cast<PackageNode *>(topLvlEntity);
        if (package) {
            auto *item = ModelUtil::createTreeItemFromLakosianNode(*package, shouldLoadChildren);
            root->appendRow(item);
            continue;
        }

        auto *repository = dynamic_cast<RepositoryNode *>(topLvlEntity);
        if (repository) {
            if (repository->name().empty()) {
                continue;
            }
            auto *item = ModelUtil::createTreeItemFromLakosianNode(*repository, shouldLoadChildren);
            root->appendRow(item);
        }
    }
}

void PackageTreeModel::fetchMore(const QModelIndex& parent)
{
    auto childItemsAlreadyLoaded = parent.data(ModelRoles::e_ChildItemsLoaded).toBool();
    if (childItemsAlreadyLoaded) {
        return;
    }

    auto qualifiedName = parent.data(ModelRoles::e_QualifiedName).toString().toStdString();
    auto *parentNode = d->nodeStorage.findByQualifiedName(qualifiedName);
    if (!parentNode) {
        return;
    }

    auto *parentItem = itemForLakosianNode(parentNode);
    if (!parentItem) {
        return;
    }

    for (auto row = parentItem->rowCount() - 1; row >= 0; --row) {
        parentItem->removeRow(row);
    }
    ModelUtil::populateTreeItemChildren(*parentNode, *parentItem, [](LakosianNode const&) {
        return true;
    });
    parentItem->setData(true, ModelRoles::e_ChildItemsLoaded);
}

} // end namespace Codethink::lvtmdl
