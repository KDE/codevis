// ct_lvtqtc_lakosentity.cpp                                        -*-C++-*-

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

#include <ct_lvtqtc_lakosentity.h>

#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_ellipsistextitem.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentitypluginutils.h>
#include <ct_lvtqtc_lakosrelation.h>

#include <ct_lvtqtc_alg_level_layout.h>
#include <ct_lvtqtc_undo_cover.h>
#include <ct_lvtqtc_undo_expand.h>
#include <ct_lvtqtc_undo_move.h>
#include <ct_lvtqtc_undo_notes.h>

#include <ct_lvtldr_lakosiannode.h>

#include <mrichtextedit.h>

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QJsonArray>
#include <QMenu>
#include <QPen>
#include <QPointF>
#include <QPropertyAnimation>
#include <QTextBrowser>
#include <QVector2D>

#include <cmath>
#include <fstream>
#include <optional>
#include <preferences.h>

// too many false positives for loops over QList<T>: qAsConst<QList<T>> has
// been explicitly deleted clazy:excludeall=range-loop,range-loop-detach

namespace {
static QPointF s_lastClick; // NOLINT
// used to track the last click on the screen.

constexpr int TITLE_LAYER = 1;
constexpr int COVER_LAYER = 100;
constexpr int ONE_CHILD_BORDER = 20;
constexpr int DEFAULT_BORDER = 10;
constexpr int SHRINK_BORDER = 20;

} // namespace

namespace Codethink::lvtqtc {

struct LakosEntity::Private {
    std::string name;
    // The name of the source file or C++ class

    std::string qualifiedName;
    // The fully qualified name of the source file or C++ class

    std::string toolTip;
    // the tooltip that appears when hovered.
    // if tooltip is empty, then we use the QualifiedName.

    std::string id;
    // the unique id of this node

    std::string colorId;
    // The string used to generate a color for this entity. This should be
    // set by subclasses in their constructor. The actual color is set when
    // setColorManagement() is called

    std::vector<std::shared_ptr<EdgeCollection>> edges;
    // the collections of edges that have this node as source

    std::vector<std::shared_ptr<EdgeCollection>> targetEdges;
    // the collection of edges that have this node as target

    std::vector<std::shared_ptr<EdgeCollection>> redundantRelations;
    // Edges found to be rendundant by transitive reduction

    std::vector<QGraphicsItem *> ignoredItems;
    // childrens that the layout mechanis ignores.
    // TODO: Perhaps we could eliminate this, since we also store
    // the list of LakosEntity* that are the children.

    long long shortId = 0;
    // Non-unique. Used for UniqueId.

    QList<LakosEntity *> lakosChildren;
    // All the elements that belongs to this LakosEntity

    QVector2D movementDelta;
    // When in drag movement, this is the relative amount of
    // movement we did at each mouseMove event.
    // TODO: pack this together with isMoving?

    QColor color = QColor(204, 229, 255);
    // The background color of the entity

    QColor selectedNodeColor = QColor(204, 229, 255);
    // The background color of the entity when it's selected

    Qt::BrushStyle fillPattern = Qt::BrushStyle::SolidPattern;
    // The brush pattern for this node.

    lvtshr::LoaderInfo loaderInfo;
    // the instance of the class that holds the information on how
    // this entity should be loaded.

    EllipsisTextItem *text = nullptr;
    // Pointer owned by Qt (it should be a child object of this Container)

    GraphicsRectItem *cover = nullptr;
    // Cover item for this item, hiding content.

    QGraphicsPixmapItem *notesIcon = nullptr;
    // notes, with a hover activation.

    QGraphicsSimpleTextItem *levelNr = nullptr;
    // The level number on this entity.

    bool forceHideLevelNr = false;

    QGraphicsEllipseItem *centerDbg = nullptr;
    QGraphicsRectItem *boundingRectDbg = nullptr;

    QGraphicsSimpleTextItem *notAllChildrenLoadedInfo = nullptr;
    // show an icon on the top right when this element misses child
    // elements (if we loaded the item partially).
    // TODO: 453

    QPropertyAnimation *expandAnimation = nullptr;
    // handles animated expanding / shrink.

    bool isExpanded = true;
    // Is this item exanded or collapsed?

    bool isOpaque = false;
    // We have an item covering all our internal items.

    bool initialConstruction = true;
    // Are we just constructing this item or it's already constructed?

    bool layoutUpdatesEnabled = false;
    // is this item layout automatically being triggered?

    bool showRedundantEdges = false;
    // Are we showing redundant edges on this entity?

    bool highlighted = false;
    // is this highlighted ?

    bool isMainNode = false;
    // is this the main node on the visualization?

    bool showBackground = true;
    // are we showing the bg?

    lvtldr::LakosianNode *node = nullptr;
    // the in-memory node that represents this visual node

    QFont font = QFont();
    // the current font being used by this node

    bool enableGradientOnMainNode = true;

    int presentationFlags = PresentationFlags::NoFlag;

    std::optional<std::reference_wrapper<Codethink::lvtplg::PluginManager>> pluginManager = std::nullopt;
};

LakosEntity::LakosEntity(const std::string& uniqueId, lvtldr::LakosianNode *node, lvtshr::LoaderInfo loaderInfo):
    d(std::make_unique<LakosEntity::Private>())
{
    d->node = node;
    d->name = node->name();
    d->id = uniqueId;
    d->qualifiedName = node->qualifiedName();
    d->shortId = node->id();
    d->loaderInfo = loaderInfo;

    setAcceptHoverEvents(true);
    setBrush(QBrush(Qt::white));
    setFlag(ItemIsSelectable);

    QPen currentPen = pen();
    currentPen.setCosmetic(true);
    currentPen.setWidthF(0.5);
    setPen(currentPen);

    // Default to right-angle corners
    setRoundRadius(0);

    d->notesIcon = new QGraphicsPixmapItem(this);
    setNotes(node->notes());

    d->text = new EllipsisTextItem(QString::fromStdString(node->name()), this);
    d->text->setZValue(TITLE_LAYER);

    d->expandAnimation = new QPropertyAnimation(this, "rect");
    d->expandAnimation->setDuration(250);
    d->expandAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    ignoreItemOnLayout(d->text);
    ignoreItemOnLayout(d->notesIcon);

    d->cover = new GraphicsRectItem(this);
    d->cover->setBrush(QBrush(QColor(255, 255, 255, 195)));
    d->cover->setPen(QPen(Qt::NoPen));
    d->cover->setZValue(COVER_LAYER);
    d->cover->setVisible(false);
    d->cover->setRoundRadius(roundRadius());
    ignoreItemOnLayout(d->cover);

    d->levelNr = new QGraphicsSimpleTextItem(this);
    ignoreItemOnLayout(d->levelNr);

    connect(this, &GraphicsRectItem::rectangleChanged, this, [this] {
        if (d->isOpaque) {
            hideContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
        }

        updateBackground();
        layoutAllRelations();
    });

    connect(this, &LakosEntity::moving, this, [this] {
        recursiveEdgeRelayout();
    });

    d->isExpanded = true;
    toggleExpansion(QtcUtil::CreateUndoAction::e_No);

    QObject::connect(d->node, &lvtldr::LakosianNode::onChildCountChanged, this, [this](size_t) {
        updateChildrenLoadedInfo();
        layoutIgnoredItems();
    });

    QObject::connect(d->node, &lvtldr::LakosianNode::onNotesChanged, this, [this](const std::string& notes) {
        setNotes(notes);
    });

    updateChildrenLoadedInfo();
    layoutIgnoredItems();

    d->color = Preferences::entityBackgroundColor();
    d->enableGradientOnMainNode = Preferences::enableGradientOnMainNode();
    d->selectedNodeColor = Preferences::selectedEntityBackgroundColor();
    connect(Preferences::self(), &Preferences::lakosEntityNamePosChanged, this, [this] {
        layoutIgnoredItems();
    });
    connect(Preferences::self(), &Preferences::entityBackgroundColorChanged, this, [this] {
        d->color = Preferences::entityBackgroundColor();
        updateBackground();
    });
    connect(Preferences::self(), &Preferences::enableGradientOnMainNodeChanged, this, [this] {
        d->enableGradientOnMainNode = Preferences::enableGradientOnMainNode();
        updateBackground();
    });
    connect(Preferences::self(), &Preferences::selectedEntityBackgroundColorChanged, this, [this] {
        d->selectedNodeColor = Preferences::selectedEntityBackgroundColor();
        updateBackground();
    });

    connect(Preferences::self(), &Preferences::showLevelNumbersChanged, this, [this] {
        d->levelNr->setVisible(Preferences::showLevelNumbers() && !d->forceHideLevelNr);
    });

    // static, we don't need to open the file more than once.
    QFontMetrics fm(qApp->font());
    static auto pixmap = QPixmap::fromImage(QImage(":/icons/notes").scaled(fm.height(), fm.height()));

    d->notesIcon->setPixmap(pixmap);
    d->levelNr->setVisible(Preferences::showLevelNumbers() && !d->forceHideLevelNr);
}

LakosEntity::~LakosEntity()
{
    setParentItem(nullptr);
}

void LakosEntity::setNotes(const std::string& notes)
{
    if (notes.empty()) {
        d->notesIcon->hide();
    } else {
        d->notesIcon->show();
    }
    recalculateRectangle();
}

void LakosEntity::setFont(const QFont& f)
{
    d->text->setFont(f);
    d->levelNr->setFont(f);

    const bool updatesEnabled = d->layoutUpdatesEnabled;

    d->layoutUpdatesEnabled = true;
    recalculateRectangle();
    update();

    d->layoutUpdatesEnabled = updatesEnabled;
}

lvtldr::LakosianNode *LakosEntity::internalNode() const
{
    return d->node;
}

void LakosEntity::toggleExpansion(QtcUtil::CreateUndoAction createUndoAction,
                                  std::optional<QPointF> moveToPosition,
                                  RelayoutBehavior behavior)
{
    bool shouldExpand = !isExpanded();
    if (lakosEntities().empty()) {
        shouldExpand = false;
    }

    if (shouldExpand) {
        expand(createUndoAction, moveToPosition, behavior);
    } else {
        shrink(createUndoAction, moveToPosition, behavior);
    }

    Q_SIGNAL void formFactorChanged();
}

void LakosEntity::layoutIgnoredItems()
{
    const auto titleCorner = Preferences::lakosEntityNamePos();
    const auto textRect = d->text->boundingRect();

    const auto x = [&]() -> qreal {
        if (d->isExpanded && !d->lakosChildren.empty()) {
            if (titleCorner == Qt::TopLeftCorner || titleCorner == Qt::BottomLeftCorner) {
                return boundingRect().left();
            }
            return boundingRect().right() - textRect.width();
        }
        return boundingRect().left() + boundingRect().width() / 2 - textRect.width() / 2;
    }();

    const auto y = [&]() -> qreal {
        if (d->isExpanded && !d->lakosChildren.empty()) {
            if (titleCorner == Qt::TopLeftCorner || titleCorner == Qt::TopRightCorner) {
                return boundingRect().top() - textRect.height();
            }
            return boundingRect().bottom();
        }
        return boundingRect().center().y() - d->text->boundingRect().height() / 2;
    }();

    if (d->isOpaque) {
        d->cover->setVisible(true);
    }

    d->notesIcon->setPos(boundingRect().left(), boundingRect().top());
    d->text->setPos(x, y);
    d->text->setVisible(true);

    const auto topRight = boundingRect().topRight();
    d->levelNr->setPos(topRight.x() - d->levelNr->boundingRect().width() - 10, topRight.y());

    if (d->notAllChildrenLoadedInfo) {
        constexpr auto x_spacing = 10;
        constexpr auto y_spacing = 5;
        const auto bottomRight = boundingRect().bottomRight();
        const auto children_info_x = bottomRight.x() - d->notAllChildrenLoadedInfo->boundingRect().width() - x_spacing;
        const auto children_info_y = bottomRight.y() - d->notAllChildrenLoadedInfo->boundingRect().height() - y_spacing;
        d->notAllChildrenLoadedInfo->setPos(QPointF(children_info_x, children_info_y));
        d->notAllChildrenLoadedInfo->setVisible(true);
    }
}

void LakosEntity::expand(QtcUtil::CreateUndoAction createUndoAction,
                         std::optional<QPointF> moveToPosition,
                         RelayoutBehavior behavior)
{
    if (createUndoAction == QtcUtil::CreateUndoAction::e_Yes) {
        Q_EMIT undoCommandCreated(
            new UndoExpand(qobject_cast<GraphicsScene *>(scene()), d->id, moveToPosition, behavior));
        return;
    }

    // should be set even if layout updates are disabled
    // because when the layout update is re-enabled, it will
    // run the layout logic and needs to know the form factor.

    d->isExpanded = true;
    if (!layoutUpdatesEnabled()) {
        return;
    }
    if (lakosEntities().empty()) {
        return;
    }

    setFlag(QGraphicsItem::ItemIgnoresTransformations, false);

    auto font = d->text->font();
    font.setBold(false);
    d->text->setFont(font);

    const int children = d->lakosChildren.size();
    const int border = children == 1 ? ONE_CHILD_BORDER : DEFAULT_BORDER;
    const QRectF currentRect = rect();
    if (moveToPosition) {
        setPos(*moveToPosition);
    }
    const QRectF nextRect = childrenBoundingRect().adjusted(-border, -border, border, border);

    if (d->initialConstruction) {
        setRectangle(nextRect);
        d->initialConstruction = false;
        recalculateRectangle();
        Q_EMIT formFactorChanged();
        return;
    }

    d->expandAnimation->setStartValue(currentRect);
    d->expandAnimation->setEndValue(nextRect);

    disconnect(d->expandAnimation, nullptr, this, nullptr);

    connect(d->expandAnimation, &QPropertyAnimation::finished, this, [this, behavior] {
        const QList<QGraphicsItem *> childList = childItems();
        for (auto *child : childList) {
            if (child != d->cover && child != d->notesIcon) {
                child->setVisible(true);
            }
        }

        calculateEdgeVisibility();

        for (auto *child : lakosEntities()) {
            layoutEdges(child);
        }

        layoutEdges(this);
        if (d->isOpaque) {
            hideContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
        }

        recalculateRectangle();
        Q_EMIT formFactorChanged();

        if (behavior == RelayoutBehavior::e_RequestRelayout) {
            Q_EMIT requestGraphRelayout();
        }
    });

    d->expandAnimation->start();
}

void LakosEntity::shrink(QtcUtil::CreateUndoAction createUndoAction,
                         std::optional<QPointF> moveToPosition,
                         RelayoutBehavior behavior)
{
    if (createUndoAction == QtcUtil::CreateUndoAction::e_Yes) {
        Q_EMIT undoCommandCreated(
            new UndoExpand(qobject_cast<GraphicsScene *>(scene()), d->id, moveToPosition, behavior));
        return;
    }

    // should be set even if layout updates are disabled
    // because when the layout update is re-enabled, it will
    // run the layout logic and needs to know the form factor.
    d->isExpanded = false;
    if (!layoutUpdatesEnabled()) {
        qDebug() << "Layout updates disabled!";
        return;
    }

    auto font = d->text->font();
    font.setBold(true);
    d->text->setFont(font);

    const QRectF currentRect = rect();
    if (moveToPosition) {
        setPos(*moveToPosition);
    }
    const QRectF nextRect =
        d->text->boundingRect().adjusted(-SHRINK_BORDER, -SHRINK_BORDER, SHRINK_BORDER, SHRINK_BORDER);

    for (LakosEntity *child : lakosEntities()) {
        child->setVisible(false);
        child->setRelationshipVisibility(QtcUtil::VisibilityMode::e_Hidden);
    }

    setRelationshipVisibility(QtcUtil::VisibilityMode::e_Hidden);

    if (d->initialConstruction) {
        // skip animation, going straight to how things should be
        setRectangle(nextRect);
        d->initialConstruction = false;
        layoutIgnoredItems();
        Q_EMIT formFactorChanged();
        setRelationshipVisibility(QtcUtil::VisibilityMode::e_Visible);
        return;
    }

    // This looks weird, as I'm setting the rect multiple times, but it's not:
    // I need to shrink the size so that the edges can point to the right place
    // before the animation starts (if not, the edges would point outside of the
    // package during the collapse animation), then I need to set it back to the
    // previous size, so I can animate.
    setRectangle(currentRect);
    if (currentRect == nextRect) {
        return;
    }

    if (currentRect.width() == nextRect.width() || currentRect.height() == nextRect.height()) {
        return;
    }

    d->expandAnimation->setStartValue(currentRect);
    d->expandAnimation->setEndValue(nextRect);

    disconnect(d->expandAnimation, nullptr, this, nullptr);

    connect(d->expandAnimation, &QPropertyAnimation::finished, this, [this, behavior] {
        d->cover->setVisible(false);
        layoutIgnoredItems();
        calculateEdgeVisibility();
        layoutEdges(this);
        setRelationshipVisibility(QtcUtil::VisibilityMode::e_Visible);
        Q_EMIT formFactorChanged();
        if (behavior == RelayoutBehavior::e_RequestRelayout) {
            Q_EMIT requestGraphRelayout();
        }
    });

    d->expandAnimation->start();
}

void LakosEntity::setRelationshipVisibility(QtcUtil::VisibilityMode mode)
{
    for (const auto& vec : {d->edges, d->targetEdges}) {
        for (const auto& collection : vec) {
            if (mode == QtcUtil::VisibilityMode::e_Visible && collection->isRedundant() && !d->showRedundantEdges) {
                continue;
            }

            for (auto *relationship : collection->relations()) {
                relationship->setVisible(mode == QtcUtil::VisibilityMode::e_Visible);
            }
        }
    }
}

void LakosEntity::layoutEdges(LakosEntity *child)
{
    if (!child) {
        return;
    }
    // top level call to layout edges for the container
    const EdgeCollection::PointFrom pointFrom =
        d->isExpanded ? EdgeCollection::PointFrom::SOURCE : EdgeCollection::PointFrom::PARENT;
    const EdgeCollection::PointTo pointTo =
        d->isExpanded ? EdgeCollection::PointTo::TARGET : EdgeCollection::PointTo::PARENT;

    layoutEdges(child, pointFrom, pointTo);
}

void LakosEntity::layoutEdges(LakosEntity *child,
                              const EdgeCollection::PointFrom pointFrom,
                              const EdgeCollection::PointTo pointTo)
{
    assert(child);

    // recursive call to layout edges in the child and then in grandchildren etc
    for (const auto& edgeCollection : child->edgesCollection()) {
        edgeCollection->setPointFrom(pointFrom);
    }
    for (const auto& edgeCollection : child->targetCollection()) {
        edgeCollection->setPointTo(pointTo);
    }

    for (LakosEntity *grandchild : child->lakosEntities()) {
        layoutEdges(grandchild, pointFrom, pointTo);
    }
}

LakosEntity *LakosEntity::getTopLevelParent()
{
    LakosEntity *oldParent = nullptr;
    LakosEntity *parent = this;
    while (parent) {
        oldParent = parent;
        parent = qgraphicsitem_cast<LakosEntity *>(parent->parentItem());
    }
    return oldParent;
}

void LakosEntity::setHighlighted(bool highlighted)
{
    if (d->highlighted == highlighted) {
        return;
    }

    d->highlighted = highlighted;

    if (layoutUpdatesEnabled()) {
        update();
    }
}

void LakosEntity::setName(const std::string& name)
{
    d->name = name;
    setText(name);

    if (layoutUpdatesEnabled()) {
        update();
    }
}

void LakosEntity::setPresentationFlags(PresentationFlags flags, bool value)
{
    if (value) {
        d->presentationFlags |= flags;
    } else {
        d->presentationFlags &= ~flags;
    }

    if (d->presentationFlags & PresentationFlags::Highlighted) {
        setZValue(1000);
    } else {
        setZValue(1);
    }
    update();
}

void LakosEntity::setQualifiedName(const std::string& qname)
{
    d->qualifiedName = qname;
}

void LakosEntity::setText(const std::string& text)
{
    d->text->setText(QString::fromStdString(text));
    auto tmp = d->layoutUpdatesEnabled;
    d->layoutUpdatesEnabled = true;
    this->recalculateRectangle();
    d->layoutUpdatesEnabled = tmp;
}

void LakosEntity::setTooltipString(const std::string& tt)
{
    d->toolTip = tt;
    setToolTip(QString::fromStdString(d->toolTip));
}

QList<QAction *> LakosEntity::actionsForMenu(QPointF scenePosition)
{
    if (parentItem()) {
        scenePosition = parentItem()->mapFromScene(scenePosition);
    }

    QList<QAction *> retValues;

    if (instanceType() != lvtshr::DiagramType::ClassType) {
        auto *selectAction = new QAction(isSelected() ? tr("Deselect") : tr("Select"));
        connect(selectAction, &QAction::triggered, this, &LakosEntity::toggleSelection);
        retValues.append(selectAction);
    }

    if (instanceType() != lvtshr::DiagramType::RepositoryType) {
        if (d->loaderInfo.isValid()) {
            auto *loadClientsAction = new QAction();
            loadClientsAction->setText(tr("Load all clients"));
            connect(loadClientsAction, &QAction::triggered, this, [this] {
                Q_EMIT loadClients(/*onlyLocal=*/false);
            });

            auto *loadLocalClientsAction = new QAction();
            loadLocalClientsAction->setText(tr("Load local clients"));
            connect(loadLocalClientsAction, &QAction::triggered, this, [this] {
                Q_EMIT loadClients(/*onlyLocal=*/true);
            });

            auto *loadProvidersAction = new QAction();
            loadProvidersAction->setText(tr("Load all providers"));
            connect(loadProvidersAction, &QAction::triggered, this, [this] {
                Q_EMIT loadProviders(/*onlyLocal=*/false);
            });

            auto *loadLocalProvidersAction = new QAction();
            loadLocalProvidersAction->setText(tr("Load local providers"));
            connect(loadLocalProvidersAction, &QAction::triggered, this, [this] {
                Q_EMIT loadProviders(/*onlyLocal=*/true);
            });

            retValues.append(loadProvidersAction);
            retValues.append(loadLocalProvidersAction);
            retValues.append(loadClientsAction);
            retValues.append(loadLocalClientsAction);
        }
    }

    if (instanceType() != lvtshr::DiagramType::RepositoryType) {
        auto *unloadAction = new QAction();
        unloadAction->setText(tr("Unload %1").arg(QString::fromStdString(name())));
        connect(unloadAction, &QAction::triggered, this, &LakosEntity::unloadThis);
        retValues.append(unloadAction);
    }

    auto *thisScene = qobject_cast<GraphicsScene *>(scene());
    if (thisScene) {
        auto selectedEntities = thisScene->selectedEntities();
        if (!selectedEntities.empty()) {
            auto *unloadAllAction = new QAction();
            unloadAllAction->setText(tr("Unload all selected entities"));
            for (auto& e : selectedEntities) {
                connect(unloadAllAction, &QAction::triggered, e, &LakosEntity::unloadThis);
            }
            retValues.append(unloadAllAction);
        }
    }

    if (loaderInfo().isValid()) {
        if (d->notAllChildrenLoadedInfo) {
            auto *childAction = new QAction();

            const auto type = internalNode()->type();
            const auto text = [type]() -> QString {
                switch (type) {
                case lvtshr::DiagramType::RepositoryType:
                    return tr("Load packages");
                case lvtshr::DiagramType::PackageType:
                    return tr("Load packages or components");
                case lvtshr::DiagramType::ComponentType:
                    return tr("Load UDT's");
                case lvtshr::DiagramType::ClassType:
                    return tr("Load Inner Classes and Structs");
                case lvtshr::DiagramType::FreeFunctionType:
                    // Currently, we do not expect types or functions inside free functions
                    return {};
                case lvtshr::DiagramType::NoneType:
                    assert(false && "Should never hit.");
                }
                return {};
            }();

            childAction->setText(text);
            connect(childAction, &QAction::triggered, this, &LakosEntity::loadChildren);
            retValues.append(childAction);
        } else {
            if (!internalNode()->children().empty()) {
                auto *noChildAction = new QAction();
                const auto type = internalNode()->type();
                const auto text = [type]() -> QString {
                    switch (type) {
                    case lvtshr::DiagramType::RepositoryType:
                        return tr("Unload packages");
                    case lvtshr::DiagramType::PackageType:
                        return tr("Unload packages or components");
                    case lvtshr::DiagramType::ComponentType:
                        return tr("Unload UDT's");
                    case lvtshr::DiagramType::ClassType:
                        return tr("Unload Inner Classes and Structs");
                    case lvtshr::DiagramType::FreeFunctionType:
                        // Currently, we do not expect types or functions inside free functions
                        return {};
                    case lvtshr::DiagramType::NoneType:
                        assert(false && "Should never hit.");
                    }
                    return {};
                }();

                noChildAction->setText(text);
                connect(noChildAction, &QAction::triggered, this, [this] {
                    Q_EMIT unloadChildren();
                });
                retValues.append(noChildAction);
            }
        }
    }

    auto *separator = new QAction();
    separator->setSeparator(true);
    retValues.append(separator);

    {
        auto *action = new QAction();
        action->setText(tr("Rename entity"));
        connect(action, &QAction::triggered, this, [this] {
            bool ok = false;
            auto newName = QInputDialog::getText(nullptr,
                                                 tr("Rename entity"),
                                                 tr("New name:"),
                                                 QLineEdit::Normal,
                                                 QString::fromStdString(name()),
                                                 &ok)
                               .toStdString();
            if (ok && !newName.empty()) {
                Q_EMIT entityRenameRequest(uniqueId(), newName);
            }
        });
        retValues.append(action);
    }
    if (instanceType() != lvtshr::DiagramType::RepositoryType) {
        auto *action = new QAction();
        const QString toolTip = d->node->notes().empty() ? tr("Add Notes") : tr("Change Notes");

        action->setText(toolTip);
        connect(action, &QAction::triggered, this, [this] {
            showNotesDialog();
        });
        retValues.append(action);
    }

    if (isExpanded()) {
        auto *action = new QAction(tr("Layout"));

        auto *layoutMenu = new QMenu();
        action->setMenu(layoutMenu);
        auto *verticalLayout = new QAction(tr("Vertical"));

        connect(verticalLayout, &QAction::triggered, this, [this, scenePosition] {
            auto direction = Preferences::invertVerticalLevelizationLayout() ? +1 : -1;
            levelizationLayout(LevelizationLayoutType::Vertical, direction, scenePosition);
            Q_EMIT graphUpdate();
        });
        layoutMenu->addAction(verticalLayout);

        auto *horizontalLayout = new QAction(tr("Horizontal"));
        connect(horizontalLayout, &QAction::triggered, this, [this, scenePosition] {
            auto direction = Preferences::invertHorizontalLevelizationLayout() ? +1 : -1;
            levelizationLayout(LevelizationLayoutType::Horizontal, direction, scenePosition);
            Q_EMIT graphUpdate();
        });
        layoutMenu->addAction(horizontalLayout);

        retValues.append(action);
    }

    if (!isMainEntity()) {
        auto *action = new QAction();
        action->setText(tr("Navigate to %1").arg(QString::fromStdString(d->qualifiedName)));
        connect(action, &QAction::triggered, this, [this] {
            Q_EMIT navigateRequested();
        });
        retValues.append(action);
    }

    auto *removeAction = new QAction(tr("Remove"));
    removeAction->setToolTip(tr("Removes this entity from the database"));
    connect(removeAction, &QAction::triggered, this, &LakosEntity::requestRemoval);
    retValues.append(removeAction);

    if (d->showRedundantEdges) {
        auto *hideEdgesAction = new QAction();
        hideEdgesAction->setText(tr("Hide redundant edges"));
        connect(hideEdgesAction, &QAction::triggered, this, [this] {
            showRedundantRelations(false);
            showChildRedundantRelations(false);
        });
        retValues.append(hideEdgesAction);
    } else {
        auto *showEdgesAction = new QAction();
        showEdgesAction->setText(tr("Show redundant edges"));
        connect(showEdgesAction, &QAction::triggered, this, [this] {
            showRedundantRelations(true);
            showChildRedundantRelations(true);
        });
        retValues.append(showEdgesAction);
    }

    if (lakosEntities().empty()) {
        return retValues;
    }

    if (d->isExpanded) {
        auto *action = new QAction(tr("Collapse"));
        connect(action, &QAction::triggered, this, [this, scenePosition] {
            toggleExpansion(QtcUtil::CreateUndoAction::e_Yes, scenePosition, RelayoutBehavior::e_RequestRelayout);
        });
        retValues.append(action);
    } else {
        auto *action = new QAction(tr("Expand"));
        connect(action, &QAction::triggered, this, [this] {
            toggleExpansion(QtcUtil::CreateUndoAction::e_Yes, std::nullopt, RelayoutBehavior::e_RequestRelayout);
        });
        retValues.append(action);
    }

    if (d->isExpanded) {
        if (d->isOpaque) {
            auto *action = new QAction(tr("Show Content"));
            action->setToolTip(
                tr("Uncovers this entity, showing all entities "
                   "within it, but does not uncovers the children "
                   "entities if they are covered."));

            connect(action, &QAction::triggered, this, [this] {
                showContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_Yes);
                Q_EMIT graphUpdate();
            });
            retValues.append(action);

            action = new QAction(tr("Show Content Recursively"));
            action->setToolTip(tr("Uncovers this entity, and all boxes within it, recursively"));
            connect(action, &QAction::triggered, this, [this] {
                showContent(ToggleContentBehavior::Recursive, QtcUtil::CreateUndoAction::e_Yes);
                Q_EMIT graphUpdate();
            });

            retValues.append(action);
        } else {
            // We can't have a non recursive version for the hide, because
            // edges crossing from other containers will appear quite broken.
            auto *action = new QAction(tr("Hide Content"));
            action->setToolTip(tr("Covers this entity, and all boxes within it, recursively"));
            connect(action, &QAction::triggered, this, [this] {
                hideContent(ToggleContentBehavior::Recursive, QtcUtil::CreateUndoAction::e_Yes);
                Q_EMIT graphUpdate();
            });
            retValues.append(action);
        }
    }

    return retValues;
}

std::unordered_map<LakosEntity *, int> LakosEntity::childrenLevels() const
{
    auto children = lakosEntities();
    auto childrenStdVec = std::vector<LakosEntity *>(children.begin(), children.end());
    return computeLevelForEntities(childrenStdVec, this);
}

std::unique_ptr<QDialog> LakosEntity::createNotesDialog()
{
    auto notesDialog = std::make_unique<QDialog>();
    notesDialog->setWindowTitle(tr("%1 Notes").arg(QString::fromStdString(name())));
    auto *notesTextEdit = new MRichTextEdit();
    auto *layout = new QBoxLayout(QBoxLayout::TopToBottom);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    notesDialog->setLayout(layout);
    layout->addWidget(notesTextEdit);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, notesDialog.get(), &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, notesDialog.get(), &QDialog::reject);
    connect(notesDialog.get(), &QDialog::accepted, this, [=, this] {
        const std::string oldNotes = d->node->notes();
        const std::string newNotes = notesTextEdit->toHtml().toStdString();
        const QString text = notesTextEdit->toHtml();
        d->node->setNotes(text.toStdString());

        Q_EMIT undoCommandCreated(
            new UndoNotes(qualifiedName(), instanceType(), oldNotes, newNotes, qobject_cast<GraphicsScene *>(scene())));
    });
    return notesDialog;
}

void LakosEntity::showNotesDialog()
{
    auto notesDialog = createNotesDialog();
    auto *textEdit = notesDialog->findChild<MRichTextEdit *>();
    textEdit->setText(QString::fromStdString(d->node->notes()));
    notesDialog->exec();
}

void LakosEntity::populateMenu(QMenu& menu, QMenu *debugMenu, QPointF scenePosition)
{
    // We need to test the parent to see if we can access this element.
    // if the parent is covered, the contextMenu is just for the parent.
    auto *containerParent = dynamic_cast<LakosEntity *>(parentItem());
    if (containerParent) {
        if (containerParent->isCovered()) {
            return;
        }
    }

    if (d->pluginManager) {
        auto& pm = d->pluginManager->get();
        auto getEntity = [this]() {
            return createWrappedEntityFromLakosEntity(this);
        };

        auto addReport = [this, &menu](std::string const& actionLabel,
                                       std::string const& reportTitle,
                                       std::function<void(PluginEntityReportActionHandler *)> const& userAction) {
            auto *action = menu.addAction(QString::fromStdString(actionLabel));
            connect(action, &QAction::triggered, this, [this, reportTitle, userAction]() {
                auto& pm = d->pluginManager->get();
                auto getPluginData = [&pm](auto&& id) { // clazy:exclude=lambda-in-connect
                    return pm.getPluginData(id);
                };
                auto getEntity = [this]() {
                    return createWrappedEntityFromLakosEntity(this);
                };
                auto reportContents = std::string{};
                // Clazy is afraid of the lambda capture by reference below, but it is safe since the lambda is consumed
                // only within the scope of the current function, and not used elsewhere. There's a bug report on:
                // https://bugs.kde.org/show_bug.cgi?id=443342
                auto setReportContents =
                    [&reportContents](std::string const& contentsHTML) { // clazy:exclude=lambda-in-connect
                        reportContents = contentsHTML;
                    };

                auto h = PluginEntityReportActionHandler{getPluginData, getEntity, setReportContents};
                userAction(&h);

                Q_EMIT createReportActionClicked(reportTitle, reportContents);
            });
        };

        pm.callHooksSetupEntityReport(getEntity, addReport);
    }

    QList<QAction *> actions = actionsForMenu(scenePosition);
    if (actions.empty()) {
        return;
    }

    {
        auto *action = menu.addAction(QString::fromStdString(name()));
        action->setToolTip(tr("Copy element name"));
        connect(action, &QAction::triggered, this, [this] {
            auto *clip = QGuiApplication::clipboard();
            clip->setText(QString::fromStdString(name()));
        });
    }

    menu.addSeparator();

    if (debugMenu) {
        auto *toggleBackgroundAction = debugMenu->addAction(tr("Toggle background"));
        toggleBackgroundAction->setCheckable(true);
        toggleBackgroundAction->setChecked(d->showBackground);

        connect(toggleBackgroundAction, &QAction::triggered, this, [this] {
            d->showBackground = !d->showBackground;
            updateBackground();
        });

        auto *toggleCenterPoint = debugMenu->addAction(tr("Toggle Center"));
        toggleCenterPoint->setChecked(d->centerDbg ? d->centerDbg->isVisible() : false);

        connect(toggleCenterPoint, &QAction::triggered, this, [this](bool toggled) {
            if (!d->centerDbg) {
                d->centerDbg = new QGraphicsEllipseItem(-5, -5, 10, 10);
                d->centerDbg->setPos(pos());
                d->centerDbg->setZValue(zValue() + 1);
                scene()->addItem(d->centerDbg);
            }
            d->centerDbg->setVisible(toggled);
        });

        auto *boundingRectBtn = debugMenu->addAction(tr("Show Enveloping Rectangle"));
        boundingRectBtn->setToolTip(
            tr("Displays the rectangle of this item, <br/> that might be bigger than what the visual element is."));
        boundingRectBtn->setCheckable(true);
        boundingRectBtn->setChecked(false);
        connect(boundingRectBtn, &QAction::triggered, this, [this](bool checked) {
            if (!checked) {
                delete d->boundingRectDbg;
                d->boundingRectDbg = nullptr;
                return;
            }

            delete d->boundingRectDbg;
            d->boundingRectDbg = new QGraphicsRectItem(boundingRect());
            d->boundingRectDbg->setPos(pos());
            // we need to set the bounding rect behind this item for mouse event propagation
            d->boundingRectDbg->setZValue(zValue() - 1);
            scene()->addItem(d->boundingRectDbg);
            d->boundingRectDbg->setVisible(true);
        });

        auto *childBoundingRect = debugMenu->addAction(tr("Show Children Rectangle"));
        childBoundingRect->setToolTip(
            tr("Display this element's children bounding rectangle <br/> This should be the same size of the item if "
               "expanded <br/> and larger if shrinked."));
        childBoundingRect->setCheckable(true);
        childBoundingRect->setChecked(false);
        connect(childBoundingRect, &QAction::triggered, this, [this](bool checked) {
            if (!checked) {
                delete d->boundingRectDbg;
                d->boundingRectDbg = nullptr;
                return;
            }

            delete d->boundingRectDbg;
            d->boundingRectDbg = new QGraphicsRectItem(childrenBoundingRect());
            d->boundingRectDbg->setPos(pos());
            // we need to set the bounding rect behind this item for mouse event propagation
            d->boundingRectDbg->setZValue(zValue() - 1);
            scene()->addItem(d->boundingRectDbg);
            d->boundingRectDbg->setVisible(true);
        });
    }

    for (auto *action : actions) {
        menu.addAction(action);
    }
}

bool LakosEntity::isBlockingEvents() const
{
    auto *containerParent = dynamic_cast<LakosEntity *>(parentItem());
    if (containerParent) {
        return containerParent->isCovered();
    }
    return false;
}

/* Variables that control the Moving / Dragging of the LakosEntities. */
namespace {
// Shared variables. Only one element can be moved at a time.
bool s_isDraggingItem = false; // NOLINT
// Are we moving this node with the mouse?

bool s_tooSlowToMove = false; // NOLINT
// Qt does not manages correctly huge items,
// and if we have an item that's too
// slow to move, we need to use a hack.
// this is visible if we try to load bal/balb and try to move
// BSL. 80% of the time is on PrepareGeometryChange and moving
// a single pixel can take up to 1 second.

std::optional<QPointF> s_originalPos; // NOLINT
// holds the original position before a move starts.

QVector2D s_movementDelta; // NOLINT
// When in drag movement, this is the relative amount of
// movement we did at each mouseMove event.

QGraphicsSimpleTextItem *s_dragTextItem = nullptr; // NOLINT
// if it's too slow to move, show a text to the new position.
} // namespace

void LakosEntity::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        Q_EMIT toggleSelection();
        ev->accept();
        return;
    }
}

void LakosEntity::mousePressEvent(QGraphicsSceneMouseEvent *ev)
{
    s_lastClick = ev->scenePos();

    qDebug() << "LakosEntity MousePressEvent" << QString::fromStdString(name());

    if (isBlockingEvents()) {
        // If it has a parent item, then it's an item inside a container.
        if (parentItem()) {
            ev->ignore();
            return;
        }
        ev->accept();
        return;
    }

    if (ev->button() != Qt::LeftButton || ev->modifiers() != Qt::KeyboardModifier::NoModifier) {
        ev->ignore();
        return;
    }

    QLineF line(mapToParent(ev->pos()), pos());
    s_isDraggingItem = true;
    s_originalPos = pos();

    // it isn't clear why Qt uses qreal (double) for QLineF, but float for
    // QVector2D (decltype(s_movementDelta)). Anyway, float should be enough
    // precision for this use case
    s_movementDelta.setX(static_cast<float>(line.dx()));
    s_movementDelta.setY(static_cast<float>(line.dy()));

    Q_EMIT dragStarted();
}

void LakosEntity::mouseMoveEvent(QGraphicsSceneMouseEvent *ev)
{
    if (!s_isDraggingItem) {
        return;
    }

    const auto nextPos = mapToParent(ev->pos() + s_movementDelta.toPointF());
    if (nextPos == pos()) {
        return;
    }

    if (!s_tooSlowToMove) {
        QElapsedTimer timer;
        timer.start();
        setPos(nextPos);
        if (timer.elapsed() > 35) { // ~30 FPS
            s_tooSlowToMove = true;
            s_dragTextItem = new QGraphicsSimpleTextItem();
            scene()->addItem(s_dragTextItem);
            s_dragTextItem->setPos(mapToScene(ev->pos()));
        } else {
            Q_EMIT moving();
        }
        if (d->boundingRectDbg) {
            d->boundingRectDbg->setPos(nextPos);
        }
        if (d->centerDbg) {
            d->centerDbg->setPos(nextPos);
        }
    } else {
        s_dragTextItem->setText(tr("Move Update is too slow\nFrom (%1, %2) to (%3, %4)")
                                    .arg(s_originalPos->x())
                                    .arg(s_originalPos->y())
                                    .arg(ev->pos().x())
                                    .arg(ev->pos().y()));
        s_dragTextItem->setFlag(ItemIgnoresTransformations);
        s_dragTextItem->setPos(mapToScene(ev->pos()));
        s_dragTextItem->setZValue(QtcUtil::e_INFORMATION);
    }
}

void LakosEntity::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev)
{
    if (Preferences::enableDebugOutput()) {
        qDebug() << "LakosEntity MouseReleaseEvent" << QString::fromStdString(name());
    }

    if (s_isDraggingItem) {
        s_isDraggingItem = false;
        const QPointF newPos = mapToParent(ev->pos() + s_movementDelta.toPointF());

        // QFuzzyCompare fails here.
        constexpr float EPISILON = 0.00001;

        const bool xEqual = fabs(newPos.x() - s_originalPos.value().x()) < EPISILON;
        const bool yEqual = fabs(newPos.y() - s_originalPos.value().y()) < EPISILON;
        if (!xEqual || !yEqual) {
            auto *thisScene = qobject_cast<GraphicsScene *>(scene());
            Q_EMIT undoCommandCreated(new UndoMove(thisScene, d->qualifiedName, *s_originalPos, newPos));

            recursiveEdgeRelayout();
            Q_EMIT graphUpdate();
        }
        s_originalPos = {};
        Q_EMIT dragFinished();

        if (s_tooSlowToMove) {
            s_tooSlowToMove = false;
            delete s_dragTextItem;
            s_dragTextItem = nullptr;
        }
    }

    if (s_lastClick != ev->scenePos()) {
        return;
    }

    ev->accept();

    // Hide or show content based on the number of children.
    if (isExpanded() && !lakosEntities().empty()) {
        if (ev->button() == Qt::LeftButton) {
            if (d->isOpaque) {
                showContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_Yes);
                Q_EMIT graphUpdate();
            } else {
                hideContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_Yes);
                Q_EMIT graphUpdate();
            }
            Q_EMIT coverChanged();
            return;
        }
    }
}

void LakosEntity::recursiveEdgeRelayout()
{
    if (!layoutUpdatesEnabled()) {
        return;
    }

    if (!d->isOpaque) {
        for (auto *child : d->lakosChildren) {
            if (child->isVisible()) {
                child->recursiveEdgeRelayout();
            }
        }
    }

    for (auto& edges : d->edges) {
        if (!edges->isRedundant() || d->showRedundantEdges) {
            edges->layoutRelations();
        }
    }

    for (auto& edges : d->targetEdges) {
        if (!edges->isRedundant() || d->showRedundantEdges) {
            edges->layoutRelations();
        }
    }
}

void LakosEntity::layoutAllRelations()
{
    if (!layoutUpdatesEnabled()) {
        return;
    }

    for (auto& edges : d->edges) {
        edges->layoutRelations();
    }
    for (auto& edges : d->targetEdges) {
        edges->layoutRelations();
    }
}

void LakosEntity::setMainEntity()
{
    d->isMainNode = true;
    updateBackground();
}

void LakosEntity::recursiveEdgeHighlight(bool highlight)
{
    for (auto *item : childItems()) {
        auto *childEntity = dynamic_cast<LakosEntity *>(item);
        if (childEntity) {
            for (const auto& eCollection : childEntity->edgesCollection()) {
                eCollection->setHighlighted(highlight);
            }
        }
    }
    for (const auto& eCollection : edgesCollection()) {
        eCollection->setHighlighted(highlight);
    }
}

void LakosEntity::hoverEnterEvent(QGraphicsSceneHoverEvent *ev)
{
    if (isBlockingEvents()) {
        return;
    }

    Q_UNUSED(ev);

    auto *pItem = dynamic_cast<LakosEntity *>(parentItem());
    if (!pItem) {
        recursiveEdgeHighlight(true);
    }

    if (signalsBlocked()) {
        return;
    }

    if (ev->modifiers()) {
        return;
    }

    QGuiApplication::setOverrideCursor(Qt::CursorShape::PointingHandCursor);

    auto *ourScene = qobject_cast<GraphicsScene *>(scene());
    if (ourScene && !ourScene->blockNodeResizeOnHover()) {
        if (!d->isExpanded) {
            setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        }
    }

    if (!isMainEntity()) {
        if (d->showBackground) {
            QBrush thisBrush = brush();
            const QColor& thisColor = thisBrush.color();
            setBrush(QBrush(QColor(thisColor.red(), thisColor.green(), thisColor.blue(), 180)));
        }
    }
}

void LakosEntity::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev)
{
    if (isBlockingEvents()) {
        return;
    }

    Q_UNUSED(ev);
    QGuiApplication::restoreOverrideCursor();

    auto *pItem = dynamic_cast<LakosEntity *>(parentItem());
    if (!pItem) {
        recursiveEdgeHighlight(false);
    }

    setFlag(QGraphicsItem::ItemIgnoresTransformations, false);

    if (!isMainEntity()) {
        if (d->showBackground) {
            QBrush thisBrush = brush();
            const QColor& thisColor = thisBrush.color();
            setBrush(QBrush(QColor(thisColor.red(), thisColor.green(), thisColor.blue())));
        }
    }
}

void LakosEntity::updateBackground()
{
    if (!d->showBackground) {
        QBrush b = QBrush();
        b.setColor(QColor(10, 10, 10, 0));
        setBrush(b);
        return;
    }

    if (!layoutUpdatesEnabled()) {
        return;
    }

    if (d->isMainNode) {
        const QRectF bRect = rect();
        if (d->enableGradientOnMainNode) {
            QLinearGradient linearGrad(bRect.topLeft(), bRect.bottomRight());
            linearGrad.setColorAt(0, d->color);
            linearGrad.setColorAt(1, d->color.lighter());
            setBrush(QBrush(linearGrad));
        } else {
            QBrush b = brush();
            b.setColor(d->selectedNodeColor);
            b.setStyle(d->fillPattern);
            setBrush(b);
        }
        setPen(QPen(QBrush(Qt::blue), 1));
    } else {
        QBrush b = brush();
        b.setColor(d->color);
        b.setStyle(d->fillPattern);
        setBrush(b);
    }
}

void LakosEntity::setColorManagement(lvtclr::ColorManagement *colorManagement)
{
    assert(colorManagement);
    if (Preferences::colorBlindMode()) {
        d->color = colorManagement->getColorFor(d->colorId);
    } else {
        d->color = Preferences::entityBackgroundColor();
    }
    d->fillPattern = colorManagement->fillPattern(d->colorId);

    updateBackground();

    connect(colorManagement, &lvtclr::ColorManagement::requestNewColor, this, [this, colorManagement] {
        if (Preferences::colorBlindMode()) {
            d->color = colorManagement->getColorFor(d->colorId);
        } else {
            d->color = Preferences::entityBackgroundColor();
        }
        d->fillPattern = colorManagement->fillPattern(d->colorId);
        updateBackground();
    });
}

void LakosEntity::makeToolTip(const std::string& noColorStr)
{
    if (d->toolTip.length()) {
        setToolTip(QString::fromStdString(d->toolTip));
        return;
    }

    std::string toolTip(d->qualifiedName + '\n');
    if (d->colorId.empty()) {
        toolTip += noColorStr;
    } else {
        toolTip += d->colorId;
    }
    setToolTip(QString::fromStdString(toolTip));
}

void LakosEntity::setColorId(const std::string& colorId)
{
    if (d->colorId == colorId) {
        return;
    }
    d->colorId = colorId;

    if (layoutUpdatesEnabled()) {
        update();
    }
}

void LakosEntity::enableLayoutUpdates()
{
    d->layoutUpdatesEnabled = true;

    // We need to invert the logic of the isExpanded
    // here, because we set it when the layout updates
    // where disabled, so invert, and toggle back to
    // the correct value - doing the calculations.
    d->isExpanded = !d->isExpanded;
    toggleExpansion(QtcUtil::CreateUndoAction::e_No);

    updateBackground();
    layoutAllRelations();

    // After testing a bit, the best scenario on enableLayoutUpdates
    // is to hide everything recursively, but show only the toplevel.
    if (d->isOpaque) {
        hideContent(ToggleContentBehavior::Recursive, QtcUtil::CreateUndoAction::e_No);
    } else {
        showContent(ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
    }
}

void LakosEntity::setTextPos(qreal x, qreal y)
{
    const auto textRect = d->text->boundingRect();
    d->text->setPos(mapFromScene(QPointF(x - textRect.width() / 2, y - textRect.height() / 2)));
}

QPointF LakosEntity::getTextPos() const
{
    const QPointF pos = d->text->pos();
    const QRectF textRect = d->text->boundingRect();
    return mapToScene(pos.x() + textRect.width() / 2, pos.y() + textRect.height() / 2);
}

void LakosEntity::removeEdge(LakosRelation *relation)
{
    auto removeEdgeFrom = [relation](std::vector<std::shared_ptr<EdgeCollection>>& collection) {
        auto itSource = std::find_if(std::begin(collection),
                                     std::end(collection),
                                     [relation](const std::shared_ptr<EdgeCollection>& ec) {
                                         return ec && (ec->from() == relation->from() && ec->to() == relation->to());
                                     });

        if (itSource != std::end(collection)) {
            auto& ec = *itSource;
            ec->removeEdge(relation);
            if (ec->relations().empty()) {
                collection.erase(itSource);
            }
        }
    };

    removeEdgeFrom(d->edges);
    removeEdgeFrom(d->targetEdges);
    removeEdgeFrom(d->redundantRelations);
}

bool LakosEntity::isCoveredByParent() const
{
    auto *parent = dynamic_cast<LakosEntity *>(parentItem());
    while (parent) {
        if (parent->isCovered()) {
            return true;
        }
        parent = dynamic_cast<LakosEntity *>(parent->parentItem());
    }
    return false;
}

bool LakosEntity::isParentCollapsed() const
{
    auto *parent = dynamic_cast<LakosEntity *>(parentItem());
    while (parent) {
        if (!parent->isExpanded()) {
            return true;
        }
        parent = dynamic_cast<LakosEntity *>(parent->parentItem());
    }
    return false;
}

bool LakosEntity::hasRelationshipWith(LakosEntity *entity) const
{
    if (entity == nullptr) {
        return false;
    }

    return std::any_of(d->edges.begin(), d->edges.end(), [entity](const auto& ec) {
        return ec->to() == entity;
    });
}

std::shared_ptr<EdgeCollection> LakosEntity::getRelationshipWith(LakosEntity *entity) const
{
    for (auto const& ec : d->edges) {
        if (ec->to() == entity) {
            return ec;
        }
    }
    return nullptr;
}

/* This is really about the children, not this node. We want to know if the
 * child has an edge that crosses boundaries with the boundingRect of this entity.
 */
bool LakosEntity::childrenHasVisibleRelationshipWith(LakosEntity *otherEntity) const
{
    for (auto *child : d->lakosChildren) {
        if (child->childrenHasVisibleRelationshipWith(otherEntity)) {
            return true;
        }

        for (const auto& collection : {child->edgesCollection(), child->targetCollection()}) {
            for (const auto& ec : collection) {
                if (ec->from() == otherEntity || ec->to() == otherEntity) {
                    for (LakosRelation *relation : ec->relations()) {
                        if (relation->isVisible()) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool childrenConnected(LakosEntity *a, LakosEntity *b)
{
    for (LakosEntity *childA : a->lakosEntities()) {
        for (const LakosEntity *childB : b->lakosEntities()) {
            for (const auto& ec : childA->edgesCollection()) {
                if (ec->to() == childB) {
                    return true;
                }
            }
            for (const auto& ec : childA->targetCollection()) {
                if (ec->from() == childB) {
                    return true;
                }
            }
        }
    }
    return false;
}

void LakosEntity::calculateEdgeVisibility(const std::shared_ptr<EdgeCollection>& ec)
{
    /* Edge Visibility Rules: check file lvtqtc/edge_rules.svg */
    // TODO: let's remove those std::shared_ptr's. this is just adding complexity.
    if (!ec) {
        return;
    }

    // rule 1: Entities inside a collapsed or covered panel are always hidden
    auto *from = ec->from();
    auto *to = ec->to();
    if (from->isParentCollapsed() || to->isParentCollapsed()) {
        ec->setVisible(false);
        return;
    }

    // Rule 2 - hide redudant edges, if we can.
    if (ec->isRedundant() && !d->showRedundantEdges) {
        ec->setVisible(false);
        return;
    }

    auto *pFrom = qgraphicsitem_cast<LakosEntity *>(from->parentItem());
    auto *pTo = qgraphicsitem_cast<LakosEntity *>(to->parentItem());
    if (pFrom && pTo && pFrom == pTo) {
        ec->setVisible(true);
        return;
    }

    if (from->isCoveredByParent() || to->isCoveredByParent()) {
        ec->setVisible(false);
        return;
    }

    // Rule 3, we (unfortunately) really need to check if a child
    // has a visible edge to a node, before drawing this edge.
    // it would be really good to not have a specific rule for this, but,
    // I could think of a way to make this more generic.
    LakosEntity *otherEntity = ec->from() == this ? ec->to() : ec->from();
    if (childrenHasVisibleRelationshipWith(otherEntity)) {
        ec->setVisible(false);
        return;
    }

    // Rule 4 - If there's a connection between 2 entities, but no connections between the children, we must show the
    // parent edge.
    if (!childrenConnected(ec->from(), ec->to())) {
        ec->setVisible(true);
        return;
    }

    // Rule 5 - If there's a connection between 2 entities, and also between the children, we must only show the parent
    // edge if it is covering the children.
    const bool fVisibility = ec->from()->isCovered() || !ec->from()->isExpanded();
    const bool tVisibility = ec->to()->isCovered() || !ec->to()->isExpanded();
    ec->setVisible(fVisibility || tVisibility);
}

void LakosEntity::calculateEdgeVisibility()
{
    // When we cover / uncover, collapse, etc. this might
    // affect edges on children, so this needs to be recursive.
    for (const auto& child : d->lakosChildren) {
        child->calculateEdgeVisibility();
    }

    for (const auto& ec : d->edges) {
        calculateEdgeVisibility(ec);
    }

    for (const auto& ec : d->targetEdges) {
        calculateEdgeVisibility(ec);
    }
}

void LakosEntity::toggleCover(ToggleContentBehavior behavior, QtcUtil::CreateUndoAction create)
{
    if (d->isOpaque) {
        showContent(behavior, create);
    } else {
        hideContent(behavior, create);
    }
}

void LakosEntity::hideContent(ToggleContentBehavior behavior, QtcUtil::CreateUndoAction create)
{
    if (create == QtcUtil::CreateUndoAction::e_Yes) {
        Q_EMIT undoCommandCreated(new UndoCover(qobject_cast<GraphicsScene *>(scene()), d->id));
        return;
    }

    if (lakosEntities().empty()) {
        return;
    }

    d->isOpaque = true;

    if (!layoutUpdatesEnabled()) {
        return;
    }

    d->cover->setRectangle(rect());
    d->cover->setRoundRadius(roundRadius());

    d->cover->setVisible(true);

    calculateEdgeVisibility();

    for (LakosEntity *child : d->lakosChildren) {
        if (behavior == ToggleContentBehavior::Recursive) {
            child->hideContent(behavior, QtcUtil::CreateUndoAction::e_No);
        }
    }
    recursiveEdgeRelayout();
}

void LakosEntity::showContent(ToggleContentBehavior behavior, QtcUtil::CreateUndoAction create)
{
    if (create == QtcUtil::CreateUndoAction::e_Yes) {
        Q_EMIT undoCommandCreated(new UndoCover(qobject_cast<GraphicsScene *>(scene()), d->id));
        return;
    }

    d->isOpaque = false;

    if (!layoutUpdatesEnabled()) {
        return;
    }

    d->cover->setVisible(false);

    calculateEdgeVisibility();

    for (LakosEntity *child : d->lakosChildren) {
        if (behavior == ToggleContentBehavior::Recursive) {
            child->showContent(behavior, QtcUtil::CreateUndoAction::e_No);
        }
    }
    recursiveEdgeRelayout();
}

void LakosEntity::reactChildRemoved(QGraphicsItem *child)
{
    if (auto *lEntity = qgraphicsitem_cast<LakosEntity *>(child)) {
        // This is not slow as there is no memory deallocations / reallocations
        // for the remove. the vector will be completely deallocated only
        // on destruction of this object.
        d->lakosChildren.removeOne(lEntity);
        updateChildrenLoadedInfo();

        if (!d->lakosChildren.isEmpty()) {
            recalculateRectangle();
        } else {
            shrink(QtcUtil::CreateUndoAction::e_No);
        }
    }
}

void LakosEntity::reactChildAdded(QGraphicsItem *child)
{
    if (auto *lEntity = qgraphicsitem_cast<LakosEntity *>(child)) {
        d->lakosChildren.append(lEntity);
        if (d->lakosChildren.size() == 1 && !d->isExpanded) {
            // Automatically expand a package that just received it's first item
            expand(QtcUtil::CreateUndoAction::e_No);
        }

        // update our size now we have a new child
        recalculateRectangle();

        // update our size whenever a child is moved
        connect(lEntity, &LakosEntity::moving, this, &LakosEntity::recalculateRectangle);

        // update our size whenever a child is resized
        connect(lEntity, &LakosEntity::rectangleChanged, this, &LakosEntity::recalculateRectangle);

        updateChildrenLoadedInfo();
    }
}

void LakosEntity::updateChildrenLoadedInfo()
{
    auto drawMissingChildsMark = [&]() {
        if (d->notAllChildrenLoadedInfo != nullptr) {
            return;
        }
        d->notAllChildrenLoadedInfo = new QGraphicsSimpleTextItem(QStringLiteral("..."));
        d->notAllChildrenLoadedInfo->setParentItem(this);
        d->notAllChildrenLoadedInfo->setVisible(true);
        d->notAllChildrenLoadedInfo->setBrush(QBrush(Qt::gray));
        d->notAllChildrenLoadedInfo->setPen(Qt::NoPen);
        d->ignoredItems.push_back(d->notAllChildrenLoadedInfo);
    };
    auto removeMissingChildsMark = [&]() {
        if (d->notAllChildrenLoadedInfo == nullptr) {
            return;
        }
        auto it = std::remove(std::begin(d->ignoredItems), std::end(d->ignoredItems), d->notAllChildrenLoadedInfo);
        d->ignoredItems.erase(it, std::end(d->ignoredItems));
        delete d->notAllChildrenLoadedInfo;
        d->notAllChildrenLoadedInfo = nullptr;
    };

    auto numberOfChildrenLoadedOnScreen = static_cast<size_t>(d->lakosChildren.size());
    auto numberOfChildrenAvailableOnModel = d->node->children().size();
    if (numberOfChildrenAvailableOnModel != numberOfChildrenLoadedOnScreen) {
        drawMissingChildsMark();
    } else {
        removeMissingChildsMark();
    }
}

QVariant LakosEntity::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    GraphicsRectItem::itemChange(change, value);

    switch (change) {
    case QGraphicsItem::ItemChildRemovedChange:
        reactChildRemoved(value.value<QGraphicsItem *>());
        break;
    case QGraphicsItem::ItemChildAddedChange:
        reactChildAdded(value.value<QGraphicsItem *>());
        break;
    case QGraphicsItem::ItemSelectedHasChanged: {
        for (auto&& e : d->edges) {
            e->toggleRelationFlags(EdgeCollection::RelationIsSelected, value.value<bool>());
        }
        for (auto&& e : d->targetEdges) {
            e->toggleRelationFlags(EdgeCollection::RelationIsSelected, value.value<bool>());
        }
        break;
    }
    default:
        break;
    }

    return value;
}

void LakosEntity::showRedundantRelations(bool show)
{
    d->showRedundantEdges = show;
    calculateEdgeVisibility();
}

void LakosEntity::showChildRedundantRelations(bool show)
{
    for (LakosEntity *entity : lakosEntities()) {
        entity->showRedundantRelations(show);
        entity->showChildRedundantRelations(show);
    }
}

std::string LakosEntity::colorIdText() const
{
    if (colorId().empty()) {
        return name() + " has no namespace";
    }

    return name() + " has namespace " + colorId();
}

std::string LakosEntity::legendText() const
{
    const auto bRect = boundingRect();
    std::string information;
    information += "Name: " + name() + "\n";
    information += "Qualified Name: " + qualifiedName() + "\n";
    information += "Color ID: " + colorIdText() + "\n";

    if (QtcUtil::isDebugMode() && Preferences::enableSceneContextMenu()) {
        information += "Cover Status " + std::to_string(d->isOpaque) + "\n";
        information += "Item Visibility: " + std::to_string(d->cover->isVisible()) + "\n";
        information += "Pos: " + std::to_string(pos().x()) + ',' + std::to_string(pos().y()) + "\n";
        information +=
            "Top Left: " + std::to_string(bRect.topLeft().x()) + ',' + std::to_string(bRect.topLeft().y()) + "\n";
        information += "Bottom Right: " + std::to_string(bRect.bottomRight().x()) + ','
            + std::to_string(bRect.bottomRight().y()) + "\n";
    }

    information += "\n";

    auto displayEdgesLambda =
        [](const std::string& none, const std::string& some, std::string& information, auto& edges) {
            if (edges.empty()) {
                information += none + '\n';
            } else {
                information += some + ": " + std::to_string(edges.size()) + '\n';
                std::string edge_names;
                for (auto& edge : edges) {
                    edge_names += edge->to()->name() + " ";
                    if (edge_names.size() > 40) {
                        information += edge_names + "\n";
                        edge_names = std::string{};
                    }
                }
                information += "\n";
            }
        };

    displayEdgesLambda("No outgoing edges", "Connections from this node", information, d->edges);
    information += "\n";
    displayEdgesLambda("No incoming edges", "Connections to this node", information, d->targetEdges);
    information += "\n";

    return information;
}

void LakosEntity::recalculateRectangle()
{
    if (!layoutUpdatesEnabled()) {
        return;
    }

    constexpr int TEXT_ADJUST = 20;
    if (!d->isExpanded) {
        const QRectF textRect = d->text->boundingRect().adjusted(-TEXT_ADJUST, // x1
                                                                 -TEXT_ADJUST, // y1
                                                                 TEXT_ADJUST, // x2
                                                                 TEXT_ADJUST); // y2
        setRectangle(textRect);
        layoutIgnoredItems();
        return;
    }

    // we can't use childrenBoundingRect as it takes in consideration the size of
    // the children of the children, even if they are hidden.
    QRectF childRect;
    for (auto *child : lakosEntities()) {
        if (!child->isVisible()) {
            continue;
        }

        QRectF thisChild = child->boundingRect();
        QPointF thisChildPoint = child->mapToParent(child->boundingRect().center());

        if (child->isExpanded()) {
            // We need to take in consideratio nthe text size at the bottom.
            thisChild.setHeight(thisChild.height() + TEXT_ADJUST);
        }

        thisChild.moveCenter(thisChildPoint);
        childRect = childRect.united(thisChild);
    }

    const int border =
        d->lakosChildren.size() <= 1 ? ONE_CHILD_BORDER : static_cast<int>(d->text->boundingRect().height());
    setRectangle(childRect.adjusted(-border, -border, border, border));
    layoutIgnoredItems();
}

// ONE LINERS.
QList<LakosEntity *> LakosEntity::lakosEntities() const
{
    return d->lakosChildren;
}

int LakosEntity::type() const
{
    return Type;
}

std::string LakosEntity::name() const
{
    return d->name;
}

long long LakosEntity::shortId() const
{
    return d->shortId;
}

std::string LakosEntity::qualifiedName() const
{
    return d->qualifiedName;
}

std::string LakosEntity::colorId() const
{
    return d->colorId;
}

QColor LakosEntity::color() const
{
    return d->color;
}

std::vector<std::shared_ptr<EdgeCollection>>& LakosEntity::edgesCollection() const
{
    return d->edges;
}

bool LakosEntity::layoutUpdatesEnabled() const
{
    return d->layoutUpdatesEnabled;
}

std::string LakosEntity::tooltipString() const
{
    return d->toolTip;
}

const std::string& LakosEntity::uniqueIdStr() const
{
    return d->id;
}

[[nodiscard]] lvtshr::UniqueId LakosEntity::uniqueId() const
{
    return {instanceType(), d->shortId};
}

// TODO: This is being used only on the GraphicsScene on a single bit:
//  mutEntity->setIsBranch(qtcEntity->highlighted());
//  and there's no actual meaning of 'highlithed' at all.
//  either we fix this (rename for a correct meaning) or remove it.
bool LakosEntity::highlighted() const
{
    return d->highlighted;
}

bool LakosEntity::isMainEntity() const
{
    return d->isMainNode;
}

void LakosEntity::setRelationRedundant(const std::shared_ptr<EdgeCollection>& edgeCollection)
{
    d->redundantRelations.push_back(edgeCollection);
}

void LakosEntity::resetRedundantRelations()
{
    for (const auto& ec : d->redundantRelations) {
        ec->setRedundant(false);
    }

    d->redundantRelations.clear();
}

const std::vector<std::shared_ptr<EdgeCollection>>& LakosEntity::redundantRelationshipsCollection() const
{
    return d->redundantRelations;
}

std::vector<std::shared_ptr<EdgeCollection>>& LakosEntity::targetCollection() const
{
    return d->targetEdges;
}

const lvtshr::LoaderInfo& LakosEntity::loaderInfo() const
{
    return d->loaderInfo;
}

bool LakosEntity::isCovered() const
{
    return d->isOpaque;
}

bool LakosEntity::isExpanded() const
{
    return d->isExpanded;
}

void LakosEntity::addTargetCollection(const std::shared_ptr<EdgeCollection>& collection)
{
    d->targetEdges.push_back(collection);
}

void LakosEntity::ignoreItemOnLayout(QGraphicsItem *item)
{
    d->ignoredItems.push_back(item);
}

QList<LakosEntity *> LakosEntity::parentHierarchy() const
{
    QList<LakosEntity *> parents;
    QGraphicsItem *temp = parentItem();
    while (temp) {
        auto *pEntity = qgraphicsitem_cast<LakosEntity *>(temp);
        assert(pEntity);
        parents.prepend(pEntity);
        temp = temp->parentItem();
    }
    return parents;
}

void LakosEntity::updateZLevel()
{
    setZValue(isSelected() ? 100 : 1);
}

void LakosEntity::levelizationLayout(LevelizationLayoutType type, int direction, std::optional<QPointF> moveToPosition)
{
    auto entityToLevel = childrenLevels();
    for (auto [entity, level] : entityToLevel) {
        if (!entity->d->forceHideLevelNr) {
            entity->d->levelNr->setText(QString::number(level + 1));
        }
        entity->layoutIgnoredItems();
    }

    runLevelizationLayout(entityToLevel, type, direction);

    recalculateRectangle();
    if (moveToPosition) {
        setPos(*moveToPosition);
    }
    recursiveEdgeRelayout();
    auto *parent = qgraphicsitem_cast<LakosEntity *>(this->parentItem());
    while (parent) {
        parent->recalculateRectangle();
        parent = qgraphicsitem_cast<LakosEntity *>(parent->parentItem());
    }
}

void LakosEntity::truncateTitle(EllipsisTextItem::Truncate v)
{
    d->text->truncate(v);
}

void LakosEntity::forceHideLevelNumbers()
{
    d->forceHideLevelNr = true;
    d->levelNr->setVisible(false);
}

QJsonObject LakosEntity::toJson() const
{
    const auto rect = boundingRect();
    const auto children = lakosEntities();

    QJsonArray childrenJson;
    for (LakosEntity *childEntity : children) {
        childrenJson.append(childEntity->toJson());
    }

    QJsonObject posObj = {
        {"x", pos().x()},
        {"y", pos().y()},
    };

    QJsonObject jsonRect = {
        {"x", rect.x()},
        {"y", rect.y()},
        {"width", rect.width()},
        {"height", rect.height()},
    };

    QJsonObject thisEntity = {
        {"pos", posObj},
        {"rect", jsonRect},
        {"qualifiedName", QString::fromStdString(qualifiedName())},
        {"children", childrenJson},
        {"expanded", isExpanded()},
        {"covered", isCovered()},
    };

    return thisEntity;
}

void LakosEntity::fromJson(const QJsonObject& obj)
{
    const QJsonObject rectObj = obj["rect"].toObject();
    setRectangle(QRectF(rectObj["x"].toDouble(),
                        rectObj["y"].toDouble(),
                        rectObj["width"].toDouble(),
                        rectObj["height"].toDouble()));

    // After loading all elements, we need to cover / shrink if needed.
    if (obj["expanded"].toBool()) {
        expand(QtcUtil::CreateUndoAction::e_No);
    } else {
        shrink(QtcUtil::CreateUndoAction::e_No);
    }

    if (obj["covered"].toBool()) {
        hideContent(LakosEntity::ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
    }
}

void LakosEntity::setColor(const QColor& color)
{
    d->color = color;
    updateBackground();
}

// cppcheck-suppress constParameter // This parameter cannot be marked as const, but cppcheck thinks it can. Marking it
// as const wouldn't compile.
void LakosEntity::setPluginManager(Codethink::lvtplg::PluginManager& pm)
{
    d->pluginManager = pm;
}

} // end namespace Codethink::lvtqtc
