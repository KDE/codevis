// mainwindow.h                                                      -*-C++-*-

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtmdl_debugmodel.h>
#include <ct_lvtmdl_modelhelpers.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtw_splitterview.h>
#include <ct_lvtqtw_statusbar.h>

#include <ct_lvtprj_projectfile.h>

#include <ui_mainwindow.h>

#include <KXmlGuiWindow>

#include <QElapsedTimer>
#include <QFileDialog>
#include <QSplitter>
#include <optional>

class QModelIndex;
class QLabel;

class CodeVisApplicationTestFixture;

namespace Codethink::lvtmdl {
class CircularRelationshipsModel;
class ErrorsModel;
class NamespaceTreeModel;
class PackageTreeModel;
class TreeFilterModel;
class BaseTableModel;
} // namespace Codethink::lvtmdl

namespace Codethink::lvtqtc {
class GraphicsView;
class UndoManager;
} // namespace Codethink::lvtqtc

namespace Codethink::lvtqtw {
class TabWidget;
class ParseCodebaseDialog;
class ConfigurationDialog;
} // namespace Codethink::lvtqtw

class WrappedUiMainWindow : public Ui::MainWindow {
  public:
    WrappedUiMainWindow(Codethink::lvtldr::NodeStorage& sharedNodeStorage,
                        Codethink::lvtprj::ProjectFile& projectFile,
                        Codethink::lvtplg::PluginManager *pluginManager = nullptr);
    void setupUi(QMainWindow *mw);

    Codethink::lvtqtw::SplitterView *mainSplitter = nullptr;
    Codethink::lvtldr::NodeStorage& sharedNodeStorage;
    Codethink::lvtprj::ProjectFile& projectFile;
    Codethink::lvtplg::PluginManager *pluginManager = nullptr;
};

class MainWindow : public KXmlGuiWindow {
    Q_OBJECT
  public:
    friend class ::CodeVisApplicationTestFixture;

    // initializes the qrc file, should be called before the mainwindow is build.
    static void initializeResource();
    explicit MainWindow(Codethink::lvtldr::NodeStorage& sharedNodeStorage,
                        Codethink::lvtplg::PluginManager *pluginManager = nullptr,
                        Codethink::lvtqtc::UndoManager *undoManager = nullptr,
                        Codethink::lvtmdl::DebugModel *debugModel = nullptr);
    ~MainWindow() noexcept;

    [[nodiscard]] bool openProjectFromPath(const QString& path);
    void openProjectSettings();

    void newTab();
    // Creates a new tab on the focused tabwidget.

    void closeCurrentTab();
    // closes the current tab on the focused tabwidget.

    void toggleSplitView() const;

    void selectLeftSplitView() const;
    void selectRightSplitView() const;

    void setCurrentGraphFromString(Codethink::lvtmdl::NodeType::Enum type, const QString& qualifiedName);
    // searches for an QModelIndex that matches the str, and calls setCurrentGraph

    Codethink::lvtprj::ProjectFile& projectFile();

    [[nodiscard]] QString currentMessage() const;

    void setupActions();

  protected:
    virtual QString requestProjectName();

  private:
    void saveTabsOnProject();
    void loadTabsFromProject();

    void openGenerateDatabase();

    void openCodeGenerationWindow();

    void exportSvg();

    void currentGraphSplitChanged(Codethink::lvtqtw::TabWidget *tabWidget);

    void closeProject();
    bool newProject();
    void newProjectFromSource();

    void openProjectAction();

    [[nodiscard]] bool askCloseCurrentProject();
    [[nodiscard]] bool tryCreateEmptyProjectFile();

    void setProjectWidgetsEnabled(bool enabled);
    void createStatusBar();

    void updateSessionPtr();
    // when a database changes, we need to update the session
    // pointer to all objects that can query the db.

    void showWelcomeScreen();
    void showProjectView();

    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void closeEvent(QCloseEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    void configurePluginDocks();

    Q_SLOT void focusedGraphChanged(const QString& qualifiedName);
    Q_SLOT void graphLoadStarted();
    Q_SLOT void graphLoadFinished();
    Q_SLOT void triggerUndo();
    Q_SLOT void triggerRedo();

    Q_SLOT void bookmarksChanged();

    Q_SLOT void generateDatabaseReadyForUpdate();
    // database generation is ready to replace our database when it is next
    // idle

    Q_SLOT void prepareForCodeDatabaseUpdate();
    // To be called once the database is idle

    Q_SLOT void generateCodeDatabaseFinished(Codethink::lvtqtw::ParseCodebaseDialog::State state);
    // database generation is finished. Load new database.

    Q_SLOT void setCurrentGraph(const QModelIndex& idx);
    // callback when something is selected in the tree view

    Q_SLOT void newTabRequested(const QModelIndex& idx);
    Q_SLOT void newTabRequested(const QModelIndexList& idxList);
    // callback when a new tab is requested from the tree view

    Q_SLOT void changeCurrentGraphWidget(int graphTabIdx);
    // callback for when the tab we update the title.

    Q_SLOT void openPreferences();
    // callback for openPreferences action

    Q_SLOT void openPreferencesAt(std::optional<QString> preferredPage = std::nullopt);

    Q_SLOT void saveProject();
    // saves the current project

    Q_SLOT void saveProjectAs();
    // saves the current project on the specified folder.

    Q_SLOT void showWarningMessage(const QString& message);
    Q_SLOT void showErrorMessage(const QString& message);
    Q_SLOT void showSuccessMessage(const QString& message);
    Q_SLOT void showMessage(const QString& message, KMessageWidget::MessageType type);

    Q_SLOT void requestSearch();
    // Show / Hide the search box on the selected widget, if the widget is searchable.

    Q_SLOT void disableWindow();

    Q_SLOT void enableWindow();

    Q_SIGNAL void databaseIdle();

    Q_SLOT void
    requestMenuPackageView(const QModelIndexList& multiSelection, const QModelIndex& clickedOn, const QPoint& pos);
    Q_SLOT void requestMenuNamespaceView([[maybe_unused]] const QModelIndexList& multiSelection,
                                         const QModelIndex& clickedOn,
                                         const QPoint& pos);
    Q_SLOT void updatePluginData();

    void createReport(std::string const& title, std::string const& htmlContents);

    WrappedUiMainWindow ui;

    Codethink::lvtldr::NodeStorage& sharedNodeStorage;

    std::unique_ptr<Codethink::lvtqtw::ParseCodebaseDialog> d_parseCodebaseDialog_p;

    // Data Models
    Codethink::lvtmdl::NamespaceTreeModel *namespaceModel = nullptr;
    Codethink::lvtmdl::PackageTreeModel *packageModel = nullptr;
    Codethink::lvtmdl::ErrorsModel *d_errorModel_p = nullptr;

    QList<Codethink::lvtmdl::BaseTableModel *> tableModels;

    // TODO: Maybe we should move these variables to a GraphWidgetManager of sorts.
    Codethink::lvtqtc::GraphicsView *currentGraphWidget = nullptr;
    Codethink::lvtqtw::TabWidget *currentGraphTab = nullptr;

    Codethink::lvtqtw::CodeVisStatusBar *d_status_bar;
    QString m_currentQualifiedName;

    QList<QWidget *> d_disabledWidgets;
    // Widgets we disabled during the last graph load

    bool d_graphLoadRunning = false;

    QElapsedTimer d_graphProgressPartialTimer;
    QElapsedTimer d_graphProgressTimer;

    Codethink::lvtprj::ProjectFile d_projectFile;

    Codethink::lvtplg::PluginManager *d_pluginManager_p = nullptr;
    Codethink::lvtqtc::UndoManager *d_undoManager_p = nullptr;
    Codethink::lvtmdl::DebugModel *debugModel = nullptr;

    QDockWidget *d_dockReports = nullptr;
    QTabWidget *d_reportsTabWidget = nullptr;
};

#endif
