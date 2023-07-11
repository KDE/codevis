// ct_lvtqtw_tool_rubberband.cpp                                                                               -*-C++-*-

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

#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_tool_rubberband.h>
#include <ct_lvtshr_functional.h>

#include <QBrush>
#include <QDebug>
#include <QMouseEvent>
#include <QPen>

namespace Codethink::lvtqtc {

struct ToolRubberband::Private {
    bool rubberBandActive = false;
    QPoint rubberBandStart;
    QPoint rubberBandEnd;

    QBrush brush;
    QPen pen;
};

ToolRubberband::ToolRubberband(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv):
    ITool(name, tooltip, icon, gv), d(std::make_unique<ToolRubberband::Private>())
{
}

ToolRubberband::~ToolRubberband() noexcept = default;

void ToolRubberband::mousePressEvent(QMouseEvent *event)
{
    d->rubberBandActive = true;
    d->rubberBandStart = event->pos();
}

void ToolRubberband::mouseMoveEvent(QMouseEvent *event)
{
    if (!d->rubberBandActive) {
        return;
    }

    d->rubberBandEnd = event->pos();
    const int left = std::min(d->rubberBandStart.x(), d->rubberBandEnd.x());
    const int right = std::max(d->rubberBandStart.x(), d->rubberBandEnd.x());
    const int top = std::min(d->rubberBandStart.y(), d->rubberBandEnd.y());
    const int bottom = std::max(d->rubberBandStart.y(), d->rubberBandEnd.y());
    rubberbandChanged(QPoint(left, top), QPoint(right, bottom));
    graphicsView()->viewport()->update();
}

void ToolRubberband::mouseReleaseEvent(QMouseEvent *event)
{
    using Codethink::lvtshr::ScopeExit;
    ScopeExit _([&]() {
        deactivate();
    });

    if (!d->rubberBandActive) {
        return;
    }

    const int left = std::min(d->rubberBandStart.x(), d->rubberBandEnd.x());
    const int right = std::max(d->rubberBandStart.x(), d->rubberBandEnd.x());
    const int top = std::min(d->rubberBandStart.y(), d->rubberBandEnd.y());
    const int bottom = std::max(d->rubberBandStart.y(), d->rubberBandEnd.y());
    rubberbandFinished(QPoint(left, top), QPoint(right, bottom));
    event->accept();
}

void ToolRubberband::deactivate()
{
    d->rubberBandActive = false;
    graphicsView()->viewport()->update();
    ITool::deactivate();
}

void ToolRubberband::drawForeground(QPainter *painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!d->rubberBandActive) {
        return;
    }

    painter->save();
    painter->setWorldMatrixEnabled(false);
    painter->setPen(d->pen);
    painter->setBrush(d->brush);
    painter->drawRect(QRect(d->rubberBandStart, d->rubberBandEnd));
    painter->setWorldMatrixEnabled(true);
    painter->restore();
}

void ToolRubberband::setRubberbandBrush(const QBrush& brush)
{
    d->brush = brush;
}

void ToolRubberband::setRubberbandPen(const QPen& pen)
{
    d->pen = pen;
}

} // namespace Codethink::lvtqtc
