// ct_lvtqtc_tooltip.cpp                                              -*-C++-*-

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

#include <ct_lvtqtc_tooltip.h>

#include <QBrush>
#include <QDebug>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

#include <preferences.h>

namespace Codethink::lvtqtc {

// TODO: Make it configurable
constexpr int ICON_SIZE = 5;

struct ToolTipItem::Private {
    using ToolTip = QPair<QLabel *, QLabel *>;
    QVector<ToolTip> toolTips;
    ToolTip entryToolTip;
    QLabel *title = nullptr;
    QGridLayout *layout = nullptr;
    QPoint movementDelta;
    bool isDragging = false;
};

ToolTipItem::ToolTipItem(QWidget *parent): QWidget(parent), d(std::make_unique<ToolTipItem::Private>())
{
    d->title = new QLabel(tr("Information"), this),

    d->entryToolTip.first = nullptr;
    d->entryToolTip.second = nullptr;
    d->layout = new QGridLayout();
    d->layout->setSizeConstraint(QLayout::SetFixedSize);
    d->layout->addWidget(d->title, 2, 1, Qt::AlignmentFlag::AlignHCenter);
    setLayout(d->layout);

    setStyleSheet("QLabel { color: white; }");
}

void ToolTipItem::addToolTip(const QString& toolTip, const QIcon& icon, const QPixmap& pixmap)
{
    const int currRowCount = d->layout->rowCount();
    auto *iconLabel = new QLabel();

    if (!icon.isNull()) {
        iconLabel->setPixmap(icon.pixmap(ICON_SIZE, ICON_SIZE));
        d->layout->addWidget(iconLabel, currRowCount, 0);
    } else if (!pixmap.isNull()) {
        iconLabel->setPixmap(pixmap);
        d->layout->addWidget(iconLabel, currRowCount, 0);
    }

    auto *textLabel = new QLabel();
    textLabel->setText(toolTip);
    d->layout->addWidget(textLabel, currRowCount, 1);

    d->toolTips.push_back(qMakePair(iconLabel, textLabel));
}

void ToolTipItem::clear()
{
    for (auto t : qAsConst(d->toolTips)) {
        delete t.first;
        delete t.second;
    }
    d->toolTips.clear();
}

ToolTipItem::~ToolTipItem()
{
    clear();
}

void ToolTipItem::persistPos()
{
    // TODO: Implement - me.
}

void ToolTipItem::readPos()
{
    // TODO: Implement me.
}

void ToolTipItem::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);
    painter.setBrush(QBrush(QColor(50, 50, 50, 200)));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, geometry().width(), geometry().height(), 10, 10);
    QWidget::paintEvent(ev);
}

void ToolTipItem::mousePressEvent(QMouseEvent *ev)
{
    if (ev->modifiers() & Qt::Modifier(Preferences::dragModifier())) {
        QLine line(mapToParent(ev->pos()), pos());
        d->movementDelta.setX(line.dx());
        d->movementDelta.setY(line.dy());
        d->isDragging = true;
    }
}

void ToolTipItem::mouseMoveEvent(QMouseEvent *ev)
{
    if (!d->isDragging) {
        return;
    }

    move(mapToParent(ev->pos()) + d->movementDelta);
}

void ToolTipItem::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    d->isDragging = false;
}

} // end namespace Codethink::lvtqtc

#include "moc_ct_lvtqtc_tooltip.cpp"
