// ct_lvtqtw_tabwidget.cpp                                           -*-C++-*-

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

#include <ct_lvtprj_projectfile.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_undo_manager.h>

#include <ct_lvtqtw_graphtabelement.h>
#include <ct_lvtqtw_tabwidget.h>

#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtmdl_modelhelpers.h>

#include <ct_lvtqtc_inputdialog.h>

#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QString>
#include <QTabBar>
#include <QToolButton>

#include <unordered_map>
#include <utility>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtqtc;
using namespace Codethink::lvtmdl;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtprj;

namespace {
DiagramType nodeTypeToDiagramType(NodeType::Enum type)
{
    switch (type) {
    case NodeType::e_Class:
        return DiagramType::ClassType;
    case NodeType::e_Namespace:
        break;
    case NodeType::e_Package:
        return DiagramType::PackageType;
    case NodeType::e_Repository:
        return DiagramType::RepositoryType;
    case NodeType::e_Component:
        return DiagramType::ComponentType;
    case NodeType::e_Invalid:
        break;
    }
    return DiagramType::NoneType;
}
} // namespace

namespace Codethink::lvtqtw {

struct TabWidget::Private {
    QToolButton *addGraphBtn = nullptr;
    std::shared_ptr<lvtclr::ColorManagement> colorManagement;
    QString cdbPath;
    NodeStorage& nodeStorage;
    lvtprj::ProjectFile& projectFile;
    UndoManager *undoManager = nullptr;
    std::unordered_map<const QUndoCommand *, GraphTabElement *> commandToTab;
    lvtplg::PluginManager *pluginManager = nullptr;

    explicit Private(lvtldr::NodeStorage& nodeStorage,
                     lvtprj::ProjectFile& projectFile,
                     std::shared_ptr<lvtclr::ColorManagement> colorManagement,
                     lvtplg::PluginManager *pluginManager = nullptr):
        colorManagement(std::move(colorManagement)),
        nodeStorage(nodeStorage),
        projectFile(projectFile),
        pluginManager(pluginManager)
    {
    }
};

// --------------------------------------------
// class TabWidget
// --------------------------------------------

TabWidget::TabWidget(NodeStorage& nodeStorage,
                     lvtprj::ProjectFile& projectFile,
                     std::shared_ptr<lvtclr::ColorManagement> colorManagement,
                     lvtplg::PluginManager *pluginManager,
                     QWidget *parent):
    QTabWidget(parent),
    d(std::make_unique<TabWidget::Private>(nodeStorage, projectFile, std::move(colorManagement), pluginManager))
{
    d->addGraphBtn = new QToolButton();
    d->addGraphBtn->setText("+");

    // TODO: Icons
    setCornerWidget(d->addGraphBtn, Qt::TopLeftCorner);
    connect(d->addGraphBtn, &QToolButton::clicked, this, [this]() {
        this->openNewGraphTab();
    });
    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::closeTab);
    setTabsClosable(true);

    setDocumentMode(true);
    openNewGraphTab();

    tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar(), &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        const auto idx = tabBar()->tabAt(pos);
        if (idx == -1) {
            return;
        }

        QMenu menu;
        if (tabIcon(idx).isNull()) {
            auto *action = menu.addAction(tr("Bookmark this tab"));
            connect(action, &QAction::triggered, this, [this, idx] {
                InputDialog dlg;
                dlg.addTextField("name", tr("Bookmark Name:"));
                dlg.finish();
                auto res = dlg.exec();
                if (res == QDialog::DialogCode::Rejected) {
                    return;
                }

                const auto text = std::any_cast<QString>(dlg.fieldValue("name"));
                saveBookmark(text, idx, Codethink::lvtprj::ProjectFile::Bookmark);
            });
        } else {
            auto *action = menu.addAction(tr("Remove bookmark"));
            connect(action, &QAction::triggered, this, [this, idx] {
                setTabIcon(idx, QIcon());

                // QWidgets sometimes adds a `&` to denote mnemonic,
                // remove it.
                auto text = tabText(idx);
                text.remove(QLatin1Char('&'));
                d->projectFile.removeBookmark(text);
            });
        }
        menu.exec(tabBar()->mapToGlobal(pos));
    });
}

TabWidget::~TabWidget() noexcept = default;

void TabWidget::closeTab(int idx)
{
    QWidget *widgetAt = widget(idx);
    removeTab(idx);

    widgetAt->deleteLater();

    if (count() == 0) {
        openNewGraphTab();
    }
}

void TabWidget::setCurrentTabText(const QString& fullyQualifiedName, lvtshr::DiagramType type)
{
    setTabText(currentIndex(), fullyQualifiedName);
    Q_EMIT currentTabTextChanged(fullyQualifiedName, type);
}

GraphTabElement *TabWidget::createTabElement()
{
    auto *tabElement = new GraphTabElement(d->nodeStorage, d->projectFile, this);
    if (d->pluginManager) {
        tabElement->setPluginManager(*d->pluginManager);
    }
    if (d->undoManager) {
        auto *gv = tabElement->graphicsView();
        gv->setUndoManager(d->undoManager);
        connect(gv,
                &GraphicsView::onUndoCommandReceived,
                this,
                [this, tabElement](GraphicsView *view, QUndoCommand *command) {
                    d->commandToTab[command] = tabElement;
                });
    }
    auto *view = tabElement->graphicsView();
    view->setColorManagement(d->colorManagement);

    connect(tabElement, &GraphTabElement::historyUpdate, this, &TabWidget::setCurrentTabText);
    connect(view, &lvtqtc::GraphicsView::classNavigateRequested, this, [this](QString qname) {
        this->setCurrentGraphTab(GraphInfo{std::move(qname), NodeType::e_Class});
    });
    connect(view, &lvtqtc::GraphicsView::packageNavigateRequested, this, [this](QString qname) {
        this->setCurrentGraphTab(GraphInfo{std::move(qname), NodeType::e_Package});
    });
    connect(view, &lvtqtc::GraphicsView::componentNavigateRequested, this, [this](QString qname) {
        this->setCurrentGraphTab(GraphInfo{std::move(qname), NodeType::e_Component});
    });

    return tabElement;
}

void TabWidget::openNewGraphTab(std::optional<GraphInfo> info)
{
    auto *tabElement = createTabElement();
    QString title = [&]() {
        if (!info) {
            return tr("Unnamed %1").arg(count());
        }
        tabElement->setCurrentGraph(info->qualifiedName,
                                    GraphTabElement::HistoryType::History,
                                    nodeTypeToDiagramType(info->nodeType));
        return info->qualifiedName;
    }();
    int tabIdx = addTab(tabElement, title);
    setCurrentIndex(tabIdx);
}

void TabWidget::setCurrentGraphTab(GraphInfo const& info)
{
    auto *currentTab = qobject_cast<lvtqtw::GraphTabElement *>(currentWidget());
    auto diagramType = nodeTypeToDiagramType(info.nodeType);
    if (!currentTab->setCurrentGraph(info.qualifiedName, GraphTabElement::HistoryType::History, diagramType)) {
        return;
    }
    const auto idx = currentIndex();
    setTabIcon(idx, QIcon());
    setCurrentTabText(info.qualifiedName, diagramType);
}

lvtqtc::GraphicsView *TabWidget::graphicsView()
{
    auto *tabElement = qobject_cast<lvtqtw::GraphTabElement *>(currentWidget());
    if (!tabElement) {
        return nullptr; // RETURN
    }
    return tabElement->graphicsView();
}

void TabWidget::setUndoManager(lvtqtc::UndoManager *undoManager)
{
    d->undoManager = undoManager;
    auto onBeforeUndoRedo = [this](const QUndoCommand *command) {
        try {
            auto *tab = d->commandToTab.at(command);
            setCurrentWidget(tab);
        } catch (std::out_of_range const&) {
            // Ignore tab change if not found
            return;
        }
    };
    connect(d->undoManager, &UndoManager::onBeforeUndo, this, onBeforeUndoRedo);
    connect(d->undoManager, &UndoManager::onBeforeRedo, this, onBeforeUndoRedo);
    for (int index = 0; index < count(); ++index) {
        auto *tab = qobject_cast<GraphTabElement *>(widget(index));
        auto *gv = tab->graphicsView();
        gv->setUndoManager(undoManager);
        connect(gv, &GraphicsView::onUndoCommandReceived, this, [this, tab](GraphicsView *view, QUndoCommand *command) {
            d->commandToTab[command] = tab;
        });
    }
}

void TabWidget::saveBookmark(const QString& title, int idx, ProjectFile::BookmarkType type)
{
    auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(widget(idx));
    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(tabElement->graphicsView()->scene());
    auto jsonObj = scene->toJson();

    QJsonObject mainObj{{"scene", jsonObj},
                        {"tabname", title},
                        {"id", title},
                        {"zoom_level", tabElement->graphicsView()->zoomFactor()}};

    const cpp::result<void, ProjectFileError> ret = d->projectFile.saveBookmark(QJsonDocument(mainObj), type);

    if (ret.has_error()) {
        Q_EMIT errorMessage(tr("Error saving bookmark."));
    }

    setTabText(idx, title);
}

void TabWidget::saveTabsOnProject(ProjectFile::BookmarkType type)
{
    for (int i = 0; i < count(); i++) {
        saveBookmark(tabText(i), i, type);
    }
}

void TabWidget::loadBookmark(const QJsonDocument& doc)
{
    QJsonObject obj = doc.object();
    const auto idx = currentIndex();

    setTabText(idx, obj["tabname"].toString());
    setTabIcon(idx, QIcon(":/icons/build"));

    auto *scene = qobject_cast<GraphicsScene *>(graphicsView()->scene());
    scene->fromJson(obj["scene"].toObject());

    graphicsView()->setZoomFactor(obj["zoom_level"].toInt());
}

} // end namespace Codethink::lvtqtw
