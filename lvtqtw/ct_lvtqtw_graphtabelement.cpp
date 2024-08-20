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
#include <ct_lvtqtw_bulkedit.h>

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
    lvtprj::ProjectFile& projectFile;

    lvtmdl::HistoryListModel historyModel;

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

    Private(lvtprj::ProjectFile& prj): projectFile(prj)
    {
    }
};

GraphTabElement::GraphTabElement(NodeStorage& nodeStorage, lvtprj::ProjectFile& projectFile, QWidget *parent):
    QWidget(parent),
    ui(std::make_unique<Ui::GraphTabElement>()),
    d(std::make_unique<GraphTabElement::Private>(projectFile))
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

    // Floating search panel.
    d->searchWidget = new SearchWidget(this);
    d->searchWidget->setAttribute(Qt::WA_StyledBackground, true);
    d->searchWidget->setBackgroundRole(QPalette::Window);
    d->searchWidget->setAutoFillBackground(true);
    d->graphicsView->installEventFilter(d->searchWidget);
    // End Toolbar Setup.

    ui->zoomToolBox->setValue(Preferences::zoomLevel());

    // sets up the scene.
    connect(d->graphicsView, &Codethink::lvtqtc::GraphicsView::zoomFactorChanged, ui->zoomToolBox, &QSpinBox::setValue);

    connect(ui->zoomToolBox,
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

    connect(ui->backHistory, &QToolButton::clicked, this, [this] {
        d->historyModel.previous();
    });

    connect(ui->forwardHistory, &QToolButton::clicked, this, [this] {
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
    connect(d->actRelayout, &QAction::triggered, scene, &lvtqtc::GraphicsScene::reLayout);

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
        qDebug() << scene->toJson();
    });

    auto *bulkEditAction = new QAction();
    bulkEditAction->setToolTip(tr("Show Bulk Edit Dialog"));
    bulkEditAction->setText(tr("Bulk Edit"));
    bulkEditAction->setCheckable(false);
    bulkEditAction->setIcon(IconHelpers::iconFrom(":/icon/fatal"));
    connect(bulkEditAction, &QAction::triggered, this, [this, scene] {
        BulkEdit be(this);
        QJsonDocument doc;
        connect(&be, &BulkEdit::sendBulkJson, this, [scene](const QString& jsonDoc) {
            scene->loadJsonWithDocumentChanges(jsonDoc);
        });
        be.exec();
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
    legendAction->setChecked(Preferences::showLegend());
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

    // Common Tools:
    const QString visualizationId = tr("Visualization");
    ui->toolBox->createGroup(visualizationId);
    ui->toolBox->createToolButton(visualizationId, d->actRelayout);
    ui->toolBox->createToolButton(visualizationId, zoomTool);
    ui->toolBox->createToolButton(visualizationId, fitInViewAction);
    ui->toolBox->createToolButton(visualizationId, resetZoomAction);
    ui->toolBox->createToolButton(visualizationId, minimapAction);
    ui->toolBox->createToolButton(visualizationId, legendAction);
    ui->toolBox->createToolButton(visualizationId, bulkEditAction);

    // Debug
    const QString debugId = tr("Debug");
    ui->toolBox->createGroup(debugId);
    ui->toolBox->createToolButton(debugId, dumpVisibleRectAction);
    ui->toolBox->createToolButton(debugId, dumpSceneAction);

    ui->splitter->setSizes({100, 999});
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    if (!Preferences::enableSceneContextMenu()) {
        ui->toolBox->hideElements("Debug");
    }
}

lvtqtc::GraphicsView *GraphTabElement::graphicsView() const
{
    return d->graphicsView;
}

void GraphTabElement::toggleFilterVisibility()
{
    d->searchWidget->setVisible(!d->searchWidget->isVisible());
}

void GraphTabElement::setCurrentDiagramFromHistory(int idx)
{
    Q_EMIT historyUpdate(d->historyModel.at(idx));
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

QJsonDocument GraphTabElement::toJsonDocument(const QString& tabTitle) const
{
    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(graphicsView()->scene());
    auto jsonObj = scene->toJson();

    QJsonObject mainObj{{"scene", jsonObj},
                        {"tabname", tabTitle},
                        {"id", tabTitle},
                        {"zoom_level", graphicsView()->zoomFactor()}};

    return QJsonDocument(mainObj);
}

void GraphTabElement::saveBookmark(const QString& title, lvtprj::ProjectFile::BookmarkType type)
{
    const cpp::result<void, lvtprj::ProjectFileError> ret = d->projectFile.saveBookmark(toJsonDocument(title), type);

    if (ret.has_error()) {
        Q_EMIT sendMessage(tr("Error saving bookmark."), KMessageWidget::MessageType::Error);
    }
}

void GraphTabElement::loadBookmark(const QJsonDocument& doc, lvtshr::HistoryType historyType)
{
    QJsonObject obj = doc.object();
    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(graphicsView()->scene());

    scene->fromJson(obj["scene"].toObject());

    graphicsView()->setZoomFactor(obj["zoom_level"].toInt());
    if (historyType == lvtshr::HistoryType::History) {
        d->historyModel.append(doc["id"].toString());
    }
}

} // namespace Codethink::lvtqtw

#include "moc_ct_lvtqtw_graphtabelement.cpp"
