// ct_lvtqtc_pluginmanagerutils.cpp                                  -*-C++-*-

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

#include <QTreeView>
#include <ct_lvtqtc_lakosentitypluginutils.h>
#include <ct_lvtqtc_pluginmanagerutils.h>

namespace Codethink::lvtqtc {

using lvtplg::PluginManager;
using QtUserDataMap = QMap<QString, void *>;

PluginTreeItemHandler PluginManagerQtUtils::createPluginTreeItemHandler(
    PluginManager *pm, QTreeView *treeView, QStandardItemModel *treeModel, QStandardItem *item, GraphicsScene *gs)
{
    auto getPluginData = [pm](auto&& id) {
        return pm->getPluginData(id);
    };
    auto addItem = [pm, treeView, treeModel, item, gs](std::string const& label) {
        auto *child = new QStandardItem(QString::fromStdString(label));
        child->setData(QVariant::fromValue(QtUserDataMap{}));
        item->appendRow(child);
        return createPluginTreeItemHandler(pm, treeView, treeModel, child, gs);
    };
    auto addOnClickAction = [pm, treeView, treeModel, item, gs](
                                std::function<void(PluginTreeItemClickedActionHandler * selectedItem)> const& f) {
        QObject::connect(treeView,
                         &QTreeView::clicked,
                         treeView,
                         [pm, item, f, treeView, treeModel, gs](const QModelIndex& index) {
                             auto *selectedItem = qobject_cast<QStandardItemModel *>(treeModel)->itemFromIndex(index);
                             if (selectedItem != item) {
                                 return;
                             }
                             auto handler = createPluginTreeItemClickedActionHandler(pm, treeView, treeModel, item, gs);
                             f(&handler);
                         });
    };
    auto addUserData = [item](std::string const& dataId, void *userData) {
        auto userDataMap = item->data().value<QtUserDataMap>();
        userDataMap[QString::fromStdString(dataId)] = userData;
        item->setData(QVariant::fromValue(userDataMap));
    };
    auto getUserData = [item](std::string const& dataId) {
        auto userDataMap = item->data().value<QtUserDataMap>();
        return userDataMap[QString::fromStdString(dataId)];
    };
    return PluginTreeItemHandler{getPluginData, addItem, addUserData, getUserData, addOnClickAction};
}

PluginTreeWidgetHandler
PluginManagerQtUtils::createPluginTreeWidgetHandler(PluginManager *pm, std::string const& id, GraphicsScene *gs)
{
    auto getPluginData = [pm](auto&& id) {
        return pm->getPluginData(id);
    };
    auto addRootItem = [pm, id, gs](std::string const& label) -> PluginTreeItemHandler {
        auto *treeView = dynamic_cast<QTreeView *>(pm->getPluginQObject(id + "::view"));
        auto *treeModel = dynamic_cast<QStandardItemModel *>(pm->getPluginQObject(id + "::model"));
        auto *item = new QStandardItem(QString::fromStdString(label));
        item->setData(QVariant::fromValue(QtUserDataMap{}));
        treeModel->invisibleRootItem()->appendRow(item);
        return PluginManagerQtUtils::createPluginTreeItemHandler(pm, treeView, treeModel, item, gs);
    };

    auto clear = [pm, id]() {
        auto *treeModel = dynamic_cast<QStandardItemModel *>(pm->getPluginQObject(id + "::model"));
        treeModel->clear();
    };

    return PluginTreeWidgetHandler{getPluginData, addRootItem, clear};
}

PluginTreeItemClickedActionHandler PluginManagerQtUtils::createPluginTreeItemClickedActionHandler(
    PluginManager *pm, QTreeView *treeView, QStandardItemModel *treeModel, QStandardItem *item, GraphicsScene *gs)
{
    auto getPluginData = [pm](auto&& id) {
        return pm->getPluginData(id);
    };
    auto getItem = [pm, treeView, treeModel, item, gs]() {
        return createPluginTreeItemHandler(pm, treeView, treeModel, item, gs);
    };
    auto getGraphicsView = [gs]() {
        return createPluginGraphicsViewHandler(gs);
    };

    return PluginTreeItemClickedActionHandler{getPluginData, getItem, getGraphicsView};
}

PluginGraphicsViewHandler PluginManagerQtUtils::createPluginGraphicsViewHandler(GraphicsScene *gs)
{
    auto getEntityByQualifiedName = [gs](std::string const& qualifiedName) -> std::optional<Entity> {
        auto *e = gs->entityByQualifiedName(qualifiedName);
        if (!e) {
            return std::nullopt;
        }
        return createWrappedEntityFromLakosEntity(e);
    };
    auto getVisibleEntities = [gs]() {
        auto entities = std::vector<Entity>{};
        for (auto&& e : gs->allEntities()) {
            entities.push_back(createWrappedEntityFromLakosEntity(e));
        }
        return entities;
    };
    auto getEdgeByQualifiedName = [gs](std::string const& fromQualifiedName,
                                       std::string const& toQualifiedName) -> std::optional<Edge> {
        auto *fromEntity = gs->entityByQualifiedName(fromQualifiedName);
        if (!fromEntity) {
            return std::nullopt;
        }
        auto *toEntity = gs->entityByQualifiedName(toQualifiedName);
        if (!toEntity) {
            return std::nullopt;
        }
        return createWrappedEdgeFromLakosEntity(fromEntity, toEntity);
    };

    return PluginGraphicsViewHandler{getEntityByQualifiedName, getVisibleEntities, getEdgeByQualifiedName};
}

} // namespace Codethink::lvtqtc
