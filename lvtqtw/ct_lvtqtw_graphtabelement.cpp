// ct_lvtqtw_graphtabelement.h                                -*-C++-*-

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

#include <ct_lvtqtw_graphtabelement.h>

#include <ui_ct_lvtqtw_graphtabelement.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtprj_projectfile.h>

#include <ct_lvtmdl_historylistmodel.h>

#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtc_itool.h>
#include <ct_lvtqtc_tool_add_component.h>
#include <ct_lvtqtc_tool_add_logical_entity.h>
#include <ct_lvtqtc_tool_add_logical_relation.h>
#include <ct_lvtqtc_tool_add_package.h>
#include <ct_lvtqtc_tool_add_physical_dependency.h>
#include <ct_lvtqtc_tool_reparent_entity.h>
#include <ct_lvtqtc_tool_zoom.h>

#include <ct_lvtqtw_searchwidget.h>
#include <ct_lvtqtw_toolbox.h>

// Using QMap because it has the .keys() method that makes it easier to pass
// the key list to the comboboxes.
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QMap>
#include <QResizeEvent>
#include <QSpinBox>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QtGlobal>

#include <preferences.h>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtqtc;
using namespace Codethink::lvtshr;

namespace Codethink::lvtqtw {

// defines how the load algorithm will handle the packages
enum PackageView { DependencyTree, ReverseDependencyTree, DependencyGraph };

struct GraphTabElement::Private {
    const QMap<QString, lvtshr::ClassView> classViewMap = {
        {tr("Traverse by relation"), lvtshr::ClassView::TraverseByRelation},
        {tr("Traverse all paths"), lvtshr::ClassView::TraverseAllPaths},
        {tr("Class hierarchy graph"), lvtshr::ClassView::ClassHierarchyGraph},
        {tr("Class Hierarchy tree"), lvtshr::ClassView::ClassHierarchyTree},
    };

    const QMap<QString, lvtshr::ClassScope> scopeMap = {
        {tr("All"), lvtshr::ClassScope::All},
        {tr("Namespace Only"), lvtshr::ClassScope::NamespaceOnly},
        {tr("Package Only"), lvtshr::ClassScope::PackageOnly},
    };

    lvtmdl::HistoryListModel historyModel;

    // Elements that can't be in the ui file, because they are on a ToolBar.
    QToolBar *topBar = nullptr;
    QComboBox *classView = nullptr;
    QComboBox *scope = nullptr;
    QSpinBox *traversalLimit = nullptr;
    QSpinBox *relationLimit = nullptr;
    QSpinBox *zoom = nullptr;
    QToolButton *historyBack = nullptr;
    QToolButton *historyForward = nullptr;
    QAction *modeCad = nullptr;

    // Actions that will trigger a reload of the scene
    QAction *actRelayout = nullptr;

    // Actions that allow the user to Edit the elements
    lvtqtc::ITool *toolAddPhysicalDependency = nullptr;
    lvtqtc::ITool *toolAddIsALogicalRelation = nullptr;
    lvtqtc::ITool *toolAddUsesInTheImplementationRelation = nullptr;
    lvtqtc::ITool *toolAddUsesInTheInterfaceRelation = nullptr;
    lvtqtc::ITool *toolAddPackage = nullptr;
    lvtqtc::ITool *toolAddComponent = nullptr;
    lvtqtc::ITool *toolAddLogEntity = nullptr;
    lvtqtc::ITool *toolReparentEntity = nullptr;

    // Floating panel.
    SearchWidget *searchWidget = nullptr;

    std::vector<QAction *> classOnlyActions;
    std::vector<lvtqtc::ITool *> tools;

    GraphicsView *graphicsView = nullptr;
};

GraphTabElement::GraphTabElement(NodeStorage& nodeStorage, lvtprj::ProjectFile const& projectFile, QWidget *parent):
    QWidget(parent), ui(std::make_unique<Ui::GraphTabElement>()), d(std::make_unique<GraphTabElement::Private>())
{
    ui->setupUi(this);

    d->graphicsView = new GraphicsView(nodeStorage, projectFile, ui->splitter);
    d->graphicsView->setObjectName(QString::fromUtf8("graphicsView"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(ui->splitter->sizePolicy().hasHeightForWidth());
    sizePolicy.setHeightForWidth(d->graphicsView->sizePolicy().hasHeightForWidth());
    d->graphicsView->setSizePolicy(sizePolicy);
    ui->splitter->addWidget(d->graphicsView);

    d->topBar = new QToolBar(this);
    d->classView = new QComboBox(this);
    d->scope = new QComboBox(this);
    d->traversalLimit = new QSpinBox(this);
    d->relationLimit = new QSpinBox(this);
    d->zoom = new QSpinBox(this);
    d->historyBack = new QToolButton(this);
    d->historyForward = new QToolButton(this);

    // Floating search panel.
    d->searchWidget = new SearchWidget(this);
    d->searchWidget->setAttribute(Qt::WA_StyledBackground, true);
    d->searchWidget->setBackgroundRole(QPalette::Window);
    d->searchWidget->setAutoFillBackground(true);
    d->graphicsView->installEventFilter(d->searchWidget);

    d->historyBack->setText("<");
    d->historyForward->setText(">");

    d->topBar->addWidget(d->historyBack);
    d->topBar->addWidget(d->historyForward);
    d->classOnlyActions.push_back(d->topBar->addWidget(d->classView));
    d->classOnlyActions.push_back(d->topBar->addWidget(d->scope));
    d->classOnlyActions.push_back(d->topBar->addWidget(d->traversalLimit));
    d->classOnlyActions.push_back(d->topBar->addWidget(d->relationLimit));
    d->topBar->addWidget(d->zoom);
    d->topBar->layout()->setContentsMargins(0, 0, 0, 0);

    d->zoom->setMinimum(1);
    d->zoom->setMaximum(999);
    d->zoom->setPrefix(tr("Zoom: "));
    // End Toolbar Setup.

    ui->topLayout->addWidget(d->topBar);

    d->classView->addItems(d->classViewMap.keys());
    d->scope->addItems(d->scopeMap.keys());

    d->traversalLimit->setValue(Preferences::classLimit());
    d->traversalLimit->setPrefix(tr("Traversal Limit: "));

    d->relationLimit->setValue(Preferences::relationLimit());
    d->relationLimit->setPrefix(tr("Relation Limit: "));

    d->classView->setCurrentText(tr("Traverse by relation"));
    d->scope->setCurrentText(tr("All"));

    d->zoom->setValue(Preferences::zoomLevel());

    // sets up the scene.
    auto *scene = qobject_cast<lvtqtc::GraphicsScene *>(d->graphicsView->scene());
    scene->setClassView(d->classViewMap[d->classView->currentText()]);
    scene->setScope(d->scopeMap[d->scope->currentText()]);
    scene->setTraversalLimit(d->traversalLimit->value());
    scene->setRelationLimit(d->relationLimit->value());

    connect(d->classView, &QComboBox::currentTextChanged, this, [this, scene] {
        d->searchWidget->hide();
        scene->setClassView(d->classViewMap[d->classView->currentText()]);
    });

    connect(d->scope, &QComboBox::currentTextChanged, this, [this, scene] {
        d->searchWidget->hide();
        scene->setScope(d->scopeMap[d->scope->currentText()]);
    });

    connect(d->traversalLimit, qOverload<int>(&QSpinBox::valueChanged), this, [this, scene] {
        d->searchWidget->hide();
        scene->setTraversalLimit(d->traversalLimit->value());
    });

    connect(d->relationLimit, qOverload<int>(&QSpinBox::valueChanged), this, [this, scene] {
        d->searchWidget->hide();
        scene->setRelationLimit(d->relationLimit->value());
    });

    connect(d->graphicsView, &Codethink::lvtqtc::GraphicsView::zoomFactorChanged, d->zoom, &QSpinBox::setValue);

    connect(d->zoom,
            QOverload<int>::of(&QSpinBox::valueChanged),
            d->graphicsView,
            &Codethink::lvtqtc::GraphicsView::setZoomFactor);

    // We don't need to fix the search on the history model, as it will
    // load a different graph, that's already handled by setCurentDiagramFromHistory.
    connect(d->graphicsView, &lvtqtc::GraphicsView::requestNext, this, [this] {
        d->historyModel.next();
    });

    connect(d->graphicsView, &lvtqtc::GraphicsView::requestPrevious, this, [this] {
        d->historyModel.previous();
    });

    connect(d->historyBack, &QToolButton::clicked, this, [this] {
        d->historyModel.previous();
    });

    connect(d->historyForward, &QToolButton::clicked, this, [this] {
        d->historyModel.next();
    });
    connect(&d->historyModel,
            &lvtmdl::HistoryListModel::currentIndexChanged,
            this,
            &GraphTabElement::setCurrentDiagramFromHistory);

    connect(d->searchWidget, &SearchWidget::searchModeChanged, d->graphicsView, &lvtqtc::GraphicsView::setSearchMode);

    connect(d->searchWidget,
            &SearchWidget::searchStringChanged,
            d->graphicsView,
            &lvtqtc::GraphicsView::setSearchString);

    connect(d->searchWidget,
            &SearchWidget::requestNextElement,
            d->graphicsView,
            &lvtqtc::GraphicsView::highlightedNextSearchElement);

    connect(d->searchWidget,
            &SearchWidget::requestPreviousElement,
            d->graphicsView,
            &lvtqtc::GraphicsView::highlightedPreviousSearchElement);

    connect(d->graphicsView,
            &lvtqtc::GraphicsView::searchTotal,
            d->searchWidget,
            &SearchWidget::setNumberOfMatchedItems);

    connect(d->graphicsView,
            &lvtqtc::GraphicsView::currentSearchItemHighlighted,
            d->searchWidget,
            &SearchWidget::setCurrentItem);

    setClassViewMode();

    d->graphicsView->toggleLegend(Preferences::showLegend());
    d->graphicsView->toggleMinimap(Preferences::showMinimap());

    d->searchWidget->setVisible(false);

    setupToolBar(nodeStorage);
}

GraphTabElement::~GraphTabElement() = default;

void GraphTabElement::setupToolBar(NodeStorage& nodeStorage)
{
    auto *scene = qobject_cast<lvtqtc::GraphicsScene *>(d->graphicsView->scene());

    lvtqtc::ITool *zoomTool = new lvtqtc::ToolZoom(d->graphicsView);
    d->toolAddPhysicalDependency = new lvtqtc::ToolAddPhysicalDependency(d->graphicsView, nodeStorage);
    d->toolAddIsALogicalRelation = new lvtqtc::ToolAddIsARelation(d->graphicsView, nodeStorage);
    d->toolAddUsesInTheImplementationRelation =
        new lvtqtc::ToolAddUsesInTheImplementation(d->graphicsView, nodeStorage);
    d->toolAddUsesInTheInterfaceRelation = new lvtqtc::ToolAddUsesInTheInterface(d->graphicsView, nodeStorage);
    d->toolAddPackage = new lvtqtc::ToolAddPackage(d->graphicsView, nodeStorage);
    d->toolAddComponent = new lvtqtc::ToolAddComponent(d->graphicsView, nodeStorage);
    d->toolAddLogEntity = new lvtqtc::ToolAddLogicalEntity(d->graphicsView, nodeStorage);
    d->toolReparentEntity = new lvtqtc::ToolReparentEntity(d->graphicsView, nodeStorage);

    d->tools = {
        /* 0 */ zoomTool,
        /* 1 */ d->toolAddPhysicalDependency,
        /* 2 */ d->toolAddPackage,
        /* 3 */ d->toolAddComponent,
        /* 4 */ d->toolAddLogEntity,
        /* 5 */ d->toolAddIsALogicalRelation,
        /* 6 */ d->toolAddUsesInTheImplementationRelation,
        /* 7 */ d->toolAddUsesInTheInterfaceRelation,
        /* 8 */ d->toolReparentEntity,
    };
    for (auto *tool : d->tools) {
        connect(tool, &lvtqtc::ITool::sendMessage, this, &GraphTabElement::sendMessage);
    }

    d->actRelayout = new QAction();
    d->actRelayout->setToolTip(tr("Recalculates the layout of the current view elements"));
    d->actRelayout->setText(tr("Refresh Layout"));
    d->actRelayout->setIcon(IconHelpers::iconFrom(":/icons/refresh"));
    connect(d->actRelayout, &QAction::triggered, scene, &lvtqtc::GraphicsScene::relayout);

    auto *dumpVisibleRectAction = new QAction();
    dumpVisibleRectAction->setToolTip(tr("Output to stdout the items on the visible rectangle"));
    dumpVisibleRectAction->setText(tr("Output visible rect"));
    dumpVisibleRectAction->setIcon(IconHelpers::iconFrom(":/icons/debug"));
    connect(dumpVisibleRectAction, &QAction::triggered, d->graphicsView, &lvtqtc::GraphicsView::debugVisibleScreen);

    auto *dumpSceneAction = new QAction();
    dumpSceneAction->setToolTip(tr("Output to stdout the whole scene"));
    dumpSceneAction->setText(tr("Output scene"));
    dumpSceneAction->setIcon(IconHelpers::iconFrom(":/icons/fatal"));
    connect(dumpSceneAction, &QAction::triggered, scene, [scene] {
        scene->dumpScene();
    });

    auto *minimapAction = new QAction();
    minimapAction->setToolTip(tr("Show Minimap"));
    minimapAction->setText(tr("Show Minimap"));
    minimapAction->setIcon(IconHelpers::iconFrom(":/icons/map"));
    minimapAction->setCheckable(true);
    minimapAction->setChecked(Preferences::showMinimap());
    connect(minimapAction, &QAction::triggered, d->graphicsView, &lvtqtc::GraphicsView::toggleMinimap);

    auto *legendAction = new QAction();
    legendAction->setToolTip(tr("Show Information Panel"));
    legendAction->setText(tr("Information Panel"));
    legendAction->setIcon(IconHelpers::iconFrom(":/icons/help"));
    legendAction->setCheckable(true);
    legendAction->setChecked(Preferences::showMinimap());
    connect(legendAction, &QAction::triggered, d->graphicsView, &lvtqtc::GraphicsView::toggleLegend);

    auto *fitInViewAction = new QAction();
    fitInViewAction->setToolTip(tr("Fits the entire graph on the view."));
    fitInViewAction->setText(tr("Fit in View"));
    fitInViewAction->setIcon(IconHelpers::iconFrom(":/icons/expand"));
    connect(fitInViewAction, &QAction::triggered, this, [this] {
        d->graphicsView->fitAllInView();
    });

    auto *resetZoomAction = new QAction();
    resetZoomAction->setToolTip(tr("Reset the zoom level to 100%"));
    resetZoomAction->setText(tr("Reset Zoom"));
    resetZoomAction->setIcon(IconHelpers::iconFrom(":/icons/remove_zoom"));
    connect(resetZoomAction, &QAction::triggered, this, [this] {
        d->graphicsView->setZoomFactor(100);
    });

    d->modeCad = new QAction();
    d->modeCad->setToolTip(tr("Click to switch to edit mode"));
    d->modeCad->setText(tr("Visualize"));
    d->modeCad->setCheckable(true);
    d->modeCad->setChecked(true);
    connect(d->modeCad, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            visualizationModeTriggered();
        } else {
            editModeTriggered();
        }
    });
    ui->toolBox->createGroup(tr("Diagram Mode"));
    auto *toggleModeButton = ui->toolBox->createToolButton(tr("Toolbox Mode"), d->modeCad);
    toggleModeButton->setStyleSheet(
        "QToolButton { border: 0px;}"
        "QToolButton:hover { background-color: #DDD; font-weight: bold; }");

    // Manipulation:
    const QString manipulationId = tr("Manipulation");
    ui->toolBox->createGroup(manipulationId);
    ui->toolBox->createToolButton(manipulationId, d->toolAddPackage);
    ui->toolBox->createToolButton(manipulationId, d->toolAddComponent);
    ui->toolBox->createToolButton(manipulationId, d->toolAddLogEntity);
    ui->toolBox->createToolButton(manipulationId, d->toolAddPhysicalDependency);
    ui->toolBox->createToolButton(manipulationId, d->toolAddIsALogicalRelation);
    ui->toolBox->createToolButton(manipulationId, d->toolAddUsesInTheImplementationRelation);
    ui->toolBox->createToolButton(manipulationId, d->toolAddUsesInTheInterfaceRelation);
    ui->toolBox->createToolButton(manipulationId, d->toolReparentEntity);

    // Package View Mode tools:
    const QString packageId = tr("Packages");
    ui->toolBox->createGroup(packageId);

    // Common Tools:
    const QString visualizationId = tr("Visualization");
    ui->toolBox->createGroup(visualizationId);
    ui->toolBox->createToolButton(visualizationId, d->actRelayout);
    ui->toolBox->createToolButton(visualizationId, zoomTool);
    ui->toolBox->createToolButton(visualizationId, fitInViewAction);
    ui->toolBox->createToolButton(visualizationId, resetZoomAction);
    ui->toolBox->createToolButton(visualizationId, minimapAction);
    ui->toolBox->createToolButton(visualizationId, legendAction);

    // Debug
    const QString debugId = tr("Debug");
    ui->toolBox->createGroup(debugId);
    ui->toolBox->createToolButton(debugId, dumpVisibleRectAction);
    ui->toolBox->createToolButton(debugId, dumpSceneAction);

    ui->splitter->setSizes({100, 999});
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    visualizationModeTriggered();

    if (!Preferences::enableSceneContextMenu()) {
        ui->toolBox->hideElements("Debug");
    }
}

void GraphTabElement::visualizationModeTriggered()
{
    // Actions that will trigger a reload of the scene
    d->actRelayout->setEnabled(true);
    d->classView->setEnabled(true);
    d->scope->setEnabled(true);
    d->traversalLimit->setEnabled(true);
    d->relationLimit->setEnabled(true);
    d->historyBack->setEnabled(true);
    d->historyForward->setEnabled(true);

    // Actions that allow the user to Edit the elements
    d->toolAddPhysicalDependency->setEnabled(false);
    d->toolAddIsALogicalRelation->setEnabled(false);
    d->toolAddPackage->setEnabled(false);
    d->toolAddComponent->setEnabled(false);
    d->toolAddLogEntity->setEnabled(false);

    d->graphicsView->visualizationModeTriggered();
    d->modeCad->setIcon(QIcon(":/icons/toggle-vis-edit-state-vis"));
    d->modeCad->setToolTip(tr("Click to switch to edit mode"));
    d->modeCad->setText(tr("View"));
    ui->toolBox->hideElements("Manipulation");
    ui->toolBox->showElements("Class");
    ui->toolBox->showElements("Packages");
}

void GraphTabElement::editModeTriggered()
{
    d->actRelayout->setEnabled(false);
    d->classView->setEnabled(false);
    d->scope->setEnabled(false);
    d->traversalLimit->setEnabled(false);
    d->relationLimit->setEnabled(false);
    d->historyBack->setEnabled(false);
    d->historyForward->setEnabled(false);

    // Actions that allow the user to Edit the elements
    d->toolAddPhysicalDependency->setEnabled(true);
    d->toolAddIsALogicalRelation->setEnabled(true);
    d->toolAddPackage->setEnabled(true);
    d->toolAddComponent->setEnabled(true);
    d->toolAddLogEntity->setEnabled(true);

    d->graphicsView->editModeTriggered();
    d->modeCad->setIcon(QIcon(":/icons/toggle-vis-edit-state-edit"));
    d->modeCad->setToolTip(tr("Click to switch to visualization mode"));
    d->modeCad->setText(tr("Edit"));
    ui->toolBox->showElements("Manipulation");
    ui->toolBox->hideElements("Class");
    ui->toolBox->hideElements("Packages");
}

lvtqtc::GraphicsView *GraphTabElement::graphicsView() const
{
    return d->graphicsView;
}

void GraphTabElement::toggleFilterVisibility()
{
    d->searchWidget->setVisible(!d->searchWidget->isVisible());
}

void GraphTabElement::setPackageViewMode()
{
    for (QAction *action : d->classOnlyActions) {
        action->setVisible(false);
    }
    ui->toolBox->showElements("Packages");
    ui->toolBox->hideElements("Class");
}

void GraphTabElement::setClassViewMode()
{
    for (QAction *action : d->classOnlyActions) {
        action->setVisible(true);
    }
    ui->toolBox->hideElements("Packages");
    ui->toolBox->showElements("Class");
}

bool GraphTabElement::setCurrentGraph(const QString& fullyQualifiedName,
                                      HistoryType historyType,
                                      DiagramType diagramType)
{
    d->searchWidget->hide();
    bool success = [&]() {
        switch (diagramType) {
        case DiagramType::ClassType:
            return d->graphicsView->updateClassGraph(fullyQualifiedName);
        case DiagramType::ComponentType:
            return d->graphicsView->updateComponentGraph(fullyQualifiedName);
        case DiagramType::PackageType:
            return d->graphicsView->updatePackageGraph(fullyQualifiedName);
        case DiagramType::RepositoryType:
            return d->graphicsView->updateGraph(fullyQualifiedName, lvtshr::DiagramType::RepositoryType);
        case DiagramType::FreeFunctionType:
            return d->graphicsView->updateGraph(fullyQualifiedName, lvtshr::DiagramType::FreeFunctionType);
        case DiagramType::NoneType:
            break;
        }
        return false;
    }();
    if (!success) {
        return false;
    }

    if (historyType == GraphTabElement::HistoryType::History) {
        d->historyModel.append({fullyQualifiedName, diagramType});
    }

    if (diagramType == DiagramType::ClassType) {
        setClassViewMode();
    }
    if (diagramType == DiagramType::PackageType || diagramType == DiagramType::ComponentType) {
        setPackageViewMode();
    }

    return true;
}

void GraphTabElement::setCurrentDiagramFromHistory(int idx)
{
    d->searchWidget->hide();

    std::pair<QString, lvtshr::DiagramType> info = d->historyModel.at(idx);
    if (info.first.count() == 0) {
        return;
    }
    switch (info.second) {
    case lvtshr::DiagramType::NoneType:
        qWarning() << "Database corrupted";
        return;
    case lvtshr::DiagramType::ClassType:
        [[fallthrough]];
    case lvtshr::DiagramType::FreeFunctionType:
        [[fallthrough]];
    case lvtshr::DiagramType::PackageType:
        [[fallthrough]];
    case lvtshr::DiagramType::RepositoryType:
        [[fallthrough]];
    case lvtshr::DiagramType::ComponentType:
        setCurrentGraph(info.first, NoHistory, info.second);
        break;
    }
    Q_EMIT historyUpdate(info.first, info.second);
}

void GraphTabElement::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    constexpr int horizontalSpacing = 100;
    const int x = width() - d->searchWidget->width() - horizontalSpacing;
    const int y = d->graphicsView->y();

    d->searchWidget->adjustSize();
    d->searchWidget->setGeometry(x, y, d->searchWidget->width(), d->searchWidget->height());
}

std::vector<lvtqtc::ITool *> GraphTabElement::tools() const
{
    return d->tools;
}

void GraphTabElement::setPluginManager(Codethink::lvtplg::PluginManager& pm)
{
    d->graphicsView->setPluginManager(pm);
}

} // namespace Codethink::lvtqtw
