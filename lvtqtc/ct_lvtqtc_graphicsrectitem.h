// ct_lvtqtc_graphicsrectitem.h                                        -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTC_GRAPHICSRECTITEM
#define DEFINED_CT_LVTQTC_GRAPHICSRECTITEM

#include <lvtqtc_export.h>

#include <QGraphicsRectItem>
#include <QObject>

#include <memory>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT GraphicsRectItem : public QObject, public QGraphicsRectItem {
    // QGraphicsRectItem plus extra functionality
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ rect WRITE setRectangle NOTIFY rectangleChanged)

  public:
    explicit GraphicsRectItem(QGraphicsItem *parent = nullptr);
    ~GraphicsRectItem() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setRectangle(const QRectF& rect);

    [[nodiscard]] QPainterPath shape() const override;

    void setRoundRadius(qreal radius);
    [[nodiscard]] qreal roundRadius() const;

    void setXRadius(qreal radius);
    void setYRadius(qreal radius);

    void setNumOutlines(int numOutlines);

    Q_SIGNAL void rectangleChanged();
    // setRectangle was called.

  private:
    using QGraphicsRectItem::setRect;

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
