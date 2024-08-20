// ct_lvtqtw_treeview.cpp                                            -*-C++-*-

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

#include <ct_lvtqtw_treeview.h>

#include <ct_lvtmdl_basetreemodel.h>
#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtmdl_treefiltermodel.h>

#include <QDebug>
#include <QMouseEvent>
#include <QSortFilterProxyModel>

#include <QDrag>
#include <QMimeData>
#include <qnamespace.h>

namespace Codethink::lvtqtw {

struct TreeView::TreeViewPrivate {
    Codethink::lvtmdl::BaseTreeModel *model = nullptr;
    Codethink::lvtmdl::TreeFilterModel *filterModel = nullptr;
    QPoint mousePressPos;
};

// --------------------------------------------
// class TreeView
// --------------------------------------------

TreeView::TreeView(QWidget *parent): QTreeView(parent), d(std::make_unique<TreeView::TreeViewPrivate>())
{
    // Sorting the table loads the data, but also messes up the positions.
    // not sorting the table does not load the data.
    setSortingEnabled(false);
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    connect(this, &QTreeView::clicked, this, [this](const QModelIndex& idx) {
        auto isBranch = idx.data(lvtmdl::ModelRoles::e_IsBranch).value<bool>();

        if (isBranch) {
            Q_EMIT branchSelected(idx);
        } else {
            Q_EMIT leafSelected(idx);
        }
    });

    d->filterModel = new lvtmdl::TreeFilterModel();
}

TreeView::~TreeView() noexcept = default;

void TreeView::setModel(QAbstractItemModel *model)
{
    auto *base_model = qobject_cast<lvtmdl::BaseTreeModel *>(model);
    assert(base_model);

    d->model = base_model;
    d->filterModel->setSourceModel(d->model);

    connect(this, &QTreeView::expanded, model, &QAbstractItemModel::fetchMore);

    QTreeView::setModel(d->filterModel);
}

void TreeView::mousePressEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::MouseButton::MiddleButton) {
        const QModelIndex& idx = indexAt(ev->pos());
        if (!idx.isValid()) {
            return;
        }
        auto isBranch = idx.data(lvtmdl::ModelRoles::e_IsBranch).value<bool>();
        if (isBranch) {
            Q_EMIT branchMiddleClicked(idx);
        } else {
            Q_EMIT leafMiddleClicked(idx);
        }
    } else if (ev->buttons() & Qt::MouseButton::RightButton) {
        const QModelIndex& idx = indexAt(ev->pos());
        if (!idx.isValid()) {
            return;
        }

        auto isBranch = idx.data(lvtmdl::ModelRoles::e_IsBranch).value<bool>();
        if (isBranch) {
            Q_EMIT branchRightClicked(selectedIndexes(), idx, ev->globalPos());
        } else {
            Q_EMIT leafRightClicked(selectedIndexes(), idx, ev->globalPos());
        }
    }
    d->mousePressPos = ev->pos();

    QTreeView::mousePressEvent(ev);
}

void TreeView::setFilterText(const QString& txt)
{
    d->filterModel->setFilter(txt);
}

void TreeView::mouseReleaseEvent(QMouseEvent *ev)
{
    QTreeView::mouseReleaseEvent(ev);
    d->mousePressPos = QPoint{};
}

void TreeView::keyPressEvent(QKeyEvent *e)
{
    QTreeView::keyPressEvent(e);
    if (e->key() == Qt::Key_Escape) {
        clearSelection();
    }
}

} // namespace Codethink::lvtqtw

#include "moc_ct_lvtqtw_treeview.cpp"
