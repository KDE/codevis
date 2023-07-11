// ct_lvtqtw_tool_zoom.cpp                                           -*-C++-*-

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
#include <ct_lvtqtc_tool_zoom.h>

#include <ct_lvtqtc_iconhelpers.h>

#include <QMouseEvent>

namespace Codethink::lvtqtc {

ToolZoom::ToolZoom(GraphicsView *gv):
    ToolRubberband(tr("Rubberband Zoom"),
                   tr("Click and drag to zoom to selected area"),
                   IconHelpers::iconFrom(":/icons/zoom_selection"),
                   gv)
{
    setRubberbandPen(QPen(QBrush(QColor(0, 0, 255, 135)), 1));
    setRubberbandBrush(QBrush(QColor(0, 0, 255, 135)));
}

ToolZoom::~ToolZoom() noexcept = default;

void ToolZoom::rubberbandChanged(const QPoint& topLeft, const QPoint& bottomRight)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    // no op.
}

void ToolZoom::rubberbandFinished(const QPoint& topLeft, const QPoint& bottomRight)
{
    graphicsView()->zoomIntoRect(topLeft, bottomRight);
}

} // namespace Codethink::lvtqtc
