// ct_lvtqtw_graphicsview.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_MINIMAP
#define INCLUDED_LVTQTC_MINIMAP

#include <lvtqtc_export.h>

#include <QGraphicsView>
#include <memory>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT Minimap : public QGraphicsView
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT
  public:
    enum MinimapSize {
        MINIMAP_SMALL = 10, // 10% of the screen size.
        MINIMAP_MEDIUM = 7, // ~15% 0f the screen size
        MINIMAP_LARGE = 5 // 20% of the screen size
    };

    Minimap(QGraphicsScene *mainScene, QWidget *parent);
    ~Minimap() override;

    Q_SIGNAL void focusSceneAt(const QPointF& pos);
    void setSceneRect(const QRectF& rect);
    void setMapSize(MinimapSize size);
    void calculateGeometry();

  protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void drawForeground(QPainter *painter, const QRectF& rect) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
