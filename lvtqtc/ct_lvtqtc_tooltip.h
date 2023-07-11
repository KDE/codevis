// ct_lvtqtc_tooltip.h                                                 -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTC_TOOLTIP
#define DEFINED_CT_LVTQTC_TOOLTIP

#include <lvtqtc_export.h>

#include <QElapsedTimer>
#include <QIcon>
#include <QLabel>
#include <QPair>
#include <QRectF>
#include <QVector>
#include <QWidget>

#include <memory>

namespace Codethink::lvtqtc {
/* To use a tooltip, simply ->setToolTip on the QGraphicsItem that you want
 * or, if it's a "global" tooltip, set it on the mouseMoveEvent of the ProfileGraphicsView.
 */

class LVTQTC_EXPORT ToolTipItem : public QWidget {
    Q_OBJECT
  public:
    enum Status { COLLAPSED, EXPANDED };

    explicit ToolTipItem(QWidget *parent = nullptr);
    ~ToolTipItem() override;

    void clear();
    void addToolTip(const QString& toolTip, const QIcon& icon = QIcon(), const QPixmap& pixmap = QPixmap());
    void persistPos();
    void readPos();

    void paintEvent(QPaintEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtc

#endif
