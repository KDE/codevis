// ct_lvtqtw_namespacetreeview.cpp                               -*-C++-*-

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
#include <ct_lvtqtw_namespacetreeview.h>

#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtmdl_namespacetreemodel.h>

#include <QMenu>

namespace Codethink::lvtqtw {

struct NamespaceTreeView::Private {
    lvtmdl::NamespaceTreeModel *model;
};

NamespaceTreeView::NamespaceTreeView(QWidget *parent):
    TreeView(parent), d(std::make_unique<NamespaceTreeView::Private>())
{
    setHeaderHidden(false);
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    connect(this, &QTreeView::customContextMenuRequested, this, &NamespaceTreeView::createContextMenu);
}

NamespaceTreeView::~NamespaceTreeView() noexcept = default;

void NamespaceTreeView::createContextMenu(const QPoint& point)
{
    QModelIndex idx = indexAt(point);
    if (!idx.isValid()) {
        return;
    }

    QMenu menu;
    menu.addActions(createActionsForNamespaces(idx));
    menu.addActions(createActionsForNoElement(idx));

    if (!menu.actions().empty()) {
        menu.exec(viewport()->mapToGlobal(point));
    }
}

void NamespaceTreeView::setModel(QAbstractItemModel *model)
{
    d->model = qobject_cast<lvtmdl::NamespaceTreeModel *>(model);
    TreeView::setModel(model);
}

QList<QAction *> NamespaceTreeView::createActionsForNamespaces(const QModelIndex& idx) const
{
    if (!idx.data(lvtmdl::ModelRoles::e_IsBranch).value<bool>()) {
        return {};
    }

    if (d->model->rootNamespace()) {
        return {};
    }

    auto *action = new QAction(tr("Set as Root Namespace"));
    action->setData(idx.row());
    connect(action, &QAction::triggered, this, [this, idx] {
        d->model->setRootNamespace(idx.data().toString());
    });

    return {action};
}

QList<QAction *> NamespaceTreeView::createActionsForNoElement(const QModelIndex& idx) const
{
    Q_UNUSED(idx);

    if (!d->model->rootNamespace()) {
        return {};
    }

    auto *action = new QAction(tr("Use Global Namespace"));
    connect(action, &QAction::triggered, this, [this] {
        d->model->setRootNamespace({});
    });

    return {action};
}

} // namespace Codethink::lvtqtw

#include "moc_ct_lvtqtw_namespacetreeview.cpp"
