// ct_lvtqtw_treeview.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_TREEVIEW
#define INCLUDED_LVTQTW_TREEVIEW

#include <lvtqtw_export.h>

#include <QTreeView>

#include <ct_lvtmdl_basetreemodel.h>

#include <memory>

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT TreeView : public QTreeView
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT

  public:
    // CREATORS
    TreeView(QWidget *parent = nullptr);
    // Constructor

    ~TreeView() noexcept override;
    // Destructor

    void setFilterText(const QString& txt);

    void setModel(QAbstractItemModel *model) override;

    // Signals, auto generated.
    Q_SIGNAL void branchSelected(const QModelIndex& idx);
    // User selected a node that contains more nodes

    Q_SIGNAL void leafSelected(const QModelIndex& idx);
    // User selected a leaf node

    Q_SIGNAL void branchMiddleClicked(const QModelIndex& idx);
    // User clicked on the branch with the middle mouse click

    Q_SIGNAL void leafMiddleClicked(const QModelIndex& idx);
    // User clicked on the leaf with the middle mouse click

    Q_SIGNAL void branchRightClicked(const QModelIndex& idx, const QPoint& pos);
    // User clicked on the branch with the middle mouse click

    Q_SIGNAL void leafRightClicked(const QModelIndex& idx, const QPoint& pos);
    // User clicked on the leaf with the middle mouse click

  protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

  private:
    // DATA
    // TYPES
    struct TreeViewPrivate;
    std::unique_ptr<TreeViewPrivate> d;
};

} // end namespace Codethink::lvtqtw

#endif
