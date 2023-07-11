// ct_lvtqtw_minimap.h                                 -*-C++-*-

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

#include <ct_lvtqtc_minimap.h>

#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QScreen>

#include <preferences.h>

namespace Codethink::lvtqtc {

struct Minimap::Private {
    bool isDraggingScene = false;
    bool isDraggingSelf = false;
    QPoint movementDelta;
    QRectF sceneRect;

    int minimapSize = Minimap::MINIMAP_SMALL;
};

Minimap::Minimap(QGraphicsScene *mainScene, QWidget *parent):
    QGraphicsView(parent), d(std::make_unique<Minimap::Private>())
{
    setScene(mainScene);
    setBackgroundBrush(QBrush(Qt::white));
    setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::TextAntialiasing
                   | QPainter::RenderHint::SmoothPixmapTransform);

    setCacheMode(QGraphicsView::CacheModeFlag::CacheNone);

    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    d->minimapSize = Preferences::self()->window()->graphWindow()->minimapSize();

    setGeometry(QRect(0, 0, 100, 80));

    connect(scene(), &QGraphicsScene::sceneRectChanged, this, [this] {
        calculateGeometry();
    });
}

Minimap::~Minimap() = default;

void Minimap::setSceneRect(const QRectF& rect)
{
    if (d->sceneRect == rect) {
        return;
    }
    d->sceneRect = rect;
    scene()->update();
}

void Minimap::calculateGeometry()
{
    QScreen *thisScreen = QGuiApplication::primaryScreen();
    if (!thisScreen) {
        return;
    }

    const qreal sceneWidth = scene()->sceneRect().width();
    const qreal sceneHeight = scene()->sceneRect().height();

    if (sceneWidth == 0 || sceneHeight == 0) {
        setGeometry(QRect(0, 0, 100, 80));
        return;
    }

    int thisWidth = 0;
    int thisHeight = 0;

    if (sceneWidth > sceneHeight) {
        thisWidth = thisScreen->geometry().width() / (int) d->minimapSize;
        assert(sceneHeight < INT_MAX && scene()->sceneRect().width() < INT_MAX);
        thisHeight = (thisWidth * (int) sceneHeight) / (int) sceneWidth;
    } else {
        thisHeight = thisScreen->geometry().height() / (int) d->minimapSize;
        assert(sceneHeight < INT_MAX && scene()->sceneRect().width() < INT_MAX);
        thisWidth = (thisHeight * (int) sceneWidth) / (int) sceneHeight;
    }

    QRect geom = geometry();
    geom.setWidth(thisWidth);
    geom.setHeight(thisHeight);
    if (geom.width() < 100 || geom.height() < 100) {
        geom.setWidth(geom.width() * 2);
        geom.setHeight(geom.height() * 2);
    }

    setGeometry(geom);
    fitInView(scene()->sceneRect());
}

void Minimap::setMapSize(MinimapSize size)
{
    if (d->minimapSize == size) {
        return;
    }

    d->minimapSize = size;
    Preferences::self()->window()->graphWindow()->setMinimapSize(size);

    calculateGeometry();
}

void Minimap::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    menu.addAction(tr("Select Minimap Size"));
    menu.addSeparator();
    QAction *ac = menu.addAction(tr("Small"));
    connect(ac, &QAction::triggered, this, [this] {
        setMapSize(MinimapSize::MINIMAP_SMALL);
    });

    ac = menu.addAction(tr("Medium"));
    connect(ac, &QAction::triggered, this, [this] {
        setMapSize(MinimapSize::MINIMAP_MEDIUM);
    });

    ac = menu.addAction(tr("Large"));
    connect(ac, &QAction::triggered, this, [this] {
        setMapSize(MinimapSize::MINIMAP_LARGE);
    });

    menu.exec(ev->globalPos());
}

void Minimap::drawForeground(QPainter *painter, const QRectF& rect)
{
    if (d->sceneRect.width() > rect.width()) {
        return;
    }
    if (d->sceneRect.height() > rect.height()) {
        return;
    }

    painter->save();
    painter->setPen(QPen(Qt::NoPen));
    painter->setBrush(QBrush(QColor(100, 100, 100, 100)));
    painter->drawRect(d->sceneRect);
    painter->restore();
}

void Minimap::mousePressEvent(QMouseEvent *ev)
{
    if (ev->modifiers() & Preferences::self()->window()->graphWindow()->dragModifier()) {
        d->isDraggingScene = false;
        d->isDraggingSelf = true;

        QLine line(mapToParent(ev->pos()), pos());
        d->movementDelta.setX(line.dx());
        d->movementDelta.setY(line.dy());
    } else {
        d->isDraggingScene = true;
        d->isDraggingSelf = false;
        Q_EMIT focusSceneAt(mapToScene(ev->localPos().toPoint()));
    }
}

void Minimap::mouseMoveEvent(QMouseEvent *ev)
{
    if (d->isDraggingScene) {
        Q_EMIT focusSceneAt(mapToScene(ev->localPos().toPoint()));
    } else if (d->isDraggingSelf) {
        QRect pos = geometry();
        pos.moveTo(mapToParent(ev->pos()) + d->movementDelta);
        setGeometry(pos);
    }
}

void Minimap::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    d->isDraggingScene = false;
    d->isDraggingSelf = false;
    QGraphicsView::mouseReleaseEvent(ev);
}

// TODO: Show a rect on the current visible part of the graph.
} // namespace Codethink::lvtqtc
