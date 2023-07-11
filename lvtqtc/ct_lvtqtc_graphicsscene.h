// ct_lvtqtc_graphicsscene.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_GRAPHICSSCENE
#define INCLUDED_LVTQTC_GRAPHICSSCENE

#include <ct_lvtshr_uniqueid.h>
#include <lvtqtc_export.h>

#include <ct_lvtqtc_packagedependency.h>

#include <ct_lvtldr_igraphloader.h>
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtprj_projectfile.h>

#include <ct_lvtshr_graphenums.h>

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QMenu>
#include <QTimer>

#include <oup/observable_unique_ptr.hpp>

#include <memory>
#include <tuple>

class QKeyEvent;

namespace Codethink::lvtclr {
class ColorManagement;
}

namespace Codethink::lvtldr {
class NodeStorage;
class LakosianNode;
} // namespace Codethink::lvtldr

namespace Codethink::lvtmdl {
class CircularRelationshipsModel;
}

namespace Codethink::lvtqtc {
class LakosEntity;
class LakosRelation;

class LVTQTC_EXPORT GraphicsScene : public QGraphicsScene,
                                    public Codethink::lvtldr::IGraphLoader,
                                    public oup::enable_observer_from_this_unique<GraphicsScene>
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT

  public:
    struct Private;

    // The layers are "sheets of translucent plastic" where the objects
    // are drawn. lower values means that it's on the bottom, and high
    // values are on the top. This controls the drawing order of things.

    enum LoadFlags {
        Idle, // Waiting for work
        Running, // We are still running. wait a bit.
        Success, // Everything as it should be.
        NotReady, // We are not ready yet, we shouldn't be trying to do this.
        LoadFromCacheDbError, // We hit an error while loading the cache database. Missing database cache is not an
                              // error, but expected.
        LoadFromDbError, // We hit an error loading from the llvm database, Missing database cache is an error.
        LayoutNodesError, // We can't lay out the nodes.
        LayoutEdgesError, // We can't lay out the edges.
        LayoutGvzError, // We don't know how to lay out this graph with gvz
        LayoutGvzDimensionsError,
        LayoutGvzPositionError,
        LayoutGvzLPosError,
        LayoutEnhancedDotError, // We don't know how to lay out this graph with dot
        SaveGraphError, // We can't save, check the logs.
        UserAborted // User triggered an action to abort the load mid-loading.
    };

    enum class UnloadDepth : unsigned short { Entity, Children };

    enum class CreateAction : unsigned short {
        LoadGraphAction, // The entity is created at the `loadFromDb` step, populating the whole canvas.
        UserAction // The entity is created by the User, when the canvas is already populated.
    };

    // The possible errors that can occur during the load phase.

    // CREATORS
    GraphicsScene(Codethink::lvtldr::NodeStorage& nodeStorage,
                  lvtprj::ProjectFile const& projectFile,
                  QObject *parent = nullptr);
    // Constructor

    ~GraphicsScene() noexcept override;
    // Destructor

    [[nodiscard]] Codethink::lvtmdl::CircularRelationshipsModel *circularRelationshipsModel() const;
    // returns the CircleRelationshipModel for this scene.

    void updateBoundingRect();
    // recalculates and updates the bounding rectangle based
    // on the children bounding rect.
    // must be called when we add a new element on the screen via
    // a tool, or after we load entities from the database.
    // this call is expensive, it should not be called whenever
    // we add something on the view, just after all elements
    // are finished loading.

    void setClassView(lvtshr::ClassView classView);
    // set's the class view

    void setScope(lvtshr::ClassScope classScope);
    // set's the class scope

    void setTraversalLimit(int limit);
    // set the traversal limit.

    void setRelationLimit(int limit);
    // set the relation limit.

    void setShowExternalDepEdges(bool showExternalDepEdges);
    // control if we show cross-subgraph edges between dependencies in the
    // package graph

    void setColorManagement(const std::shared_ptr<lvtclr::ColorManagement>& colorManagement);

    void setStrictMode(bool strictMode);
    // Sets whether we should attempt to continue or fail immediately on errors
    // default is off.

    bool hasMainNodeSelected() const;

    void setMainNode(const QString& fullyQualifiedName, lvtshr::DiagramType type);
    // sets the fully qualified name and type used for the main node in the
    // diagram

    [[nodiscard]] lvtshr::DiagramType diagramType() const;
    [[nodiscard]] QString qualifiedName() const;

    // This class defines what we need to implement on classes that load graphs visually
    void clearGraph() override;

    LakosEntity *addUdtVertex(lvtldr::LakosianNode *node,
                              bool selected = false,
                              LakosEntity *parent = nullptr,
                              lvtshr::LoaderInfo info = {}) override;

    LakosEntity *addRepositoryVertex(lvtldr::LakosianNode *node,
                                     bool selected = false,
                                     LakosEntity *parent = nullptr,
                                     lvtshr::LoaderInfo info = {}) override;

    LakosEntity *addPkgVertex(lvtldr::LakosianNode *node,
                              bool selected = false,
                              LakosEntity *parent = nullptr,
                              lvtshr::LoaderInfo info = {}) override;

    LakosEntity *addCompVertex(lvtldr::LakosianNode *node,
                               bool selected = false,
                               LakosEntity *parent = nullptr,
                               lvtshr::LoaderInfo info = {}) override;

    LakosRelation *addIsARelation(LakosEntity *source, LakosEntity *target) override;
    LakosRelation *addUsesInTheInterfaceRelation(LakosEntity *source, LakosEntity *target) override;
    LakosRelation *addUsesInTheImplementationRelation(LakosEntity *source, LakosEntity *target) override;
    LakosRelation *addPackageDependencyRelation(LakosEntity *source, LakosEntity *target) override;
    LakosRelation *addAllowedPackageDependencyRelation(LakosEntity *source, LakosEntity *target) override;

    void connectEntitySignals(LakosEntity *entity);

    [[nodiscard]] std::vector<LakosEntity *>& allEntities() const;

    Q_SIGNAL void packageNavigateRequested(const QString& fullyQualifiedName);
    Q_SIGNAL void classNavigateRequested(const QString& fullyQualifiedName);
    Q_SIGNAL void componentNavigateRequested(const QString& fullyQualifiedName);
    Q_SIGNAL void selectedEntityChanged(Codethink::lvtqtc::LakosEntity *entity);

    void updateGraph();
    // Redraw the graph

    void relayout();

    void runLayoutAlgorithm();
    // ignores the layout saved on the cache, and relayouts the items on screen.

    void setEntityPos(const lvtshr::UniqueId& uid, QPointF pos) const;

    void setBlockNodeResizeOnHover(bool block);
    // forbids / enables node resize on hover.

    [[nodiscard]] bool blockNodeResizeOnHover() const;
    // return the value of the blockNodeResizeOnHover flag.

    void toggleIdentifyCycles();

    // Those calls handle the MachineState that controls the flow of information
    // for loading the database.
    // First we try to load from the Cache database, the cache database has a
    // different graph data structure than the other database, that already
    // holds information on positions - so we don't need to call layoutNodes()
    // and layoutEdges().
    //
    // TODO: Merge the data structures of both graphs.
    Q_SIGNAL void graphLoadStarted();
    Q_SIGNAL void refuseToLoad(const QString& fullyQualifiedName);

    enum class CacheDbLoadStatus { CacheFound, CacheNotFound, Error };

    enum class CodeDbLoadStatus { Success, Error };

    CacheDbLoadStatus layoutFromCacheDatabase();
    CodeDbLoadStatus requestDataFromDatabase();
    void layoutVertices();
    void finalizeLayout();
    // Finishes the layout of the entire graph.

    void reLayout();
    // runs the layout algorithm again, on the current loaded graph.

    void pannelCollapse();
    void fixRelations();
    void edgesContainersLayout();
    void transitiveReduction();
    void layoutDone();
    [[nodiscard]] LoadFlags loadFlags() const;
    [[nodiscard]] QString fetchErrorMessage() const;
    Q_SIGNAL void errorMessage(const QString& error);
    Q_SIGNAL void graphLoadFinished();

    enum class GraphLoadProgress : int {
        Start = 0,
        CheckCache,
        CdbLoad,
        QtEventLoop, // the event loop takes considerable time on the first loop
                     // after all of the vertices are added
        VertexLayout,
        PannelCollapse,
        FixRelations,
        EdgesContainersLayout,
        TransitiveReduction,
        Done
    };
    Q_SIGNAL void graphLoadProgressUpdate(Codethink::lvtqtc::GraphicsScene::GraphLoadProgress progress);

    void dumpScene(QGraphicsItem *item = nullptr, int indent = 0);
    void dumpScene(const QList<QGraphicsItem *>& items);

    [[nodiscard]] LakosEntity *mainEntity() const;
    void setMainEntity(LakosEntity *entity);

    void populateMenu(QMenu& menu, QMenu *debugMenu);

    void fixTransitiveEdgeVisibility();
    // some changes on the nodes can trigger a visibility change
    // on the edges. it's important that we fix the transitive
    // edge visibility after they are possibily made visible.
    // this is an inexpensive operation, there's no painting
    // done until we finish running this.

    Q_SIGNAL void requestEnableWindow();
    Q_SIGNAL void requestDisableWindow();

    Q_SIGNAL void createReportActionClicked(std::string const& title, std::string const& htmlContents);

    /* Debug Methods, methods to help us visualize contextual information
     * of elements */
    void toggleEdgeBoundingRects();
    void toggleEdgeShapes();
    void toggleEdgeTextualInformation();
    void toggleEdgeIntersectionPaths();
    void toggleEdgeOriginalLine();

    static LakosEntity *outermostParent(LakosEntity *a, LakosEntity *b);
    // returns the outermost parent of two different lakos entities, or nullptr

    [[nodiscard]] LakosEntity *entityById(const std::string& uniqueId) const;
    // Returns an entity by it's internal ID.

    [[nodiscard]] LakosEntity *entityByQualifiedName(const std::string& qualName) const;
    // returns an entity by it's qualified name.

    void loadEntityByQualifiedName(const QString& qualifiedName, const QPointF& pos);
    // A Qualified name is just dropped onto the scene. We need to build
    // the entities for it.

    [[nodiscard]] LakosRelation *addRelation(LakosRelation *relation, bool isVisible = true);
    // Adds a relation, called internally by all of the `add*Relation` methods.
    // should not be used outside of the implementation of this class, but it cannot be private
    // because we are acessing this from the unammed namespace.

    LakosEntity *findLakosEntityFromUid(lvtshr::UniqueId uid) const;

    void visualizationModeTriggered();
    void editModeTriggered();
    bool isEditMode();
    void collapseSecondaryEntities();

    void loadEntity(lvtshr::UniqueId uuid, UnloadDepth depth);
    void unloadEntity(lvtshr::UniqueId uuid, UnloadDepth depth);
    // public request to unload one or many elements.
    // the uuid is the element where the action will happen
    // the depth is "Element" or "Children", meaning unload itself, or only
    // the children.
    // Removing the children is a recursive operation.

    void removeEdge(LakosRelation *relation);

    lvtprj::ProjectFile const& projectFile() const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& doc);

    void setPluginManager(Codethink::lvtplg::PluginManager& pm);

  private:
    void unloadEntity(LakosEntity *entity);

    void finalizeEntityPartialLoad(LakosEntity *entity);
    // Finishes the layout of partial graphs.
    // partial graphs are the ones requested by a right click on the elements on the canvas.

    lvtldr::NodeLoadFlags loadFlagsFor(lvtldr::LakosianNode *node) const override;

    bool isReady();
    // checks if we can start drawing the graph.

    void setLoadFlags(LoadFlags flags);

    void fixRelationsParentRelationship();
    // HACK. Goes through the list of edges and fixes the parent relationship
    // this is needed because the PackageLoader loads some component children before loading
    // the component parent, and the edge needs the parent to position itself correctly.

    void searchTransitiveRelations();
    // ask the thread to look for transitive relations.

    void transitiveRelationSearchFinished();
    // the thread to look for transitive relations just finished.

    void toggleTransitiveRelationVisibility(bool show);
    // shows / hides the transitive edges.

    void addEdgeBetween(LakosEntity *fromEntity, LakosEntity *toEntity, lvtshr::LakosRelationType type);

    Q_SLOT void highlightCyclesOnCicleModelChanged(const QModelIndex& _, const QModelIndex& index);
    Q_SLOT void resetHighlightedCycles() const;

    // DATA TYPES
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtc

#endif
