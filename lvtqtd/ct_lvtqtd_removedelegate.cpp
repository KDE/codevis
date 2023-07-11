#include <ct_lvtqtd_removedelegate.h>

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

#include <QApplication>
#include <QDebug>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionViewItem>

namespace Codethink::lvtqtd {
struct RemoveDelegate::Private { };

RemoveDelegate::RemoveDelegate(QObject *parent):
    QStyledItemDelegate(parent), d(std::make_unique<RemoveDelegate::Private>())
{
}

RemoveDelegate::~RemoveDelegate() noexcept = default;

void RemoveDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if (!index.isValid()) {
        return;
    }

    /////////////////////////////////////////////////////////////////
    //     I have a Rect on the QComboBox like this:
    //     ########################
    //     # TEXT HERE            #
    //     ########################
    //
    //     and I want to put a button on the right side of the box,
    //     since I want the button to be a square, I use the height
    //     dimensions for it.
    //     This will generate a button like
    //
    //     ####
    //     #  #
    //     ####
    //
    //    now I need to tell the painter where to paint this.
    //    For this, I get the option.rect (that gives me the full rect)
    //    the new X of this rect will be the starting point of this button,
    //    that's x() + (width() - buttonWidth()) - but the buttonWidth is
    //    the same as the rect.height()
    //    So I end up with
    //
    //    #########################
    //    # TEXT HERE          #  #
    //    #########################
    ////////////////////////////////////////////////////////////////////

    QRect rect = option.rect.adjusted(1, 1, -1, -1);
    rect.setX(rect.x() + rect.width() - rect.height());
    rect.setWidth(rect.height());

    QStyleOptionButton opt;
    opt.state |= QStyle::State_Enabled;
    opt.rect = rect;
    opt.text = QStringLiteral("X");
    QApplication::style()->drawControl(QStyle::CE_PushButton, &opt, painter, nullptr);
}

bool RemoveDelegate::editorEvent(QEvent *event,
                                 QAbstractItemModel *model,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        const auto pos = mouseEvent->localPos().toPoint();

        QRect rect = option.rect.adjusted(1, 1, -1, -1);
        rect.setX(rect.x() + rect.width() - rect.height());
        rect.setWidth(rect.height());

        if (rect.contains(pos)) {
            model->removeRow(index.row());
            return true; // event is handled.
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
} // namespace Codethink::lvtqtd
