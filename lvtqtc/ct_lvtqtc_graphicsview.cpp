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

    connect(d->scene, &GraphicsScene::packageNavigateRequested, this, &GraphicsView::packageNavigateRequested);
    connect(d->scene, &GraphicsScene::classNavigateRequested, this, &GraphicsView::classNavigateRequested);
    connect(d->scene, &GraphicsScene::componentNavigateRequested, this, &GraphicsView::componentNavigateRequested);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->minimap->setSceneRect(mapToScene(rect()).boundingRect());
    });

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        d->minimap->setSceneRect(mapToScene(rect()).boundingRect());
    });

    auto *graphPrefs = Preferences::self()->window()->graphWindow();
    connect(graphPrefs, &GraphWindow::backgroundColorChanged, this, [this](const QColor& c) {
        setBackgroundBrush(QBrush(c));
    });
    setBackgroundBrush(QBrush(graphPrefs->backgroundColor()));

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

lvtshr::DiagramType GraphicsView::diagramType() const
{
    return d->scene->diagramType();
}

void GraphicsView::toggleMinimap(bool toggle)
{
    d->minimap->setVisible(toggle);
}

void GraphicsView::toggleLegend(bool toggle)
{
    d->toolTipItem->setVisible(toggle);
}

void GraphicsView::setUndoManager(UndoManager *undoManager)
{
    d->undoManager = undoManager;
}

void GraphicsView::debugVisibleScreen()
{
    auto itemList = items(viewport()->rect());
    d->scene->dumpScene(itemList);
}

void GraphicsView::setColorManagement(const std::shared_ptr<lvtclr::ColorManagement>& colorManagement)
{
    assert(colorManagement);
    d->scene->setColorManagement(colorManagement);
}

bool GraphicsView::updateClassGraph(const QString& fullyQualifiedClassName)
{
    return updateGraph(fullyQualifiedClassName, lvtshr::DiagramType::ClassType);
}

bool GraphicsView::updatePackageGraph(const QString& fullyQualifiedPackageName)
{
    return updateGraph(fullyQualifiedPackageName, lvtshr::DiagramType::PackageType);
}

bool GraphicsView::updateComponentGraph(const QString& fullyQualifiedComponentName)
{
    return updateGraph(fullyQualifiedComponentName, lvtshr::DiagramType::ComponentType);
}

bool GraphicsView::updateGraph(const QString& fullyQualifiedName, lvtshr::DiagramType type)
{
    if (d->scene->isEditMode()) {
        return false;
    }

    const auto oldQualifiedName = d->fullyQualifiedName;
    d->fullyQualifiedName = fullyQualifiedName;
    d->scene->setMainNode(fullyQualifiedName, type);
    if (d->scene->mainEntity() && oldQualifiedName != d->fullyQualifiedName) {
        fitAllInView();
    }
    return true;
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

void GraphicsView::fitMainEntityInView()
{
    auto const& e = d->scene->mainEntity();

    auto w = e->rect().width();
    auto h = e->rect().height();
    auto x = e->scenePos().x() - w / 2.;
    auto y = e->scenePos().y() - h / 2.;

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
    auto zoomModifier = Preferences::self()->window()->graphWindow()->zoomModifier();
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
    auto *prefs = Preferences::self()->window()->graphWindow();
    if (event->modifiers() & prefs->panModifier()) {
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void GraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    auto *prefs = Preferences::self()->window()->graphWindow();

    Qt::Key modifier = Qt::Key_Alt;
    switch (prefs->panModifier()) {
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

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (Preferences::self()->debug()->enableDebugOutput()) {
        qDebug() << "GraphicsView mousePressEvent";
    }
    if (event->button() == Qt::ForwardButton) {
        Q_EMIT requestNext();
        return;
    }
    if (event->button() == Qt::BackButton) {
        Q_EMIT requestPrevious();
        return;
    }

    auto *prefs = Preferences::self()->window()->graphWindow();
    if (event->modifiers() & prefs->panModifier() || prefs->panModifier() == Qt::KeyboardModifier::NoModifier) {
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
    if (Preferences::self()->debug()->enableDebugOutput()) {
        qDebug() << "GraphicsView mouseReleaseEvent";
    }

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

void GraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.setToolTipsVisible(true);

    QMenu *debugMenu = nullptr;

    if (Preferences::self()->debug()->enableSceneContextMenu()) {
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
    if (event->mimeData()->hasFormat("codevis/qualifiedname")) {
        event->acceptProposedAction();
    }
}

void GraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void GraphicsView::dropEvent(QDropEvent *event)
{
    if (!d->scene->isEditMode()) {
        Q_EMIT errorMessage(
            "Entities can only be dropped in the 'Edit mode'. You are currently in 'Visualization mode'.\n"
            "You can change the mode in the tool box (side panel).");
        return;
    }

    const QString& qualName = event->mimeData()->data("codevis/qualifiedname");
    d->scene->loadEntityByQualifiedName(qualName, mapToScene(event->pos()));
}

GraphicsScene *GraphicsView::graphicsScene() const
{
    return d->scene;
}

void GraphicsView::editModeTriggered()
{
    if (d->currentTool) {
        d->currentTool->deactivate();
    }

    d->scene->editModeTriggered();
}

void GraphicsView::visualizationModeTriggered()
{
    if (d->currentTool) {
        d->currentTool->deactivate();
    }
    d->scene->visualizationModeTriggered();
}

void GraphicsView::setPluginManager(Codethink::lvtplg::PluginManager& pm)
{
    d->scene->setPluginManager(pm);
}

} // namespace Codethink::lvtqtc
