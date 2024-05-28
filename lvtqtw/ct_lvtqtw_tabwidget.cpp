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
    d->addGraphBtn->setIcon(QIcon::fromTheme("list-add"));
    d->addGraphBtn->setAutoRaise(true);

    // TODO: Icons
    setCornerWidget(d->addGraphBtn, Qt::TopLeftCorner);
    connect(d->addGraphBtn, &QToolButton::clicked, this, [this]() {
        this->openNewGraphTab();
    });
    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::closeTab);

    connect(this, &QTabWidget::tabBarDoubleClicked, this, [this](int idx) {
        if (idx != -1) {
            return;
        }
        openNewGraphTab();
    });

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
                saveBookmarkByTabIndex(idx);
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

void TabWidget::saveBookmarkByTabIndex(int tabIdx)
{
    InputDialog dlg;
    dlg.addTextField("name", tr("Bookmark Name:"));
    dlg.finish();
    auto res = dlg.exec();
    if (res == QDialog::DialogCode::Rejected) {
        return;
    }

    const auto text = std::any_cast<QString>(dlg.fieldValue("name"));
    saveBookmark(text, tabIdx, Codethink::lvtprj::ProjectFile::Bookmark);
}

void TabWidget::closeTab(int idx)
{
    QWidget *widgetAt = widget(idx);
    removeTab(idx);

    widgetAt->deleteLater();

    if (count() == 0) {
        openNewGraphTab();
    }
}

void TabWidget::setCurrentTabText(const QString& fullyQualifiedName)
{
    setTabText(currentIndex(), fullyQualifiedName);
    Q_EMIT currentTabTextChanged(fullyQualifiedName);
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

    connect(tabElement, &GraphTabElement::historyUpdate, this, [this](const QString& bookmark) {
        loadBookmark(d->projectFile.getBookmark(bookmark), HistoryType::NoHistory);
    });
    return tabElement;
}

void TabWidget::replaceGraphAt(int idx, const QString& qualifiedName)
{
    auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(widget(idx));
    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(tabElement->graphicsView()->scene());
    scene->clearGraph();
    scene->loadEntityByQualifiedName(qualifiedName, QPoint{});
}

void TabWidget::openNewGraphTab(std::optional<QSet<QString>> qualifiedNames)
{
    std::cout << "Running open new graph tab";
    auto *tabElement = createTabElement();
    int tabIdx = addTab(tabElement, tr("Unnamed %1").arg(count()));
    setCurrentIndex(tabIdx);

    if (!qualifiedNames || qualifiedNames.value().empty()) {
        return;
    }

    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(tabElement->graphicsView()->scene());
    for (const auto& qualifiedName : qualifiedNames.value()) {
        scene->loadEntityByQualifiedName(qualifiedName, QPoint{});
    }
    scene->reLayout();
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

void TabWidget::saveTabsOnProject(ProjectFile::BookmarkType type)
{
    for (int i = 0; i < count(); i++) {
        saveBookmark(tabText(i), i, type);
    }
}

void TabWidget::saveBookmark(const QString& title, int idx, ProjectFile::BookmarkType type)
{
    auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(widget(idx));
    tabElement->saveBookmark(title, type);

    setTabText(idx, title);
}

void TabWidget::loadBookmark(const QJsonDocument& doc, lvtshr::HistoryType historyType)
{
    QJsonObject obj = doc.object();
    const auto idx = currentIndex();
    auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(widget(idx));

    setTabText(idx, obj["tabname"].toString());
    setTabIcon(idx, QIcon(":/icons/build"));

    tabElement->loadBookmark(doc, historyType);
}

} // end namespace Codethink::lvtqtw
