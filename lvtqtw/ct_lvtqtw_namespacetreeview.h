// ct_lvtqtw_namespacetreeview.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_NAMESPACETREEVIEW
#define INCLUDED_LVTQTW_NAMESPACETREEVIEW

#include <lvtqtw_export.h>

#include <ct_lvtqtw_treeview.h>

#include <ct_lvtmdl_basetreemodel.h>

#include <memory>

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT NamespaceTreeView : public TreeView
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT

  public:
    // CREATORS
    NamespaceTreeView(QWidget *parent = nullptr);
    // Constructor

    ~NamespaceTreeView() noexcept override;
    // Destructor

    Q_SLOT void createContextMenu(const QPoint& point);
    // Invoked by the right click

    void setModel(QAbstractItemModel *model) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;

    [[nodiscard]] QList<QAction *> createActionsForNamespaces(const QModelIndex& idx) const;
    [[nodiscard]] QList<QAction *> createActionsForNoElement(const QModelIndex& idx) const;
    // Those calls are called inside of the setContextMenu, to create
    // the list of choices.
};

} // namespace Codethink::lvtqtw

#endif
