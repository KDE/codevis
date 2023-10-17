// ct_lvtqtw_tabwidget.h                                             -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_TABWIDGET
#define INCLUDED_LVTQTW_TABWIDGET

#include <lvtqtw_export.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtplg_pluginmanager.h>

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtshr_graphenums.h>

#include <QTabWidget>

#include <memory>
#include <optional>
#include <string>

class QGraphicsView;
class QString;

namespace Codethink::lvtldr {
class NodeStorage;
}
namespace Codethink::lvtclr {
class ColorManagement;
}
namespace Codethink::lvtqtc {
class GraphicsView;
class UndoManager;
}
namespace Codethink::lvtprj {
class ProjectFile;
}

namespace Codethink::lvtqtw {

class GraphTabElement;

class LVTQTW_EXPORT TabWidget : public QTabWidget
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT

  public:
    struct GraphInfo {
        QString qualifiedName;
        lvtmdl::NodeType::Enum nodeType;
    };

    // CREATORS
    TabWidget(Codethink::lvtldr::NodeStorage& nodeStorage,
              lvtprj::ProjectFile& projectFile,
              std::shared_ptr<lvtclr::ColorManagement> colorManagement,
              lvtplg::PluginManager *pluginManager = nullptr,
              QWidget *parent = nullptr);
    // Constructor

    ~TabWidget() noexcept override;
    // Destructor

    void openNewGraphTab(std::optional<GraphInfo> info = std::nullopt);
    void setCurrentGraphTab(GraphInfo const& info);

    void closeTab(int idx);
    // called by a slot connection with closeTabRequested

    void setCurrentTabText(const QString& fullyQualifiedName, lvtshr::DiagramType type);
    // Slot for when we need to update the tab name to keep up with a history
    // change

    lvtqtc::GraphicsView *graphicsView();

    void setUndoManager(lvtqtc::UndoManager *undoManager);

    Q_SIGNAL void currentTabTextChanged(const QString& name, lvtshr::DiagramType type);
    Q_SIGNAL void errorMessage(const QString& errorMessage);
    // triggered when we set the text of the current tab.

    void saveBookmark(const QString& name, int idx, Codethink::lvtprj::ProjectFile::BookmarkType type);
    void loadBookmark(const QJsonDocument& doc);

    // Called when the application closes or when the user clicks on save.
    void saveTabsOnProject(Codethink::lvtprj::ProjectFile::BookmarkType type);

  private:
    // Forbid using addTab and insertTab, this should work with Graphs Only.
    using QTabWidget::addTab;
    using QTabWidget::insertTab;

    GraphTabElement *createTabElement();

    // DATA
    // TYPES
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtw

#endif
