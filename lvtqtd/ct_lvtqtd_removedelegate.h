// ct_lvtqtw_removedelegate.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_REMOVEDELEGATE
#define INCLUDED_LVTQTW_REMOVEDELEGATE

#include <lvtqtd_export.h>

#include <QStyledItemDelegate>
#include <memory>

class QPainter;
class QStyleOptionViewItem;
class QModelIndex;

namespace Codethink::lvtqtd {

class LVTQTD_EXPORT RemoveDelegate : public QStyledItemDelegate
// Adds a Delete button on the right side of the
// cell, and calls the removeRow(idx) on the model
{
    Q_OBJECT

  public:
    // CREATORS
    RemoveDelegate(QObject *parent = nullptr);
    // Constructor

    ~RemoveDelegate() noexcept override;
    // Destructor

    void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtd

#endif
