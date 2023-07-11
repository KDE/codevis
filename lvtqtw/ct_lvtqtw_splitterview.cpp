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
#include <ct_lvtqtw_splitterview.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtqtc_undo_manager.h>
#include <ct_lvtqtw_tabwidget.h>

#include <QEvent>
#include <QString>

#include <optional>
#include <set>

namespace Codethink::lvtqtw {

struct SplitterView::Private {
    Codethink::lvtqtw::TabWidget *currentTab = nullptr;
    std::shared_ptr<lvtclr::ColorManagement> colorManagement;
    lvtldr::NodeStorage& nodeStorage;
    lvtqtc::UndoManager *undoManager = nullptr;
    lvtprj::ProjectFile& projectFile;
    lvtplg::PluginManager *pluginManager = nullptr;

    explicit Private(lvtldr::NodeStorage& nodeStorage,
                     lvtprj::ProjectFile& projectFile,
                     lvtplg::PluginManager *pluginManager):
        colorManagement(std::make_shared<lvtclr::ColorManagement>()),
        nodeStorage(nodeStorage),
        projectFile(projectFile),
        pluginManager(pluginManager)
    {
    }
};

// --------------------------------------------
// class SplitterView
// --------------------------------------------

SplitterView::SplitterView(lvtldr::NodeStorage& nodeStorage,
                           lvtprj::ProjectFile& projectFile,
                           lvtplg::PluginManager *pluginManager,
                           QWidget *parent):
    QSplitter(parent), d(std::make_unique<SplitterView::Private>(nodeStorage, projectFile, pluginManager))
{
    auto *wdg = new TabWidget(nodeStorage, projectFile, d->colorManagement, d->pluginManager);
    wdg->installEventFilter(this);
    d->currentTab = wdg;
    addWidget(wdg);
}

SplitterView::~SplitterView() = default;

void SplitterView::toggle()
{
    if (count() == 1) {
        auto *second_tabwdg = new TabWidget(d->nodeStorage, d->projectFile, d->colorManagement, d->pluginManager);
        second_tabwdg->installEventFilter(this);
        if (d->undoManager) {
            second_tabwdg->setUndoManager(d->undoManager);
        }
        addWidget(second_tabwdg);
    } else {
        assert(count() == 2);
        auto *second_tabwdg = qobject_cast<TabWidget *>(widget(1));
        assert(second_tabwdg);
        second_tabwdg->deleteLater();
        d->currentTab = qobject_cast<TabWidget *>(widget(0));
        Q_EMIT currentTabChanged(d->currentTab);
    }
}

int SplitterView::totalOpenTabs()
{
    int total = 0;
    for (int i = 0; i < count(); i++) {
        auto *tab = qobject_cast<Codethink::lvtqtw::TabWidget *>(widget(i));
        if (!tab) {
            continue;
        }
        total += tab->count();
    }
    return total;
}

void SplitterView::setCurrentIndex(int idx)
{
    if (count() < idx) {
        return;
    }

    d->currentTab = qobject_cast<Codethink::lvtqtw::TabWidget *>(widget(idx));
    Q_EMIT currentTabChanged(d->currentTab);
}

void SplitterView::setUndoManager(lvtqtc::UndoManager *undoManager)
{
    d->undoManager = undoManager;
    for (int i = 0; i < count(); ++i) {
        auto *tabWdg = qobject_cast<TabWidget *>(widget(i));
        if (tabWdg) {
            tabWdg->setUndoManager(undoManager);
        }
    }
}

bool SplitterView::eventFilter(QObject *watched, QEvent *event)
{
    if (std::set<QEvent::Type>{QEvent::MouseButtonPress, QEvent::ChildAdded, QEvent::FocusIn}.count(event->type())) {
        d->currentTab = qobject_cast<Codethink::lvtqtw::TabWidget *>(watched);
        Q_EMIT currentTabChanged(d->currentTab);
    }
    return QSplitter::eventFilter(watched, event);
}

void SplitterView::closeAllTabs()
{
    for (int i = 0; i < count(); ++i) {
        auto *wdg = qobject_cast<Codethink::lvtqtw::TabWidget *>(widget(i));
        for (int j = 0; j < wdg->count(); ++j) {
            wdg->closeTab(0);
        }
    }
}

lvtqtc::GraphicsView *SplitterView::graphicsView()
{
    if (!d->currentTab) {
        return nullptr; // RETURN
    }

    return d->currentTab->graphicsView();
}

} // end namespace Codethink::lvtqtw
