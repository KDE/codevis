// ct_lvtqtw_tool_rubberband.h                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_RUBBERBAND
#define INCLUDED_LVTQTC_TOOL_RUBBERBAND

#include <ct_lvtqtc_itool.h>
#include <lvtqtc_export.h>

#include <QGraphicsView>

#include <memory>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT ToolRubberband : public ITool {
    // Controls the Rubberband, for rubberband enabled tools.
    // should be used when you want to display a square drag control
    // via the mouse.
    // userful for zoom, selection and perhaps a few other things.

    Q_OBJECT
  public:
    ~ToolRubberband() noexcept override;

    void setRubberbandBrush(const QBrush& brush);
    void setRubberbandPen(const QPen& pen);

    virtual void rubberbandChanged(const QPoint& topLeft, const QPoint& bottomRight) = 0;
    virtual void rubberbandFinished(const QPoint& topLeft, const QPoint& bottomRight) = 0;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF& rect) override;
    void deactivate() override;

  protected:
    ToolRubberband(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv);

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
