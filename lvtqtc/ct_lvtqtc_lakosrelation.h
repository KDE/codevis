// ct_lvtqtc_lakosrelation.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_LAKOSRELATION
#define INCLUDED_CT_LVTGRPS_LAKOSRELATION

#include <lvtqtc_export.h>

#include <QGraphicsLineItem>

#include <memory>

#include <QBrush>
#include <QPen>
#include <QPointF>

#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_util.h>

#include <ct_lvtshr_graphenums.h>

class QUndoCommand;

namespace Codethink::lvtqtc {

struct Vertex;
class LakosEntity;

/*! \class LakosRelation lakos_relation.cpp lakos_relation.h
 *  \brief Represents and draws a Lakos Relation
 *
 * %LakosRelation draws a Lakos Relation
 */
class LVTQTC_EXPORT LakosRelation : public QObject, public QGraphicsItem {
    Q_OBJECT
  public:
    LakosRelation(LakosEntity *source, LakosEntity *target);
    ~LakosRelation() override;

    void setColor(QColor const& newColor);
    void setStyle(Qt::PenStyle const& newStyle);

    /*! \brief The type of a particular instance of sub class LakosRelation
     */
    [[nodiscard]] virtual lvtshr::LakosRelationType relationType() const = 0;

    /*! \brief Returns a string of the sub class of LakosRelation
     */
    [[nodiscard]] virtual std::string relationTypeAsString() const = 0;

    [[nodiscard]] QRectF boundingRect() const override;

    enum { Type = QtcUtil::LAKOSRELATION_TYPE };
    // for qgraphicsitem_cast magic

    [[nodiscard]] inline int type() const override
    // for qgraphicsitem_cast magic
    {
        return Type;
    }

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    [[nodiscard]] QLineF line() const;
    void setLine(const QLineF& line);
    void setLine(qreal x1, qreal y1, qreal x2, qreal y2);

    [[nodiscard]] QLineF adjustedLine() const;

    static QGraphicsPathItem *defaultArrow();
    static QGraphicsPathItem *defaultTail();
    static QGraphicsPathItem *diamondArrow();

    [[nodiscard]] LakosEntity *from() const;
    [[nodiscard]] LakosEntity *to() const;

    void toggleRelationFlags(EdgeCollection::RelationFlags flags, bool toggle);

    void setShouldBeHidden(bool hidden);
    // controls the visibility, has higher precedence than isHidden.
    // this is used to block some data from being displayed.

    bool shouldBeHidden() const;

    virtual void populateMenu(QMenu& menu, QMenu *debugMenu);

    std::pair<QPainterPath, QPainterPath> calculateIntersection(QGraphicsItem *lhs, QGraphicsItem *rhs);

    [[nodiscard]] QPainterPath shape() const override;

    void updateTooltip();
    [[nodiscard]] std::string legendText() const;
    // Returns a string which summarises what this thing is

    // Debug methods
    static void toggleBoundingRect();
    static void toggleShape();
    static void toggleTextualInformation();
    static void toggleIntersectionPaths();
    static void toggleOriginalLine();
    static bool showBoundingRect();
    static bool showShape();
    static bool showTextualInformation();
    static bool showIntersectionPaths();
    static bool showOriginalLine();

    // fist the static methods above, then call this for each LakosRelation.
    void updateDebugInformation();

    Q_SIGNAL void undoCommandCreated(QUndoCommand *command);
    // A new undo command is created for his edge.

    Q_SIGNAL void requestRemoval();

  protected:
    void setDashed(bool dashed);

    void setThickness(qreal thickness);
    // set the thickness of the line. defaults to 0.5, a hair-thin line.

    void setHead(QGraphicsPathItem *head);
    // This assumes that the item is pointed upwards
    // and that the origin point is at the mid-right

    void setTail(QGraphicsPathItem *tail);
    // this assumes that the item is pointed upwards
    // and that the origin point is at the mid-right

    virtual QColor hoverColor() const;

  private:
    QColor overrideColor() const;
    void setInnerBrushColor(QGraphicsPathItem *innerItem, const QColor& color);
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtc

#endif
