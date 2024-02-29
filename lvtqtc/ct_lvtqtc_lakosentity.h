// ct_lvtqtc_lakosentity.h                                          -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_LAKOSENTITY
#define INCLUDED_CT_LVTGRPS_LAKOSENTITY

#include <lvtqtc_export.h>

#include <ct_lvtclr_colormanagement.h>

#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_loaderinfo.h>
#include <ct_lvtshr_uniqueid.h>

#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_ellipsistextitem.h>
#include <ct_lvtqtc_graphicsrectitem.h>
#include <ct_lvtqtc_util.h>

#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>

#include <memory>
#include <unordered_map>

class QAction;
class QUndoCommand;

namespace Codethink::lvtldr {
class LakosianNode;
}
namespace Codethink::lvtqtc {
struct EdgeCollection;
class LakosRelation;

/*! \class LakosEntity lakos_entity.cpp lakos_entity.h
 *  \brief Represents and draws a Lakos Entity
 *
 * %LakosEntity draws a Lakos Entity, and handles tool tips
 * and mouse clicks
 */
class LVTQTC_EXPORT LakosEntity : public GraphicsRectItem {
    Q_OBJECT

  public:
    enum PresentationFlags { NoFlag = 0x00, Highlighted = 0x01 };

    enum class RelayoutBehavior : short { e_DoNotRelayout, e_RequestRelayout };

    enum class LevelizationLayoutType { Horizontal, Vertical };

    enum { Type = QtcUtil::LAKOSENTITY_TYPE };
    // for qgraphicsitem_cast magic

    LakosEntity(const std::string& uniqueId, lvtldr::LakosianNode *node, lvtshr::LoaderInfo info);
    ~LakosEntity() override;

    [[nodiscard]] const std::string& uniqueIdStr() const;
    /*! \brief Unique identifier string
     *
     * This string uniquely identifies a particular vertex
     * corresponding to a row in a database table, such
     * as the 'class_declaration' table. It consists of
     * the name of the table, followed by '#' and the
     * value of the id.
     *
     * Example: 'class_declaration#1234'.
     */

    [[nodiscard]] lvtldr::LakosianNode *internalNode() const;

    [[nodiscard]] lvtshr::UniqueId uniqueId() const;

    [[nodiscard]] std::string name() const;
    // returns the name of this node

    void setName(const std::string& name);
    // sets the name of this node.
    // The name is used to identify the node on screen, but there's
    // also `setText` that does the same.
    // TODO: simplify the API.

    void setPresentationFlags(PresentationFlags flags, bool value);

    void updateZLevel();
    // updates the z level based on selection state.

    void setQualifiedName(const std::string& qname);

    void setFont(const QFont& f);
    // sets the font of the current entity

    [[nodiscard]] long long shortId() const;
    // This is only guaranteed to be unique within the same instance type.
    // You almost certainly want to use uniqueId() instead

    [[nodiscard]] std::string tooltipString() const;
    // returns the tooltip for the node, excluding the qualifiedName variant
    // that is used when the tooltip string is empty. if you call
    // the Qt tooltip() method, this could return the qualified name.

    void setTooltipString(const std::string& tt);
    // set's the tooltip string to tt. if not set, the qualifiedName is used.

    virtual void updateTooltip() = 0;

    [[nodiscard]] std::string qualifiedName() const;
    // returns the qualified name of this node

    void setNotes(const std::string& notes);
    // set the notes for this entity.

    void setColorId(const std::string& colorId);
    // Sets the color id of this element.
    // The color id is used to
    // fetch a specific color from the ColorManagement.

    [[nodiscard]] std::string colorId() const;
    // returns the color id of this node.
    // The color id is used to
    // fetch a specific color from the ColorManagement.

    [[nodiscard]] QColor color() const;
    // returns the current color of this node.
    // TODO: Change this, the color is not important, but the brush is.
    // That's because the brush can hold more things than just a color,
    // and if we change the color of a brush that has a gradient, nothing
    // will change on the painted item.

    void setMainEntity();
    // Draws a different background to highlight that this is the main node.
    // TODO: rename to setAsMainEntity(). setMainEntity means that we are
    // setting a mainEntity inside of this element.

    [[nodiscard]] bool isMainEntity() const;
    // returns if this is the main node or not.

    LakosEntity *getTopLevelParent();
    // Get the top-most LakosEntity in the parent/child hierarchy

    virtual void updateBackground();
    // recalculates the background.

    [[nodiscard]] bool highlighted() const;
    // \brief Indicates if the entity is highlighted

    void setColorManagement(lvtclr::ColorManagement *colorManagement);
    // Sets color management of the entity

    [[nodiscard]] QList<LakosEntity *> lakosEntities() const;
    // TODO: pick one, std::vector or QList, don't mix them.

    [[nodiscard]] std::vector<std::shared_ptr<EdgeCollection>>& edgesCollection() const;
    // The edges that have this node as source.

    [[nodiscard]] std::vector<std::shared_ptr<EdgeCollection>>& targetCollection() const;
    // The edges that have this node as a Target.

    [[nodiscard]] const std::vector<std::shared_ptr<EdgeCollection>>& redundantRelationshipsCollection() const;
    // edges (that are also on the edgesCollection) that are redundant.

    void setRelationshipVisibility(QtcUtil::VisibilityMode mode);

    void removeEdge(LakosRelation *relation);
    // removes a relation from this node, and removes the edgesCollection
    // that hold that relation, if there are no more relations on it.

    void addTargetCollection(const std::shared_ptr<EdgeCollection>& collection);
    // add edges that have this node as target.

    void setRelationRedundant(const std::shared_ptr<EdgeCollection>& edgeCollection);
    // Mark a relation as redundant (via transitive reduction)

    void resetRedundantRelations();

    bool hasRelationshipWith(LakosEntity *entity) const;
    // returns true if we have a lakosConnection to this entity.

    std::shared_ptr<EdgeCollection> getRelationshipWith(LakosEntity *entity) const;

    void recursiveEdgeRelayout();
    // recalculate all the edges, that belongs to this entity and the
    // children.

    void recursiveEdgeHighlight(bool highlight);

    virtual void setText(const std::string& text);
    // Set the text label, this overrides the visible strings of the
    // element, where `name` is currently shown.
    // could be userful to merge those usecases.

    void setTextPos(qreal x, qreal y);
    // Set's the position of the text, in item coordinates.

    [[nodiscard]] QPointF getTextPos() const;
    // returns the position of the text

    void layoutAllRelations();
    // Call layoutRelations() on all edges

    [[nodiscard]] const lvtshr::LoaderInfo& loaderInfo() const;

    std::unique_ptr<QDialog> createNotesDialog();
    void showNotesDialog();

    void showRedundantRelations(bool show);
    // Control whether we should show redundant relations

    void populateMenu(QMenu& menu, QMenu *debugMenu, QPointF scenePosition);

    void showToggleExpansionButton(bool show);
    void toggleExpansion(QtcUtil::CreateUndoAction CreateUndoAction,
                         std::optional<QPointF> moveToPosition = std::nullopt,
                         RelayoutBehavior relayoutBehavior = RelayoutBehavior::e_DoNotRelayout);

    void recalculateRectangle();
    // recalculate the rectangle based on the child items,
    // but excluding the items specifically set to ignore.
    // Only appropriate for an expanded container.

    [[nodiscard]] bool isExpanded() const;
    // returns true if we are expanded, false if we are collapsed.
    // TODO: create a enum `VisibilityMode` to remove boolean traps.

    void calculateEdgeVisibility(const std::shared_ptr<EdgeCollection>& ec);
    void calculateEdgeVisibility();
    // calculates the visibility of all edges that leaves this node
    // and all edges that arrive on this node, and set the appropriate
    // value for it, recursively.

    [[nodiscard]] bool isParentCollapsed() const;
    // returns true if any parent of this LakosEntity is collapsed.

    bool childrenHasVisibleRelationshipWith(LakosEntity *otherEntity) const;
    // Do we have a visible relationship with the entity, from one of the
    // childrens?

    [[nodiscard]] std::string legendText() const;
    // Returns a string which summarises what this thing is

    virtual QList<QAction *> actionsForMenu(QPointF scenePosition);
    // returns a list of QActions that should be inserted on the
    // right click menu.

    virtual void enableLayoutUpdates();
    // When a LakosEntity is first constructed, we don't update layouts,
    // edges, etc with each move so that we don't spend time on that while
    // laying out a whole scene. Once the scene is set up we do want to
    // carefully update these things as we go along. On construction this
    // disabled.

    virtual void showChildRedundantRelations(bool show);
    // call showRedundnatRelations on all children

    virtual void setHighlighted(bool highlighted);
    // Set whether or not the entity is highlighted

    [[nodiscard]] virtual std::string colorIdText() const;
    // returns what the colorId means for this node, used on
    // tooltips and information texts.

    [[nodiscard]] virtual lvtshr::DiagramType instanceType() const = 0;
    // returns a specific type of the instance, for loading and storing into the database.

    [[nodiscard]] int type() const override;
    // see QGraphicsItem documentation.

    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
    // see QGraphicsItem documentation.

    [[nodiscard]] QList<LakosEntity *> parentHierarchy() const;
    // a list of parents from this item.
    // the outermost is the one directly on the scene
    // the innermost the direct parent of this item
    // empty hierarchy means that this item is directly on the scene

    void expand(QtcUtil::CreateUndoAction CreateUndoAction,
                std::optional<QPointF> moveToPosition = std::nullopt,
                RelayoutBehavior behavior = RelayoutBehavior::e_DoNotRelayout);
    // Expands this node.

    void collapse(QtcUtil::CreateUndoAction CreateUndoAction,
                  std::optional<QPointF> moveToPosition = std::nullopt,
                  RelayoutBehavior behavior = RelayoutBehavior::e_DoNotRelayout);
    // Shrinks this node.

    [[nodiscard]] std::unordered_map<LakosEntity *, int> childrenLevels() const;

    void levelizationLayout(LevelizationLayoutType type,
                            int direction,
                            std::optional<QPointF> moveToPosition = std::nullopt);

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& thisObj);

    void setColor(const QColor& color);

    void setPluginManager(Codethink::lvtplg::PluginManager& pm);

    void startDrag(QPointF startPosition);
    void doDrag(QPointF movePosition);
    void endDrag(QPointF endPosition);

  Q_SIGNALS:
    Q_SIGNAL void navigateRequested();
    // We want to load this node as the main node

    Q_SIGNAL void loadChildren();
    // Request that the children of this entity are loaded

    Q_SIGNAL void loadClients(bool onlyLocal = false);
    // load entities that are clients for this entity.
    // If `onlyLocal` is true, only load clients from the same package.

    Q_SIGNAL void loadProviders(bool onlyLocal = false);
    // Load entities that are providers for this entity.
    // If `onlyLocal` is true, only load clients from the same package.

    Q_SIGNAL void unloadThis();
    Q_SIGNAL void unloadChildren();
    // Request that the children of this entity are not loaded

    Q_SIGNAL void neverLoadEdges();
    // Request that the edges for this entity are not loaded

    Q_SIGNAL void formFactorChanged();
    // Expanded / shrunk / rect changed.

    Q_SIGNAL void dragStarted();
    // A drag operation started. child classes must implement to handle complex behavior.

    Q_SIGNAL void dragFinished();
    // a drag operation finished, this item potentially is moved.

    Q_SIGNAL void moving();
    // The user is manually dragging this LakosEntity

    Q_SIGNAL void graphUpdate();
    // The user manually changed something and we should save the graph

    Q_SIGNAL void requestMultiSelectActivation(const QPoint& positionInScene);
    // The user clicked inside a node but wants to start a multi selection.

    Q_SIGNAL void undoGroupRequested(const QString& groupName);
    // start to merge undo actions here.

    Q_SIGNAL void undoCommandCreated(QUndoCommand *command);
    // A new undo command is created for his node.

    Q_SIGNAL void entityRenameRequest(const lvtshr::UniqueId& uid, const std::string& newName);

    Q_SIGNAL void requestRemoval();
    // asks the scene to safely remove this element.

    Q_SIGNAL void requestRelayout();
    // asks the scene to relayout the nodes internal to this entity.

    Q_SIGNAL void requestGraphRelayout();
    // asks the scene to relayout the entire graph
    // this can be the case when we expand or shrink the nodes covering up
    // other nodes.

    Q_SIGNAL void toggleSelection();
    // Toggle selection.

    Q_SIGNAL void createReportActionClicked(std::string const& title, std::string const& htmlContents);

    Q_SIGNAL void requestNewTab(const QSet<QString> qualifiedNames);

  protected:
    [[nodiscard]] bool layoutUpdatesEnabled() const;
    // See enableLayoutUpdates()

    void makeToolTip(const std::string& noColorStr);
    // changes the item tooltip. this should be a smaller version of the
    // information panel.

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) override;
    // see QGraphicsItem documentation.

    void mousePressEvent(QGraphicsSceneMouseEvent *ev) override;
    // see QGraphicsItem documentation.

    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) override;
    // see QGraphicsItem documentation

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;
    // see QGraphicsItem documentation.

    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) override;
    // see QGraphicsItem documentation.

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) override;
    // see QGraphicsItem documentation.

    void truncateTitle(EllipsisTextItem::Truncate v);

    void forceHideLevelNumbers();

  private:
    void layoutIgnoredItems();
    // special handling to position items that are ignored
    // from the original layout algorithm

    void layoutEdges(LakosEntity *child);
    // Layout the edges of the specified child.

    void layoutEdges(LakosEntity *child, EdgeCollection::PointFrom pointFrom, EdgeCollection::PointTo pointTo);
    // Layout the edges of the specified child.

    void setTopMargins(qreal topMargins);
    // adds the top margins as extra spacing.

    void ignoreItemOnLayout(QGraphicsItem *item);
    // adds the item to the list of items that will be ignored on
    // the recalculation of the rectangle.

    void updateChildrenLoadedInfo();
    // show / hide the information about missing children elements.
    // this can happen when we partially load the item.

    // for the *ItemChange methods:
    void reactChildRemoved(QGraphicsItem *child);
    void reactChildAdded(QGraphicsItem *child);

    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtc

#endif
