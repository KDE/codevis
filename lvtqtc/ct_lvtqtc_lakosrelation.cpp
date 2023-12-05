// ct_lvtqtc_lakosrelation.cpp                                      -*-C++-*-

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

#include <ct_lvtqtc_lakosrelation.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_inspect_dependency_window.h>
#include <ct_lvtqtc_lakosentity.h>

#include <QDebug>
#include <QDialog>
#include <QFileInfo>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QLayout>
#include <QLine>
#include <QMenu>
#include <QPainter>
#include <QUrl>
#include <QtMath>

#include "preferences.h"

namespace {

constexpr int DEBUG_PATH_LAYER = 100;

// static, we share those for every instance of the edges.
// 5 bytes less per instance, and sometimes we have *lots* of instances.
bool s_showBoundingRect = false; // NOLINT
bool s_showShape = false; // NOLINT
bool s_showTextualInformation = false; // NOLINT
bool s_showIntersectionPaths = false; // NOLINT
bool s_showOriginalLine = false; // NOLINT

QPointF closestPointTo(const QPointF& target, const QPainterPath& sourcePath)
// Returns the closest element (position) in \a sourcePath to \a target,
// using \l{QPoint::manhattanLength()} to determine the distances.
{
    if (sourcePath.isEmpty()) {
        return {};
    }

    qreal comparedLength = std::numeric_limits<int>::max();

    QPointF foundDistance;
    for (int i = 0; i < sourcePath.elementCount(); ++i) {
        const qreal length = QPointF(sourcePath.elementAt(i) - target).manhattanLength();
        if (length < comparedLength) {
            foundDistance = sourcePath.elementAt(i);
            comparedLength = length;
        }
    }

    return foundDistance;
}

} // namespace

namespace Codethink::lvtqtc {

struct LakosRelation::Private {
    QLineF line;
    // This is the line, as set by setLine
    QLineF adjustedLine;
    // This is the line, after the adjusts we need to do
    // to take into consideration the size of the tail item
    // head item, and any boundary that we might intercept.

    QGraphicsPathItem *head = nullptr;
    // the graphicsitem on the .p2() of the line

    QGraphicsPathItem *tail = nullptr;
    // the graphicsitem on the .p1() of the line

    LakosEntity *pointsFrom = nullptr;
    LakosEntity *pointsTo = nullptr;
    // the Nodes that this edge connects

    int relationFlags = EdgeCollection::RelationFlags::RelationFlagsNone;
    // We use the information below to paint the item.

    int selectedCounter = 0;
    // Keep track on how many selections this relation has.
    // This is used to avoid unselecting something that should be selected.

    qreal thickness = 0.5;
    // thickness of the line being drawn.

    bool dashed = false;
    Qt::PenStyle penStyle = Qt::PenStyle::SolidLine;

    bool shouldBeHidden = false;
    // This will not be touched by Qt when we show() or hide()
    // an item, so it's an way to *really* hide the items.
    // TODO: create a StateMachine to control visibility.

    QGraphicsPathItem *fromIntersectionItem = nullptr;
    QGraphicsPathItem *toIntersectionItem = nullptr;

    QPainterPath fromIntersection;
    QPainterPath toIntersection;

    QRectF boundingRect;
    QPainterPath shape;
    // used to return the `shape()` call.
    // also used to draw the selection area.

    QColor color = QColor(0, 0, 0);
    QColor highlightColor = QColor(255, 0, 0);
};

LakosRelation::LakosRelation(LakosEntity *source, LakosEntity *target): d(std::make_unique<LakosRelation::Private>())
{
    d->pointsFrom = source;
    d->pointsTo = target;

    d->fromIntersectionItem = new QGraphicsPathItem(d->fromIntersection, this);
    d->toIntersectionItem = new QGraphicsPathItem(d->toIntersection, this);
    d->fromIntersectionItem->setZValue(DEBUG_PATH_LAYER);
    d->toIntersectionItem->setZValue(DEBUG_PATH_LAYER);
    d->fromIntersectionItem->setVisible(false);
    d->toIntersectionItem->setVisible(false);

    d->color = Preferences::edgeColor();
    d->highlightColor = Preferences::highlightEdgeColor();
    connect(Preferences::self(), &Preferences::edgeColorChanged, this, [this] {
        d->color = Preferences::edgeColor();
        update();
    });
    connect(Preferences::self(), &Preferences::highlightEdgeColorChanged, this, [this] {
        d->highlightColor = Preferences::highlightEdgeColor();
        update();
    });
}

LakosRelation::~LakosRelation() = default;

void LakosRelation::setColor(QColor const& newColor)
{
    if (newColor == d->color) {
        return;
    }

    d->color = newColor;
    update();
}

void LakosRelation::setStyle(Qt::PenStyle const& newStyle)
{
    if (newStyle == d->penStyle) {
        return;
    }
    d->penStyle = newStyle;
    update();
}

QRectF LakosRelation::boundingRect() const
{
    return d->boundingRect;
}

void LakosRelation::setHead(QGraphicsPathItem *head)
{
    d->head = head;
    d->head->setParentItem(this);
    d->head->setPos(d->line.p2());
    QPen currentPen = head->pen();
    currentPen.setCosmetic(true);
    head->setPen(currentPen);
}

void LakosRelation::setTail(QGraphicsPathItem *tail)
{
    d->tail = tail;
    d->tail->setParentItem(this);
    d->tail->setPos(d->line.p1());

    QPen currentPen = tail->pen();
    currentPen.setCosmetic(true);
    tail->setPen(currentPen);
}

std::pair<QPainterPath, QPainterPath> LakosRelation::calculateIntersection(QGraphicsItem *lhs, QGraphicsItem *rhs)
// creates a intersection area from the origin point up to
// the boundary of the element with 1px wide, with minimum of
// 4 points, that are going to be used on the closestTo function
// above.
// The closest point is then returned as the 'hit' point - the point
// that the line intersects to.
{
    // silence scanbuild.
    if (!lhs || !rhs) {
        return {};
    }

    QLineF pathAsLine = d->line;

    // Extend the first point in the path out by 1 pixel.
    QLineF startEdge = pathAsLine.normalVector();
    startEdge.setLength(1);

    // Swap the points in the line so the normal vector is at the other end of the line.
    pathAsLine.setPoints(pathAsLine.p2(), pathAsLine.p1());
    QLineF endEdge = pathAsLine.normalVector();

    // The end point is currently pointing the wrong way; move it to face the same
    // direction as startEdge.
    endEdge.setLength(-1);

    // Now we can create a rectangle from our edges.
    QPainterPath rectPath(startEdge.p1());
    rectPath.lineTo(startEdge.p2());
    rectPath.lineTo(endEdge.p2());
    rectPath.lineTo(endEdge.p1());
    rectPath.lineTo(startEdge.p1());

    // Map the path to global coordinates
    rectPath = mapToScene(rectPath);

    // Calculate things on the global coordinates
    const QPainterPath lhsIntersection = lhs->mapToScene(lhs->shape()).intersected(rectPath);
    const QPainterPath rhsIntersection = rhs->mapToScene(rhs->shape()).intersected(rectPath);

    // map the results to Local coordinates
    return {mapFromScene(lhsIntersection), mapFromScene(rhsIntersection)};
}

void LakosRelation::setLine(const QLineF& line)
{
    prepareGeometryChange();

    d->line = line;

    if (!d->pointsFrom->isVisible() && !d->pointsTo->isVisible()) {
        if (d->pointsFrom->parentItem() == d->pointsTo->parentItem()) {
            setVisible(false);
            return;
        }
    }

    QGraphicsItem *lhs = d->pointsFrom->isVisible() ? d->pointsFrom : d->pointsFrom->parentItem();
    QGraphicsItem *rhs = d->pointsTo->isVisible() ? d->pointsTo : d->pointsTo->parentItem();
    assert(lhs);
    assert(rhs);

    auto [intersection1, intersection2] = calculateIntersection(lhs, rhs);

    d->fromIntersection = intersection1;
    d->toIntersection = intersection2;

    d->fromIntersectionItem->setPath(d->fromIntersection);
    d->toIntersectionItem->setPath(d->toIntersection);

    // The hit position will be the element (point) of the rectangle that is the
    // closest to where the projectile was fired from.
    auto hitPoint1 = closestPointTo(line.p2(), intersection1);
    auto hitPoint2 = closestPointTo(line.p1(), intersection2);

    d->adjustedLine.setP1(hitPoint1);
    d->adjustedLine.setP2(hitPoint2);

    static const qreal kClickTolerance = 10;

    QPointF vec = d->adjustedLine.p2() - d->adjustedLine.p1();
    vec = vec * (kClickTolerance / std::sqrt(QPointF::dotProduct(vec, vec)));

    QPointF orthogonal(vec.y(), -vec.x());

    QPainterPath result(d->adjustedLine.p1() - vec + orthogonal);
    result.lineTo(d->adjustedLine.p1() - vec - orthogonal);
    result.lineTo(d->adjustedLine.p2() + vec - orthogonal);
    result.lineTo(d->adjustedLine.p2() + vec + orthogonal);
    result.closeSubpath();

    d->shape = result;
    d->boundingRect = d->shape.boundingRect();

    if (d->head) {
        d->head->setPos(d->adjustedLine.p2());
        d->head->setRotation(-d->line.angle());
    }
    if (d->tail) {
        d->tail->setPos(d->adjustedLine.p1());
        d->tail->setRotation(-d->line.angle());
    }

    updateTooltip();
    update();
}

void LakosRelation::updateTooltip()
{
    setToolTip(QString::fromStdString(legendText()));
}

void LakosRelation::setLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
    setLine(QLineF(x1, y1, x2, y2));
}

QLineF LakosRelation::line() const
{
    return d->line;
}

QLineF LakosRelation::adjustedLine() const
{
    return d->adjustedLine;
}

QPainterPath LakosRelation::shape() const
{
    return d->shape;
}

void LakosRelation::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // This feels like a hack, but we need a different flag from isVisible
    // to hide some data that we *really* do not want to display.
    if (d->shouldBeHidden) {
        return;
    }

    Q_UNUSED(option);
    Q_UNUSED(widget);

    const auto overrideC = overrideColor();
    auto const color = overrideC.isValid() ? overrideC : d->color;
    auto pen = QPen(color);

    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsHighlighted) {
        pen.setWidthF(d->thickness + 2);
    } else {
        pen.setWidthF(d->thickness);
    }
    pen.setStyle(d->penStyle);

    painter->save();
    painter->setPen(pen);
    painter->drawLine(d->adjustedLine);
    painter->restore();

    auto updateQGraphicsPathItemColor = [](QGraphicsPathItem *item, QColor const& color) {
        if (item == nullptr) {
            return;
        }

        auto headPen = item->pen();
        headPen.setColor(color);
        item->setPen(headPen);

        auto headBrush = item->brush();
        headBrush.setColor(color);
        item->setBrush(headBrush);
    };
    updateQGraphicsPathItemColor(d->head, color);
    updateQGraphicsPathItemColor(d->tail, color);

    if (s_showOriginalLine) {
        painter->save();
        painter->setPen(Qt::red);
        painter->drawLine(d->line);
        painter->restore();
    }

    if (s_showBoundingRect) {
        painter->save();
        painter->setPen(QPen(Qt::blue));
        painter->drawRect(boundingRect());
        painter->restore();
    }

    if (s_showShape) {
        painter->save();
        painter->setPen(QPen(Qt::magenta));
        painter->drawPath(shape());
        painter->restore();
    }

    if (s_showTextualInformation) {
        painter->save();
        painter->setPen(QPen(Qt::red));

        const auto x = boundingRect().topLeft().x();
        const auto y = boundingRect().topLeft().y();

        QString information = QObject::tr("Pos: (%1, %2), P1: (%3, %4), P2: (%5, %6)")
                                  .arg(pos().x())
                                  .arg(pos().y())
                                  .arg(d->line.p1().x())
                                  .arg(d->line.p1().y())
                                  .arg(d->line.p2().x())
                                  .arg(d->line.p2().y());

        painter->drawText(QPointF(x, y), information);
        painter->restore();
    }

    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsHighlighted) {
        setZValue(1000);
    } else {
        setZValue(1);
    }
}

QGraphicsPathItem *LakosRelation::defaultArrow()
{
    QPainterPath arrowHead;
    arrowHead.moveTo(0, 0);
    arrowHead.lineTo(-10, 5);
    arrowHead.lineTo(-10, -5);
    arrowHead.lineTo(0, 0);
    arrowHead.closeSubpath();
    arrowHead.setFillRule(Qt::FillRule::WindingFill);

    auto *arrow = new QGraphicsPathItem();
    arrow->setPath(arrowHead);
    arrow->setBrush(Qt::black);

    return arrow;
}

QGraphicsPathItem *LakosRelation::diamondArrow()
{
    QPainterPath arrowHead;
    arrowHead.moveTo(0, 0);
    arrowHead.lineTo(-10, 5);
    arrowHead.lineTo(-20, 0);
    arrowHead.lineTo(-10, -5);
    arrowHead.lineTo(0, 0);
    arrowHead.closeSubpath();
    arrowHead.setFillRule(Qt::FillRule::WindingFill);

    auto *arrow = new QGraphicsPathItem();
    arrow->setPath(arrowHead);
    arrow->setBrush(Qt::black);

    return arrow;
}

QGraphicsPathItem *LakosRelation::defaultTail()
{
    QPainterPath circlePath;
    circlePath.addEllipse(0, -5, 10, 10);

    auto *tail = new QGraphicsPathItem();
    tail->setPath(circlePath);
    tail->setBrush(QBrush(QColor(Qt::white)));
    return tail;
}

LakosEntity *LakosRelation::from() const
{
    return d->pointsFrom;
}

LakosEntity *LakosRelation::to() const
{
    return d->pointsTo;
}

void LakosRelation::toggleRelationFlags(EdgeCollection::RelationFlags flags, bool toggle)
{
    if (toggle) {
        if (flags | EdgeCollection::RelationFlags::RelationIsSelected) {
            d->selectedCounter += 1;
        }
        d->relationFlags |= flags;
    } else {
        if (flags == EdgeCollection::RelationFlags::RelationIsSelected) {
            d->selectedCounter -= 1;
            if (d->selectedCounter == 0) {
                d->relationFlags &= ~flags;
            }
        } else {
            d->relationFlags &= ~flags;
        }
    }

    update();
}

void LakosRelation::setThickness(qreal thickness)
{
    if (!qFuzzyCompare(thickness, d->thickness)) {
        d->thickness = thickness;
        update();
    }
}

void LakosRelation::setDashed(bool dashed)
{
    if (dashed == d->dashed) {
        return;
    }

    d->dashed = dashed;
    this->setStyle(d->dashed ? Qt::DashLine : Qt::SolidLine);
    update();
}

void LakosRelation::setShouldBeHidden(bool hidden)
{
    if (d->shouldBeHidden != hidden) {
        d->shouldBeHidden = hidden;
        if (hidden) {
            if (d->tail) {
                delete d->tail;
                d->tail = nullptr;
            }
            if (d->head) {
                delete d->head;
                d->head = nullptr;
            }
        }
        update();
    }
}

bool LakosRelation::shouldBeHidden() const
{
    return d->shouldBeHidden;
}

void LakosRelation::populateMenu(QMenu& menu, QMenu *debugMenu)
{
    auto *gs = qobject_cast<GraphicsScene *>(scene());

    menu.addAction(
        QObject::tr("%1 to %2").arg(QString::fromStdString(from()->name()), QString::fromStdString(to()->name())));

    auto *removeAction = new QAction(tr("Remove"));
    removeAction->setToolTip(tr("Removes this relation from the database"));
    connect(removeAction, &QAction::triggered, this, &LakosRelation::requestRemoval);
    menu.addAction(removeAction);

    auto *node = d->pointsFrom->internalNode();
    if (node->type() == lvtshr::DiagramType::PackageType && !node->isPackageGroup()) {
        auto *action = new QAction(tr("Inspect package dependencies"));
        action->setToolTip(tr("Shows a list of dependencies between those packages"));
        connect(action, &QAction::triggered, this, [this, gs]() {
            auto inspectDepWindow = InspectDependencyWindow{gs->projectFile(), *this};
            inspectDepWindow.show();
            inspectDepWindow.exec();
        });
        menu.addAction(action);
    }

    Q_UNUSED(debugMenu);
}

std::string LakosRelation::legendText() const
{
    std::string ret = from()->name() + " to " + to()->name() + "\n";
    ret += "Type: " + relationTypeAsString() + "\n";

    if (Preferences::enableSceneContextMenu()) {
        const std::string x1 = std::to_string(d->adjustedLine.p1().x());
        const std::string x2 = std::to_string(d->adjustedLine.p2().x());
        const std::string y1 = std::to_string(d->adjustedLine.p1().y());
        const std::string y2 = std::to_string(d->adjustedLine.p2().y());

        ret += "From: (" + x1 + "," + y1 + ") to (" + x2 + "," + y2 + ")\n";
    }

    return ret;
}

void LakosRelation::toggleIntersectionPaths()
{
    s_showIntersectionPaths = !s_showIntersectionPaths;
}

void LakosRelation::toggleBoundingRect()
{
    s_showBoundingRect = !s_showBoundingRect;
}

void LakosRelation::toggleShape()
{
    s_showShape = !s_showShape;
}

void LakosRelation::toggleTextualInformation()
{
    s_showTextualInformation = !s_showTextualInformation;
}

void LakosRelation::toggleOriginalLine()
{
    s_showOriginalLine = !s_showOriginalLine;
}

void LakosRelation::updateDebugInformation()
{
    d->fromIntersectionItem->setVisible(s_showIntersectionPaths);
    d->toIntersectionItem->setVisible(s_showIntersectionPaths);
    update();
}

bool LakosRelation::showBoundingRect()
{
    return s_showBoundingRect;
}

bool LakosRelation::showShape()
{
    return s_showShape;
}

bool LakosRelation::showTextualInformation()
{
    return s_showTextualInformation;
}

bool LakosRelation::showIntersectionPaths()
{
    return s_showIntersectionPaths;
}

bool LakosRelation::showOriginalLine()
{
    return s_showOriginalLine;
}

QColor LakosRelation::hoverColor() const
{
    return d->highlightColor;
}

QColor LakosRelation::overrideColor() const
{
    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsSelected) {
        return Qt::red;
    }
    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsHighlighted) {
        return Qt::red;
    }
    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsCyclic) {
        return Qt::blue;
    }
    if (d->relationFlags & EdgeCollection::RelationFlags::RelationIsParentHovered) {
        return hoverColor();
    }
    return {};
}

} // end namespace Codethink::lvtqtc
