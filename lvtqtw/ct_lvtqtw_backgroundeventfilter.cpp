// ct_lvtqtw_backgroundeventfilter.cpp                               -*-C++-*-

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

#include <ct_lvtqtw_backgroundeventfilter.h>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QBrush>
#include <QDebug>
#include <QEvent>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

namespace Codethink::lvtqtw {

struct BackgroundEventFilter::Private {
    QString backgroundString;
};

BackgroundEventFilter::BackgroundEventFilter(QObject *parent):
    QObject(parent), d(std::make_unique<BackgroundEventFilter::Private>())
{
}

BackgroundEventFilter::~BackgroundEventFilter() = default;

void BackgroundEventFilter::setBackgroundText(const QString& bgString)
{
    d->backgroundString = bgString;
}

bool BackgroundEventFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (d->backgroundString.isEmpty()) {
        return false;
    }

    if (!obj || !ev) {
        return false;
    }

    auto *view = qobject_cast<QWidget *>(obj);
    if (!view) {
        return false;
    }

    auto *viewParent = qobject_cast<QAbstractItemView *>(view->parentWidget());
    if (!viewParent) {
        return false;
    }

    if (ev->type() != QEvent::Paint) {
        if (ev->type() == QEvent::EnabledChange) {
            view->update();
            return false;
        }
        return false;
    }

    auto *model = viewParent->model();
    if (!model) {
        return false;
    }

    if (model->rowCount() != 0) {
        return false;
    }

    QFontMetrics fm(view->font());
    const int width = fm.horizontalAdvance(d->backgroundString);
    const int height = fm.height();
    const int xPos = (view->width() / 2) - (width / 2);
    const int yPos = (view->height() / 2) - (height / 2);

    QPainter painter;
    painter.begin(view);
    painter.setPen(QPen(QBrush(Qt::gray), 2));
    painter.drawText(xPos, yPos, d->backgroundString);
    painter.end();

    return true;
}

} // namespace Codethink::lvtqtw

#include "moc_ct_lvtqtw_backgroundeventfilter.cpp"
