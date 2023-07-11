// ct_lvtqtc_graphicsrectitem.cpp                                      -*-C++-*-

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

#include <ct_lvtqtc_graphicsrectitem.h>

#include <ct_lvtqtc_util.h>

#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QVector2D>

namespace {

constexpr int OUTLINE_ADJUSTMENT = 3;
// the amount we adjust each successive outline rectangle
} // namespace

namespace Codethink::lvtqtc {

struct GraphicsRectItem::Private {
    qreal xRadius = 5;
    qreal yRadius = 5;
    int numOutlines = 1;
    QPainterPath shapeRect;
};

GraphicsRectItem::GraphicsRectItem(QGraphicsItem *parent):
    QGraphicsRectItem(parent), d(std::make_unique<GraphicsRectItem::Private>())
{
}

GraphicsRectItem::~GraphicsRectItem() = default;

void GraphicsRectItem::setRoundRadius(qreal radius)
{
    d->xRadius = radius;
    d->yRadius = radius;
    update();
}

void GraphicsRectItem::setRectangle(const QRectF& r)
{
    if (rect() == r) {
        return;
    }

    d->shapeRect = QPainterPath();
    d->shapeRect.addRoundedRect(r, d->xRadius, d->yRadius);
    setRect(r);

    Q_EMIT rectangleChanged();
}

qreal GraphicsRectItem::roundRadius() const
{
    return d->xRadius;
    // if we are treating both radius as the same, then just return one.
}

void GraphicsRectItem::setXRadius(qreal radius)
{
    d->xRadius = radius;
    update();
}

void GraphicsRectItem::setYRadius(qreal radius)
{
    d->yRadius = radius;
    update();
}

void GraphicsRectItem::setNumOutlines(int numOutlines)
{
    d->numOutlines = numOutlines;
    update();
}

void GraphicsRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setPen(pen());
    painter->setBrush(brush());

    for (int i = 0; i < d->numOutlines; i++) {
        const int adj = OUTLINE_ADJUSTMENT * i;
        QRectF adjRect = rect().adjusted(adj, adj, -adj, -adj);

        painter->drawRoundedRect(adjRect, d->xRadius, d->yRadius);
    }

    if (option->state.testFlag(QStyle::State_Selected)) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QBrush(QColor(Qt::black)), 1, Qt::PenStyle::DashLine));
        painter->drawRect(rect());
    }

    painter->restore();
}

QPainterPath GraphicsRectItem::shape() const
{
    return d->shapeRect;
}

} // end namespace Codethink::lvtqtc
