// ct_lvtqtc_graphicsview.cpp                                        -*-C++-*-

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

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_lakosrelation.h>
#include <ct_lvtqtc_minimap.h>
#include <ct_lvtqtc_tooltip.h>
#include <ct_lvtqtc_undo_manager.h>

#include <QBrush>
#include <QCursor>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QToolTip>
#include <QTransform>

#include <qnamespace.h>
#include <set>

#include <preferences.h>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct GraphicsView::Private {
    QString fullyQualifiedName;
    GraphicsScene *scene = nullptr;
    Minimap *minimap = nullptr;
    ToolTipItem *toolTipItem = nullptr;
    int zoomFactor = 100;
    bool initialized = false;
    ITool *currentTool = nullptr;
    UndoManager *undoManager = nullptr;

    struct {
        bool isActive = false;
        QPoint start;
        QPoint end;
        const QPen pen = QPen(QBrush(Qt::black), 1, Qt::PenStyle::DashLine);
        const QBrush brush = QBrush(QColor(0, 0, 0, 30));
    } multiSelect;

    bool isMultiDragging = false;

    struct {
        QString text;
        lvtshr::SearchMode mode = lvtshr::SearchMode::CaseInsensitive;
        int current = 0;
        QList<LakosEntity *> searchResult;
    } search;
};

// --------------------------------------------
// class GraphicsView
// --------------------------------------------

GraphicsView::GraphicsView(NodeStorage& nodeStorage, lvtprj::ProjectFile const& projectFile, QWidget *parent):
    QGraphicsView(parent), d(std::make_unique<GraphicsView::Private>())
{
    d->scene = new GraphicsScene(nodeStorage, projectFile, this);
    setScene(d->scene);
    setBackgroundBrush(QBrush(Qt::white));
    setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::TextAntialiasing
                   | QPainter::RenderHint::SmoothPixmapTransform);

    connect(d->scene, &GraphicsScene::graphLoadStarted, this, &GraphicsView::graphLoadStarted);
    connect(d->scene, &GraphicsScene::graphLoadFinished, this, &GraphicsView::graphLoadFinished);

    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    d->minimap = new Minimap(d->scene, this);
    d->minimap->setVisible(false);

    connect(d->minimap, &Minimap::focusSceneAt, this, [this](const QPointF& p) {
        centerOn(p);
    });

    d->toolTipItem = new ToolTipItem(this);
    d->toolTipItem->addToolTip("Is A");
    d->toolTipItem->addToolTip("Uses in the Implementation");
    d->toolTipItem->addToolTip("Uses in the Interface");
    d->toolTipItem->move(10, 110);
    d->toolTipItem->setVisible(false);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->minimap->setSceneRect(mapToScene(rect()).boundingRect());
    });

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->minimap->setSceneRect(mapToScene(rect()).boundingRect());
    });

    connect(Preferences::self(), &Preferences::backgroundColorChanged, this, [this] {
        setBackgroundBrush(QBrush(Preferences::backgroundColor()));
    });
    setBackgroundBrush(QBrush(Preferences::backgroundColor()));

    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setAcceptDrops(true);
    setTransformationAnchor(ViewportAnchor::AnchorUnderMouse);
}

GraphicsView::~GraphicsView() noexcept = default;

void GraphicsView::undoCommandReceived(QUndoCommand *command)
{
    if (!d->undoManager) {
        return;
    }
    d->undoManager->addUndoCommand(command);
    Q_EMIT onUndoCommandReceived(this, command);
}

void GraphicsView::toggleMinimap(bool toggle)
{
    d->minimap->setVisible(toggle);
    Preferences::setShowMinimap(toggle);
}

void GraphicsView::toggleLegend(bool toggle)
{
    d->toolTipItem->setVisible(toggle);
    Preferences::setShowLegend(toggle);
}

void GraphicsView::setUndoManager(UndoManager *undoManager)
{
    d->undoManager = undoManager;
}

void GraphicsView::debugVisibleScreen()
{
    qDebug() << d->scene->toJson();
}

void GraphicsView::setColorManagement(const std::shared_ptr<lvtclr::ColorManagement>& colorManagement)
{
    assert(colorManagement);
    d->scene->setColorManagement(colorManagement);
}

void GraphicsView::fitAllInView()
{
    // Note: Do not use `d->scene->itemsBoundingRect()` to fit all in view, because it takes in consideration hidden
    // items, and we do not update the position of hidden items. Thus, using the aforementioned method will compute a
    // wrong rect for the view.
    auto boundingRect = QRectF{};
    for (auto const& entity : d->scene->allEntities()) {
        if (entity->isVisible()) {
            boundingRect |= entity->mapRectToScene(entity->boundingRect());
        }
    }
    fitRectInView(boundingRect);
}

void GraphicsView::fitItemInView(QGraphicsItem *entity)
{
    auto w = entity->boundingRect().width();
    auto h = entity->boundingRect().height();
    auto x = entity->scenePos().x() - w / 2.;
    auto y = entity->scenePos().y() - h / 2.;

    auto r = QRectF{x, y, w, h};
    fitRectInView(r);
}

void GraphicsView::fitRectInView(QRectF const& r)
{
    constexpr qreal PAD = 100;
    constexpr qreal PREFERRED_ZOOM_FACTOR = 100; // percent

    QRectF boundingRect = r.adjusted(-PAD / 2, -PAD / 2, PAD / 2, PAD / 2);
    fitInView(boundingRect, Qt::AspectRatioMode::KeepAspectRatio);
    calculateCurrentZoomFactor();

    // If the zoom factor is higher than the preferred factor, use the preferred factor to avoid zooming in too much.
    if (d->zoomFactor > PREFERRED_ZOOM_FACTOR) {
        setZoomFactor(PREFERRED_ZOOM_FACTOR);
    }
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    // Handle Zoom
    auto zoomModifier = Preferences::zoomModifier();
    if (event->modifiers() & zoomModifier || zoomModifier == Qt::KeyboardModifier::NoModifier) {
        if (event->angleDelta().y() > 0) {
            setZoomFactor(d->zoomFactor + 2);
        } else {
            setZoomFactor(d->zoomFactor - 2);
        }
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void GraphicsView::setZoomFactor(int zoomFactorInPercent)
{
    if (d->zoomFactor == zoomFactorInPercent) {
        return;
    }

    // don't let the zoom factor go below 1%:
    // 0% looks like a divide by zero and displays nothing
    // negative zoom flips the diagram
    constexpr int zoomLimit = 1;
    if (zoomFactorInPercent < zoomLimit) {
        zoomFactorInPercent = zoomLimit;
    }

    resetTransform();
    double scaleFactor = zoomFactorInPercent / 100.0;
    scale(scaleFactor, scaleFactor);
    d->zoomFactor = zoomFactorInPercent;
    Q_EMIT zoomFactorChanged(zoomFactorInPercent);

    d->scene->setBlockNodeResizeOnHover(zoomFactorInPercent >= 100);
}

int GraphicsView::zoomFactor() const
{
    return d->zoomFactor;
}

void GraphicsView::calculateCurrentZoomFactor()
{
    d->zoomFactor = static_cast<int>(transform().m11() * 100);
    Q_EMIT zoomFactorChanged(d->zoomFactor);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    assert(event);

    const bool mouseHasNewPosition = d->multiSelect.end != event->pos();

    if (d->multiSelect.isActive && mouseHasNewPosition) {
        d->multiSelect.end = event->pos();
        auto selection = QRect(d->multiSelect.start, d->multiSelect.end).normalized();
        QSet<LakosEntity *> currentSelection;
        static QSet<LakosEntity *> oldSelection;

        // Fill currentSelection with all LakosEntities in selection
        const auto itemsInSelection = items(selection, Qt::IntersectsItemBoundingRect);
        for (QGraphicsItem *item : itemsInSelection) {
            if (auto *lEntity = qgraphicsitem_cast<LakosEntity *>(item)) {
                currentSelection.insert(lEntity);
            }
        }

        // Update selection
        for (const auto lEntity : currentSelection) {
            if (!lEntity->isSelected()) {
                lEntity->setSelected(true);
            }
        }

        auto toUnselect = oldSelection;
        toUnselect.subtract(currentSelection);
        for (const auto lEntity : toUnselect) {
            lEntity->setSelected(false);
        }

        if (oldSelection != currentSelection) {
            std::deque<LakosianNode *> selectedNodes;
            for (const auto& lEntity : currentSelection) {
                selectedNodes.push_back(lEntity->internalNode());
            }
            Q_EMIT newSelectionMade(selectedNodes);
            oldSelection = currentSelection;
        }

        viewport()->update();
    }

    if (d->isMultiDragging && mouseHasNewPosition) {
        for (const auto& entity : d->scene->selectedEntities()) {
            entity->doDrag(mapToScene(event->pos()));
        }
    }

    QList<QGraphicsItem *> underMouse = items(event->pos());

    d->toolTipItem->clear();
    for (QGraphicsItem *item : underMouse) {
        if (auto *relation = qgraphicsitem_cast<LakosRelation *>(item)) {
            QString label = QString::fromStdString(relation->legendText());
            d->toolTipItem->addToolTip(label);
            break;
        }
        if (auto *entity = qgraphicsitem_cast<LakosEntity *>(item)) {
            QString label = QString::fromStdString(entity->legendText());
            d->toolTipItem->addToolTip(label);
            break;
        }
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::zoomIntoRect(const QPoint& topLeft, const QPoint& bottomRight)
{
    auto mapTopLeft = mapToScene(topLeft);
    auto mapBottomRight = mapToScene(bottomRight);
    fitInView(QRectF(mapTopLeft, mapBottomRight), Qt::AspectRatioMode::KeepAspectRatio);

    d->zoomFactor = static_cast<int>(transform().m11() * 100);
    // m11 is the matrix coordinate that takes care of the
    // scale factor.

    Q_EMIT zoomFactorChanged(d->zoomFactor);
}

void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Preferences::panModifier()) {
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void GraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    Qt::Key modifier = Qt::Key_Alt;
    switch (Preferences::panModifier()) {
    case Qt::AltModifier:
        modifier = Qt::Key_Alt;
        break;
    case Qt::ControlModifier:
        modifier = Qt::Key_Control;
        break;
    case Qt::ShiftModifier:
        modifier = Qt::Key_Shift;
        break;
    default:
        modifier = Qt::Key_Alt;
    }

    if (event->key() == modifier) {
        setDragMode(QGraphicsView::DragMode::NoDrag);
        event->accept();
    }

    QGraphicsView::keyReleaseEvent(event);
}

namespace {
template<typename T>
T castUpToParent(QGraphicsItem *item)
{
    if (!item) {
        return nullptr;
    }
    if (auto entity = dynamic_cast<T>(item)) {
        return entity;
    }
    if (item->parentItem() == nullptr) {
        return nullptr;
    }
    return castUpToParent<T>(item->parentItem());
}
} // namespace

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (Preferences::enableDebugOutput()) {
        qDebug() << "GraphicsView mousePressEvent";
    }

    if (!itemAt(event->pos())) {
        if (event->button() == Qt::LeftButton) {
            d->multiSelect.start = event->pos();
            d->multiSelect.end = event->pos();
            d->multiSelect.isActive = true;
        } else {
            d->multiSelect.isActive = false;
        }
    }

    if (event->button() == Qt::LeftButton) {
        if (QGraphicsItem *item = itemAt(event->pos())) {
            if (d->scene->selectedEntities().size() > 1) {
                if (const auto *entity = castUpToParent<LakosEntity *>(item)) {
                    if (entity->isSelected()) {
                        d->isMultiDragging = true;
                    }
                }
            }
        }
    }

    if (d->isMultiDragging) {
        for (auto& entity : d->scene->selectedEntities()) {
            entity->startDrag(mapToScene(event->pos()));
        }
    }

    if (event->button() == Qt::ForwardButton) {
        Q_EMIT requestNext();
        return;
    }
    if (event->button() == Qt::BackButton) {
        Q_EMIT requestPrevious();
        return;
    }

    if (event->modifiers() & Preferences::panModifier()
        || Preferences::panModifier() == Qt::KeyboardModifier::NoModifier) {
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
    }

    // Qt loses the selection if we right click. So we need to
    // store the selection, run the virtual call, then restore
    // the selection.
    if (event->button() == Qt::RightButton) {
        const auto selectedItems = scene()->selectedItems();
        QGraphicsView::mousePressEvent(event);
        for (auto *item : selectedItems) {
            item->setSelected(true);
        }
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (Preferences::enableDebugOutput()) {
        qDebug() << "GraphicsView mouseReleaseEvent";
    }

    // update selectedEntities using entity->toggleSelection()
    for (const auto& entity : d->scene->allEntities()) {
        const bool isSelected = entity->isSelected();
        const auto selectedEntities = d->scene->selectedEntities();
        const bool isContainedInSelectedEntities =
            std::find(selectedEntities.begin(), selectedEntities.end(), entity) != selectedEntities.end();

        if ((isSelected && !isContainedInSelectedEntities) || (!isSelected && isContainedInSelectedEntities)) {
            entity->toggleSelection();
        }
    }

    d->multiSelect.isActive = false;

    if (d->isMultiDragging) {
        for (const auto& entity : d->scene->selectedEntities()) {
            entity->endDrag(mapToScene(event->pos()));
        }
        d->isMultiDragging = false;
    }

    viewport()->update();

    setDragMode(QGraphicsView::DragMode::NoDrag);

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::showEvent(QShowEvent *event)
{
    if (!d->initialized) {
        d->initialized = true;
        fitAllInView();
    }
    QGraphicsView::showEvent(event);
}

void GraphicsView::setCurrentTool(ITool *tool)
{
    d->currentTool = tool;
}

ITool *GraphicsView::currentTool() const
{
    return d->currentTool;
}

void GraphicsView::drawForeground(QPainter *painter, const QRectF& rect)
{
    if (d->currentTool) {
        d->currentTool->drawForeground(painter, rect);
    }

    if (d->multiSelect.isActive) {
        painter->save();
        painter->setWorldMatrixEnabled(false);
        painter->setPen(d->multiSelect.pen);
        painter->setBrush(d->multiSelect.brush);
        painter->drawRect(QRect(d->multiSelect.start, d->multiSelect.end));
        painter->setWorldMatrixEnabled(true);
        painter->restore();
    }

    if (d->search.text.length() == 0) {
        return;
    }

    // Semi translucent yellow
    const QColor foundColor(0x55, 0xff, 0xff, 0x99);

    // semi translucent blue
    const QColor currentColor(0x55, 0x38, 0xB0, 0xDE);

    // semi translucent black
    const QColor foreground(155, 155, 155, 220);

    QPainterPath path;
    path.addRect(rect);

    painter->save();
    // draw the whole area with a blackish translucent brush.

    int idx = 1;
    std::set<LakosEntity *> parentsNotInSearch;

    for (LakosEntity *entity : qAsConst(d->search.searchResult)) {
        auto scenePos = entity->sceneBoundingRect().center();
        if (rect.contains(scenePos)) {
            QRectF rectHole = entity->boundingRect();

            // The LakosEntities have the center on the middle, we need to fix
            // the rect.
            rectHole.moveTo(QPointF(scenePos.x() - rectHole.width() / 2, scenePos.y() - rectHole.height() / 2));
            painter->setBrush(QBrush(idx == d->search.current ? currentColor : foundColor));

            // We can't paint the parent item unless it's the currently selected
            // one, because it covers all the child items that could have been
            // selected.
            if (!entity->parentItem() || idx == d->search.current) {
                painter->drawRect(rectHole);
            }

            // Add the parents that are not found on a set so it's faster to
            // look them up for the subsequent children. We assume that there
            // are way more children than parents on the searches, since `bal`
            // will match the package, and all the children. looking the parent
            // for every child on a vector can be slow.
            auto *parentEntity = dynamic_cast<LakosEntity *>(entity->parentItem());
            auto it = parentsNotInSearch.find(parentEntity);
            if (it == std::end(parentsNotInSearch)) {
                if (!d->search.searchResult.contains(parentEntity)) {
                    it = parentsNotInSearch.insert(parentEntity).first;
                }
            }

            if (!entity->parentItem() || it != std::end(parentsNotInSearch)) {
                path.addRect(rectHole);
            }
        }

        idx += 1;
    }
    painter->restore();

    painter->setBrush(QBrush(foreground));
    painter->drawPath(path);
}

void GraphicsView::setSearchMode(lvtshr::SearchMode mode)
{
    d->search.mode = mode;
    doSearch();
}

void GraphicsView::setSearchString(const QString& search)
{
    d->search.text = search;
    invalidateScene();
    doSearch();
}

void GraphicsView::highlightedNextSearchElement()
{
    d->search.current = d->search.current % d->search.searchResult.size() + 1;

    Q_EMIT currentSearchItemHighlighted(d->search.current);

    // We use 1 based index because this is for user facing entries, we need
    // to subtract -1 here.

    LakosEntity *entity = d->search.searchResult[d->search.current - 1];
    ensureVisible(entity);

    viewport()->update();
}

void GraphicsView::highlightedPreviousSearchElement()
{
    d->search.current = d->search.current == 1 ? d->search.searchResult.size() : d->search.current - 1;

    Q_EMIT currentSearchItemHighlighted(d->search.current);

    // We use 1 based index because this is for user facing entries, we need
    // to subtract -1 here.
    LakosEntity *entity = d->search.searchResult[d->search.current - 1];
    ensureVisible(entity);

    viewport()->update();
}

void GraphicsView::doSearch()
{
    d->search.searchResult.clear();
    std::vector<LakosEntity *> entities = d->scene->allEntities();
    const QString searchText =
        d->search.mode == lvtshr::SearchMode::CaseInsensitive ? d->search.text.toLower() : d->search.text;

    if (searchText.size() == 0) {
        Q_EMIT searchTotal(0);
        Q_EMIT currentSearchItemHighlighted(0);
        viewport()->update();
        return;
    }

    for (LakosEntity *entity : entities) {
        if (!entity->isVisible()) {
            continue;
        }

        // QString allow us to use some nice things that std::string lacks.
        const QString nameEntity = d->search.mode == lvtshr::SearchMode::CaseInsensitive
            ? QString::fromStdString(entity->name()).toLower()
            : QString::fromStdString(entity->name());

        if (nameEntity.contains(searchText)) {
            d->search.searchResult.append(entity);
        };
    }

    if (d->search.searchResult.empty()) {
        Q_EMIT searchTotal(0);
        Q_EMIT currentSearchItemHighlighted(0);
        viewport()->update();
        return;
    }

    d->search.current = 1;
    Q_EMIT searchTotal(d->search.searchResult.size());
    Q_EMIT currentSearchItemHighlighted(d->search.current);
    viewport()->update();
}

void GraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.setToolTipsVisible(true);

    QMenu *debugMenu = nullptr;

    if (Preferences::enableSceneContextMenu()) {
        debugMenu = new QMenu(tr("Debug tools"));
        debugMenu->setToolTipsVisible(true);
    }

    QGraphicsItem *item = itemAt(event->pos());
    if (!item) {
        d->scene->populateMenu(menu, debugMenu);
    } else {
        if (auto *relation = dynamic_cast<LakosRelation *>(item)) {
            relation->populateMenu(menu, debugMenu);
        } else if (auto *entity = castUpToParent<LakosEntity *>(item)) {
            entity->populateMenu(menu, debugMenu, mapToScene(event->pos()));
        }
    }

    if (debugMenu && !debugMenu->isEmpty()) {
        menu.addMenu(debugMenu);
    }

    if (menu.isEmpty()) {
        return;
    }
    menu.exec(event->globalPos());
}

void GraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("codevis/qualifiednames")) {
        event->acceptProposedAction();
    }
}

void GraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void GraphicsView::dropEvent(QDropEvent *event)
{
    const QString qualNames = event->mimeData()->data("codevis/qualifiednames");
#ifdef KDE_FRAMEWORKS_IS_OLD
    QStringList qualNameList = qualNames.split(";");
    qualNameList.removeAll(QString(";"));
#else
    const QStringList qualNameList = qualNames.split(";", Qt::SplitBehaviorFlags::SkipEmptyParts);
#endif

    for (const auto& qualName : qualNameList) {
        d->scene->loadEntityByQualifiedName(qualName, mapToScene(event->pos()));
    }

    if (qualNameList.size() > 1) {
        d->scene->reLayout();
    }
}

GraphicsScene *GraphicsView::graphicsScene() const
{
    return d->scene;
}

void GraphicsView::setPluginManager(Codethink::lvtplg::PluginManager& pm)
{
    d->scene->setPluginManager(pm);
}

} // namespace Codethink::lvtqtc
