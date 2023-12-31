// mainwindow.cpp                                                    -*-C++-*-

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

#include <mainwindow.h>

#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

#include <ct_lvtmdl_circular_relationships_model.h>
#include <ct_lvtmdl_errorsmodel.h>
#include <ct_lvtmdl_fieldstablemodel.h>
#include <ct_lvtmdl_methodstablemodel.h>
#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtmdl_namespacetreemodel.h>
#include <ct_lvtmdl_packagetreemodel.h>
#include <ct_lvtmdl_physicaltablemodels.h>
#include <ct_lvtmdl_usesintheimpltablemodel.h>
#include <ct_lvtmdl_usesintheinterfacetablemodel.h>

#include <ct_lvtldr_alloweddependencyloader.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_undo_manager.h>

#include <ct_lvtqtd_packageviewdelegate.h>

#include <ct_lvtqtw_configurationdialog.h>
#include <ct_lvtqtw_exportmanager.h>
#include <ct_lvtqtw_graphtabelement.h>
#include <ct_lvtqtw_parse_codebase.h>
#include <ct_lvtqtw_splitterview.h>
#include <ct_lvtqtw_tabwidget.h>

#include <ct_lvtqtc_lakosentity.h>

#include <ct_lvtcgn_app_adapter.h>

#include <aboutdialog.h>
#include <fstream>
#include <preferences.h>
#include <projectsettingsdialog.h>

#include <QDesktopServices>
#include <QInputDialog>
#include <QLoggingCategory>
#ifdef USE_WEB_ENGINE
#include <QWebEngineView>
#else
#include <QTextBrowser>
#endif

// in a header
Q_DECLARE_LOGGING_CATEGORY(LogWindow)

// in one source file
Q_LOGGING_CATEGORY(LogWindow, "log.window")

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtqtc;
using namespace Codethink::lvtmdl;
using namespace Codethink::lvtqtw;
using namespace Codethink::lvtqtd;
using namespace Codethink::lvtprj;
using namespace Codethink::lvtplg;

MainWindow::MainWindow(NodeStorage& sharedNodeStorage,
                       PluginManager *pluginManager,
                       UndoManager *undoManager,
                       DebugModel *debugModel):
    ui(sharedNodeStorage, d_projectFile, pluginManager),
    sharedNodeStorage(sharedNodeStorage),
    namespaceModel(new Codethink::lvtmdl::NamespaceTreeModel()),
    packageModel(new Codethink::lvtmdl::PackageTreeModel(sharedNodeStorage)),
    d_errorModel_p(new Codethink::lvtmdl::ErrorsModel()),
    d_status_bar(new CodeVisStatusBar()),
    d_pluginManager_p(pluginManager),
    d_undoManager_p(undoManager),
    debugModel(debugModel),
    d_dockReports(new QDockWidget(this)),
    d_reportsTabWidget(new QTabWidget(d_dockReports))
{
    using namespace Codethink::lvtqtw;
    using namespace Codethink::lvtmdl;

    ui.setupUi(this);

    auto *fieldsModel = new FieldsTableModel();
    auto *usesInTheImplTableModel = new UsesInTheImplTableModel();
    auto *usesInTheInterfaceTableModel = new UsesInTheInterfaceTableModel();
    auto *methodsTableModel = new MethodsTableModel();
    auto *providersTableModel = new PhysicalProvidersTableModel();
    auto *clientsTableModel = new PhysicalClientsTableModel();

    tableModels.append({fieldsModel,
                        usesInTheImplTableModel,
                        usesInTheInterfaceTableModel,
                        methodsTableModel,
                        providersTableModel,
                        clientsTableModel});

    ui.topMessageWidget->setVisible(false);
    ui.topMessageWidget->setWordWrap(true);

    connect(ui.mainSplitter, &SplitterView::currentTabChanged, this, &MainWindow::currentGraphSplitChanged);

    connect(ui.actionSvg, &QAction::triggered, this, &MainWindow::exportSvg);
    connect(ui.actionOpen_Project, &QAction::triggered, this, &MainWindow::openProjectAction);
    connect(ui.actionNew_Project, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui.actionClose_Project, &QAction::triggered, this, &MainWindow::closeProject);
    connect(ui.actionSave, &QAction::triggered, this, &MainWindow::saveProject);
    connect(ui.actionSave_as, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(ui.actionCode_Generation, &QAction::triggered, this, &MainWindow::openCodeGenerationWindow);
    connect(ui.actionParse_Codebase, &QAction::triggered, this, &MainWindow::openGenerateDatabase);

    connect(ui.namespaceFilter, &QLineEdit::textChanged, ui.namespaceTree, &TreeView::setFilterText);
    connect(ui.packagesFilter, &QLineEdit::textChanged, ui.packagesTree, &TreeView::setFilterText);

    connect(ui.namespaceTree, &TreeView::leafSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.namespaceTree, &TreeView::leafMiddleClicked, this, &MainWindow::newTabRequested);
    connect(ui.namespaceTree, &TreeView::branchRightClicked, this, &MainWindow::requestMenuNamespaceView);
    connect(ui.namespaceTree, &TreeView::leafRightClicked, this, &MainWindow::requestMenuNamespaceView);

    connect(ui.packagesTree, &TreeView::leafSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.packagesTree, &TreeView::leafMiddleClicked, this, &MainWindow::newTabRequested);
    connect(ui.packagesTree, &TreeView::branchSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.packagesTree, &TreeView::branchMiddleClicked, this, &MainWindow::newTabRequested);
    connect(ui.packagesTree, &TreeView::branchRightClicked, this, &MainWindow::requestMenuPackageView);
    connect(ui.packagesTree, &TreeView::leafRightClicked, this, &MainWindow::requestMenuPackageView);

    connect(ui.actionToggle_Split_View, &QAction::toggled, ui.mainSplitter, &SplitterView::toggle);
    connect(ui.actionPreferences, &QAction::triggered, this, [this]() {
        openPreferences();
    });
    connect(ui.actionGenerate_Database, &QAction::triggered, this, &MainWindow::newProjectFromSource);
    connect(ui.actionNew_Tab, &QAction::triggered, this, &MainWindow::newTab);
    connect(ui.actionClose_Current_Tab, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    connect(ui.actionQuit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui.actionSearch, &QAction::triggered, this, &MainWindow::requestSearch);
    connect(ui.actionProjectSettings, &QAction::triggered, this, &MainWindow::openProjectSettings);
    connect(ui.actionAbout, &QAction::triggered, this, [] {
        AboutDialog dialog;
        dialog.exec();
    });

    connect(ui.actionReset_usage_log, &QAction::triggered, this, [this] {
        this->debugModel->clear();
    });

    ui.namespaceTree->setModel(namespaceModel);
    ui.packagesTree->setModel(packageModel);
    ui.packagesTree->setItemDelegateForColumn(0, new PackageViewDelegate());

    ui.fieldsTable->setModel(fieldsModel);
    ui.usesInTheImplTable->setModel(usesInTheImplTableModel);
    ui.usesInTheInterfaceTable->setModel(usesInTheInterfaceTableModel);
    ui.methodsTable->setModel(methodsTableModel);
    ui.providersTable->setModel(providersTableModel);
    ui.clientsTable->setModel(clientsTableModel);
    ui.errorView->setModel(d_errorModel_p);

    ui.namespaceFilter->setVisible(false);
    ui.packagesFilter->setVisible(false);

    ui.namespaceFilter->installEventFilter(this);
    ui.packagesFilter->installEventFilter(this);
    ui.actionSearch->setShortcuts({Qt::Key_Find, Qt::CTRL + Qt::Key_F, Qt::Key_Slash});

    ui.mainSplitter->setUndoManager(d_undoManager_p);
    d_undoManager_p->createDock(this);

    configurePluginDocks();

    loadSettings();

#ifdef Q_OS_MACOS
    setDocumentMode(true);
#endif

    ui.packagesTree->setFocus();

    ui.graphLoadProgress->setVisible(false);
    ui.graphLoadProgress->setMinimum(static_cast<int>(Codethink::lvtqtc::GraphicsScene::GraphLoadProgress::Start));
    ui.graphLoadProgress->setMaximum(static_cast<int>(Codethink::lvtqtc::GraphicsScene::GraphLoadProgress::Done));

    ui.actionUndo->setShortcuts({Qt::Key_Undo, Qt::CTRL + Qt::Key_Z});
    connect(ui.actionUndo, &QAction::triggered, this, &MainWindow::triggerUndo);
    addAction(ui.actionUndo);

#if defined(Q_OS_WINDOWS)
    ui.actionRedo->setShortcuts({Qt::Key_Redo, Qt::CTRL + Qt::Key_Y});
#else
    ui.actionRedo->setShortcuts({Qt::Key_Redo, Qt::CTRL + Qt::SHIFT + Qt::Key_Z});
#endif
    connect(ui.actionRedo, &QAction::triggered, this, &MainWindow::triggerRedo);
    addAction(ui.actionRedo);

    // Always open with the welcome page on. When the welcomePage triggers a signal, or a
    // signal happens, we hide it.
    showWelcomeScreen();

    connect(ui.welcomeWidget, &WelcomeScreen::requestNewProject, this, &MainWindow::newProject);
    connect(ui.welcomeWidget, &WelcomeScreen::requestParseProject, this, &MainWindow::newProjectFromSource);
    connect(ui.welcomeWidget, &WelcomeScreen::requestExistingProject, this, &MainWindow::openProjectAction);

    setProjectWidgetsEnabled(false);

    // NOLINTNEXTLINE
    currentGraphTab = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(0));
    // reason for the no-lint. cppcoreguidelines wants us to initialize everything on the initalization
    // list, but we can't have the value of ui.mainspliter->widget(0) there.

    changeCurrentGraphWidget(0);

    QObject::connect(&sharedNodeStorage, &NodeStorage::storageChanged, this, [this] {
        d_projectFile.requestAutosave(Preferences::self()->document()->autoSaveBackupIntervalMsecs());
        Preferences::self()->document()->setLastDocument(QString::fromStdString(d_projectFile.backupPath().string()));
        Preferences::self()->sync();
    });

    setStatusBar(d_status_bar);
    connect(d_status_bar, &CodeVisStatusBar::mouseInteractionLabelClicked, this, [&]() {
        openPreferences(tr("Mouse"));
    });

    ui.errorDock->setVisible(false);
    connect(ui.actionDump_usage_log, &QAction::triggered, this, [this] {
        const QString fileName = QFileDialog::getSaveFileName();
        if (fileName.isEmpty()) {
            return;
        }

        const bool ret = this->debugModel->saveAs(fileName);
        if (!ret) {
            showMessage(tr("Could not save dump file"), KMessageWidget::MessageType::Error);
        }
    });

    connect(&d_projectFile, &Codethink::lvtprj::ProjectFile::bookmarksChanged, this, &MainWindow::bookmarksChanged);

    d_reportsTabWidget->setTabsClosable(true);
    connect(d_reportsTabWidget->tabBar(),
            &QTabBar::tabCloseRequested,
            d_reportsTabWidget->tabBar(),
            &QTabBar::removeTab);
    d_dockReports->setWindowTitle("Reports");
    d_dockReports->setObjectName("Reports");
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, d_dockReports);
    d_dockReports->setWidget(d_reportsTabWidget);
    d_dockReports->hide();
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::closeEvent(QCloseEvent *ev)
{
    saveSettings();
    if (d_projectFile.isOpen() && d_projectFile.isDirty()) {
        const auto choice = QMessageBox::warning(this,
                                                 tr("Save changes?"),
                                                 tr("Do you want to save the changes on the project?"),
                                                 QMessageBox::StandardButton::Save | QMessageBox::StandardButton::No);
        if (choice == QMessageBox::StandardButton::Save) {
            saveProject();
        }
    }
    QMainWindow::closeEvent(ev);
}

void MainWindow::setProjectWidgetsEnabled(bool enabled)
{
    const auto dockWidgets = findChildren<QDockWidget *>();
    for (auto *docks : dockWidgets) {
        docks->setEnabled(enabled);
    }

    ui.actionClose_Current_Tab->setEnabled(enabled);
    ui.actionClose_Project->setEnabled(enabled);
    ui.actionCode_Generation->setEnabled(enabled);
    ui.actionParse_Codebase->setEnabled(enabled);
    ui.actionImage->setEnabled(enabled);
    ui.actionNew_Tab->setEnabled(enabled);
    ui.actionSave_as->setEnabled(enabled);
    ui.actionSave->setEnabled(enabled);
    ui.actionSearch->setEnabled(enabled);
    ui.actionProjectSettings->setEnabled(enabled);
    ui.actionToggle_Split_View->setEnabled(enabled);
}

void MainWindow::closeProject()
{
    setProjectWidgetsEnabled(false);

    sharedNodeStorage.closeDatabase();
    cpp::result<void, Codethink::lvtprj::ProjectFileError> closed = d_projectFile.close();
    if (closed.has_error()) {
        showErrorMessage(
            tr("Error closing the current project\n%1").arg(QString::fromStdString(closed.error().errorMessage)));
        return;
    }

    if (d_undoManager_p) {
        d_undoManager_p->clear();
    }
    sharedNodeStorage.clear();
    packageModel->clear();
    namespaceModel->clear();
    ui.mainSplitter->closeAllTabs();
    if (ui.mainSplitter->count() > 1) {
        ui.mainSplitter->toggle();
    }
    d_status_bar->reset();
    showWelcomeScreen();
    Preferences::self()->document()->setLastDocument(QString());
}

bool MainWindow::askCloseCurrentProject()
{
    if (d_projectFile.isOpen()) {
        auto result = QMessageBox::question(this,
                                            tr("Really close project"),
                                            tr("Do you really want to close the project and create a new one?"),
                                            QMessageBox::Button::Yes,
                                            QMessageBox::Button::No);
        if (result == QMessageBox::Button::No) {
            return false;
        }
    }

    return true;
}

bool MainWindow::tryCreateEmptyProjectFile()
{
    cpp::result<void, Codethink::lvtprj::ProjectFileError> created = d_projectFile.createEmpty();
    if (created.has_error()) {
        showErrorMessage(tr("Could not create empty project, check your permissions on the temporary folder.\n%1")
                             .arg(QString::fromStdString(created.error().errorMessage)));
        return false;
    }
    return true;
}

void MainWindow::newProjectFromSource()
{
    if (newProject()) {
        openGenerateDatabase();
    }
}

bool MainWindow::newProject()
{
    if (!askCloseCurrentProject()) {
        return false;
    }
    closeProject();

    const QString projectName = requestProjectName();
    if (projectName.isEmpty()) {
        return false;
    }

    if (!tryCreateEmptyProjectFile()) {
        return false;
    }

    d_projectFile.setProjectName(projectName.toStdString());

    updateSessionPtr();
    showProjectView();
    setWindowTitle(qApp->applicationName() + " Unsaved Document");
    return true;
}

QString MainWindow::requestProjectName()
{
    bool ok = true;
    QString projectName =
        QInputDialog::getText(this, tr("Project Name"), tr("Project Name"), QLineEdit::Normal, tr("Untitled"), &ok);
    if (!ok) {
        return {};
    }
    return projectName;
}

void MainWindow::saveTabsOnProject(int idx, ProjectFile::BookmarkType type)
{
    if (ui.mainSplitter->count() <= idx) {
        return;
    }

    auto *tabWidget = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(idx));

    for (int i = 0; i < tabWidget->count(); i++) {
        auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(tabWidget->widget(i));
        auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(tabElement->graphicsView()->scene());
        auto jsonObj = scene->toJson();

        QJsonObject mainObj{{"scene", jsonObj}, {"tabname", tabWidget->tabText(i)}, {"id", QString::number(i)}};

        const cpp::result<void, ProjectFileError> ret = d_projectFile.saveBookmark(QJsonDocument(mainObj), type);
        if (ret.has_error()) {
            showMessage(tr("Error saving tab history."), KMessageWidget::MessageType::Error);
        }
    }
}

void MainWindow::saveProject()
{
    if (d_projectFile.location().empty()) {
        saveProjectAs();
        return;
    }

    d_projectFile.prepareSave();
    saveTabsOnProject(0, ProjectFile::BookmarkType::LeftPane);
    saveTabsOnProject(1, ProjectFile::BookmarkType::RightPane);

    cpp::result<void, Codethink::lvtprj::ProjectFileError> saved = d_projectFile.save();
    if (saved.has_error()) {
        showErrorMessage(tr("Error saving project: %1").arg(QString::fromStdString(saved.error().errorMessage)));
        return;
    }

    Preferences::self()->document()->setLastDocument(QString::fromStdString(d_projectFile.location().string()));
}

void MainWindow::saveProjectAs()
{
    const QString saveProjectPath =
        QFileDialog::getSaveFileName(this,
                                     tr("CodeVis Project File"),
                                     QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                     tr("CodeVis Project (*.lks)"));

    if (saveProjectPath.isEmpty()) {
        return;
    }

    d_projectFile.prepareSave();
    saveTabsOnProject(0, ProjectFile::BookmarkType::LeftPane);
    saveTabsOnProject(1, ProjectFile::BookmarkType::RightPane);

    cpp::result<void, Codethink::lvtprj::ProjectFileError> saved =
        d_projectFile.saveAs(saveProjectPath.toStdString(),
                             Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    if (saved.has_error()) {
        showErrorMessage(tr("Error saving project: %1").arg(QString::fromStdString(saved.error().errorMessage)));
        return;
    }

    Preferences::self()->document()->setLastDocument(QString::fromStdString(d_projectFile.location().string()));
    setWindowTitle(qApp->applicationName() + " " + QString::fromStdString(d_projectFile.location().string()));
}

void MainWindow::openCodeGenerationWindow()
{
    using Codethink::lvtcgn::app::CodegenAppAdapter;
    CodegenAppAdapter::run(this, sharedNodeStorage);
}

void MainWindow::openProjectAction()
{
    if (d_projectFile.isOpen()) {
        auto result = QMessageBox::question(this,
                                            tr("Really close project"),
                                            tr("Do you really want to close the project and open another?"),
                                            QMessageBox::Button::Yes,
                                            QMessageBox::Button::No);
        if (result == QMessageBox::Button::No) {
            return;
        }
        closeProject();
    }

    auto path = QFileDialog::getOpenFileName(this,
                                             tr("CodeVis Project File"),
                                             QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                             tr("CodeVis Project (*.lks)"));

    if (path.isEmpty()) {
        // User hits "Cancel" - Nothing to be done.
        return;
    }

    bool opened = openProjectFromPath(path);
    (void) opened; // CPPCHECK
}

bool MainWindow::openProjectFromPath(const QString& path)
{
    if (path.isEmpty()) {
        showErrorMessage(tr("Can't open an empty project."));
        showWelcomeScreen();
        return false;
    }

    cpp::result<void, Codethink::lvtprj::ProjectFileError> saved = d_projectFile.open(path.toStdString());
    if (saved.has_error()) {
        qDebug() << QString::fromStdString(saved.error().errorMessage);
        showErrorMessage(tr("Could not open project: %1").arg(QString::fromStdString(saved.error().errorMessage)));
        showWelcomeScreen();
        return false;
    }

    showProjectView();
    updateSessionPtr();

    const QString project = QString::fromStdString(d_projectFile.location().string());
    Preferences::self()->document()->setLastDocument(project);
    setWindowTitle(qApp->applicationName() + " " + project);

    loadTabsFromProject();
    bookmarksChanged();
    return true;
}

void MainWindow::loadTabsFromProject()
{
    auto leftTabs = d_projectFile.leftPanelTab();
    auto rightTabs = d_projectFile.rightPanelTab();

    const auto loadTab = [this](int id, const std::vector<QJsonDocument>& tabs) {
        auto *tabWidget = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(id));
        int idx = 0;
        for (const auto& tab : tabs) {
            if (idx != 0) {
                tabWidget->openNewGraphTab();
            }
            auto *currentTabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(tabWidget->widget(idx));
            auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(currentTabElement->graphicsView()->scene());

            QJsonObject obj = tab.object();

            tabWidget->setTabText(idx, obj["tabname"].toString());
            scene->fromJson(tab["scene"].toObject());
            idx += 1;
        }
    };

    loadTab(0, leftTabs);
    if (!rightTabs.empty()) {
        if (ui.mainSplitter->count() == 1) {
            toggleSplitView();
        }
        loadTab(1, rightTabs);
    }
}

void MainWindow::triggerUndo()
{
    if (!d_undoManager_p) {
        return;
    }

    if (qobject_cast<GraphicsView *>(focusWidget())) {
        d_undoManager_p->undo();
    }
}

void MainWindow::triggerRedo()
{
    if (!d_undoManager_p) {
        return;
    }

    if (qobject_cast<GraphicsView *>(focusWidget())) {
        d_undoManager_p->redo();
    }
}

void MainWindow::requestSearch()
{
    const auto *f = focusWidget();
    const auto wdgPairs =
        std::initializer_list<std::pair<QLineEdit *, QWidget *>>{{ui.namespaceFilter, ui.namespaceTree},
                                                                 {ui.packagesFilter, ui.packagesTree}};

    bool isPanels = false;
    for (const auto& [filter, widget] : wdgPairs) {
        if (f == filter || f == widget) {
            isPanels = true;
            filter->setVisible(!filter->isVisible());
            if (!filter->isVisible()) {
                filter->setText(QString());
            } else {
                filter->setFocus();
            }
        }
    }

    if (!isPanels) {
        auto *elm = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(currentGraphTab->currentWidget());

        elm->toggleFilterVisibility();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Hide search boxes.
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event); // NOLINT
        if (keyEvent->key() != Qt::Key_Escape) {
            return false;
        }

        auto *lineEdit = qobject_cast<QLineEdit *>(obj);
        if (!lineEdit) {
            return false;
        }
        lineEdit->setText(QString());
        lineEdit->setVisible(false);
        return true;
    }
    return false;
}

void MainWindow::openPreferences(std::optional<QString> preferredPage)
{
    if (!m_confDialog_p) {
        m_confDialog_p = new Codethink::lvtqtw::ConfigurationDialog(this);
    }
    m_confDialog_p->show();

    if (preferredPage) {
        m_confDialog_p->changeCurrentWidgetByString(*preferredPage);
    }
}

void MainWindow::closeCurrentTab()
{
    if (currentGraphTab) {
        currentGraphTab->closeTab(currentGraphTab->currentIndex());
    }
}

void MainWindow::newTab()
{
    if (currentGraphTab) {
        currentGraphTab->openNewGraphTab();
    }
}

void MainWindow::toggleSplitView() const
{
    ui.mainSplitter->toggle();
}

void MainWindow::selectLeftSplitView() const
{
    ui.mainSplitter->setCurrentIndex(0);
}

void MainWindow::selectRightSplitView() const
{
    ui.mainSplitter->setCurrentIndex(1);
}

void MainWindow::loadSettings()
{
    QSettings s;
    s.beginGroup("Application");
    bool isFirstRun = s.value("firstRun", true).toBool();

    if (isFirstRun) {
        tabifyDockWidget(ui.fieldsDock, ui.methodsDock);
        tabifyDockWidget(ui.fieldsDock, ui.usesInTheImplDock);
        tabifyDockWidget(ui.fieldsDock, ui.usesInTheInterfaceDock);
        return;
    }
    restoreGeometry(s.value("applicationGeometry").toByteArray());
    restoreState(s.value("applicationState").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.beginGroup("Application");
    s.setValue("firstRun", false);
    s.setValue("applicationGeometry", saveGeometry());
    s.setValue("applicationState", saveState());
}

void MainWindow::setCurrentGraphFromString(Codethink::lvtmdl::NodeType::Enum type, const QString& qualifiedName)
{
    const QModelIndex idx = packageModel->indexForData(std::vector<std::pair<QVariant, int>>({
        {qualifiedName, Codethink::lvtmdl::ModelRoles::e_QualifiedName},
        {type, Codethink::lvtmdl::ModelRoles::e_NodeType},
    }));

    if (!idx.isValid()) {
        qDebug() << "Could not find data for" << qualifiedName;
    }
    setCurrentGraph(idx);
}

void MainWindow::setCurrentGraph(const QModelIndex& idx)
{
    QString qualifiedName = idx.data(ModelRoles::e_QualifiedName).toString();
    NodeType::Enum type = static_cast<NodeType::Enum>(idx.data(ModelRoles::e_NodeType).toInt());
    currentGraphTab->setCurrentGraphTab(TabWidget::GraphInfo{qualifiedName, type});
    d_projectFile.setDirty();
}

void MainWindow::newTabRequested(const QModelIndex& idx)
{
    QString qualifiedName = idx.data(ModelRoles::e_QualifiedName).toString();
    NodeType::Enum type = static_cast<NodeType::Enum>(idx.data(ModelRoles::e_NodeType).toInt());
    currentGraphTab->openNewGraphTab(TabWidget::GraphInfo{qualifiedName, type});
}

void MainWindow::exportSvg()
{
    using GraphicsView = Codethink::lvtqtc::GraphicsView;
    using ExportManager = Codethink::lvtqtw::ExportManager;

    auto *view = qobject_cast<GraphicsView *>(ui.mainSplitter->graphicsView());
    assert(view);

    ExportManager exporter(view);
    auto res = exporter.exportSvg();
    if (res.has_error()) {
        showErrorMessage(QString::fromStdString(res.error().what));
    }
}

void MainWindow::changeCurrentGraphWidget(int graphTabIdx)
{
    using Codethink::lvtmdl::BaseTableModel;
    using Codethink::lvtqtc::GraphicsScene;
    using Codethink::lvtqtc::GraphicsView;
    using Codethink::lvtqtw::GraphTabElement;

    auto *tab = qobject_cast<GraphTabElement *>(currentGraphTab->widget(graphTabIdx));
    if (!tab) {
        return;
    }
    connect(tab, &GraphTabElement::sendMessage, this, &MainWindow::showMessage, Qt::UniqueConnection);

    auto *graphWidget = tab->graphicsView();
    if (!graphWidget) {
        return;
    }

    // disconnect everything related to the old graph and the window.
    [&]() {
        if (!currentGraphWidget) {
            return;
        }
        disconnect(currentGraphWidget, nullptr, this, nullptr);
        disconnect(this, nullptr, currentGraphWidget, nullptr);

        auto *graphicsScene = qobject_cast<GraphicsScene *>(currentGraphWidget->scene());
        if (!graphicsScene) {
            return;
        }
        disconnect(graphicsScene, nullptr, this, nullptr);
    }();

    currentGraphWidget = graphWidget;
    if (!currentGraphWidget) {
        return;
    }

    // Update window title
    auto projectLocation = d_projectFile.location();
    auto projectName = projectLocation.empty() ? tr("Untitled") : QString::fromStdString(projectLocation.string());
    setWindowTitle(qApp->applicationName() + " " + projectName + " " + currentGraphTab->tabText(graphTabIdx));

    // connect everything related to the new graph widget and the window
    auto addGWdgConnection = [this](auto signal, auto slot) {
        connect(currentGraphWidget, signal, this, slot, Qt::UniqueConnection);
    };
    addGWdgConnection(&GraphicsView::graphLoadStarted, &MainWindow::graphLoadStarted);
    addGWdgConnection(&GraphicsView::graphLoadFinished, &MainWindow::graphLoadFinished);
    addGWdgConnection(&GraphicsView::errorMessage, &MainWindow::showErrorMessage);

    // connect everything related to the new graph scene and the window
    auto *graphicsScene = qobject_cast<GraphicsScene *>(currentGraphWidget->scene());
    if (!graphicsScene) {
        return;
    }
    auto addGSConnection = [this, &graphicsScene](auto signal, auto slot) {
        connect(graphicsScene, signal, this, slot, Qt::UniqueConnection);
    };
    addGSConnection(&GraphicsScene::errorMessage, &MainWindow::showErrorMessage);
    addGSConnection(&GraphicsScene::graphLoadProgressUpdate, &MainWindow::graphLoadProgress);
    addGSConnection(&GraphicsScene::requestEnableWindow, &MainWindow::enableWindow);
    addGSConnection(&GraphicsScene::requestDisableWindow, &MainWindow::disableWindow);
    addGSConnection(&GraphicsScene::selectedEntityChanged, &MainWindow::updateFocusedEntityOnTableModels);
    addGSConnection(&GraphicsScene::createReportActionClicked, &MainWindow::createReport);
}

void MainWindow::createReport(std::string const& title, std::string const& htmlContents)
{
#ifdef USE_WEB_ENGINE
    auto *htmlReportTab = new QWebEngineView(this);
#else
    auto *htmlReportTab = new QTextBrowser(this);
#endif
    htmlReportTab->setHtml(QString::fromStdString(htmlContents));

    auto idx = d_reportsTabWidget->addTab(htmlReportTab, QString::fromStdString(title));
    d_reportsTabWidget->setCurrentIndex(idx);
    d_dockReports->show();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    static bool initialized = [this]() -> bool {
        const auto dockWidgets = findChildren<QDockWidget *>();
        for (auto *dock : dockWidgets) {
            auto *action = new QAction();
            action->setText(dock->windowTitle());
            action->setCheckable(true);
            action->setChecked(dock->isVisible());
            connect(action, &QAction::toggled, dock, &QDockWidget::setVisible);
            ui.menuView->addAction(action);
        }
        return true;
    }();

    Q_UNUSED(initialized);
}

void MainWindow::showWarningMessage(const QString& message)
{
    showMessage(message, KMessageWidget::MessageType::Warning);
}

void MainWindow::showErrorMessage(const QString& message)
{
    showMessage(message, KMessageWidget::MessageType::Error);
}

void MainWindow::showSuccessMessage(const QString& message)
{
    showMessage(message, KMessageWidget::MessageType::Positive);
}

QString MainWindow::currentMessage() const
{
    return ui.topMessageWidget->text();
}

void MainWindow::showMessage(const QString& message, KMessageWidget::MessageType type)
{
    if (message.isEmpty()) {
        ui.topMessageWidget->animatedHide();
        return;
    }

    ui.topMessageWidget->setText(message);
    ui.topMessageWidget->setMessageType(type);
    ui.topMessageWidget->animatedShow();
}

void MainWindow::currentGraphSplitChanged(Codethink::lvtqtw::TabWidget *tabWidget)
{
    if (currentGraphTab) {
        disconnect(currentGraphTab, &QTabWidget::currentChanged, this, &MainWindow::changeCurrentGraphWidget);
        disconnect(currentGraphTab,
                   &Codethink::lvtqtw::TabWidget::currentTabTextChanged,
                   this,
                   &MainWindow::focusedGraphChanged);
    }

    currentGraphTab = tabWidget;
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::changeCurrentGraphWidget);
    connect(tabWidget, &Codethink::lvtqtw::TabWidget::currentTabTextChanged, this, &MainWindow::focusedGraphChanged);
    connect(tabWidget,
            &Codethink::lvtqtw::TabWidget::requestSaveBookmark,
            this,
            &MainWindow::saveBookmark,
            Qt::UniqueConnection);
    changeCurrentGraphWidget(tabWidget->currentIndex());
}

void MainWindow::saveBookmark(const QString& title, int idx)
{
    auto *tabWidget = qobject_cast<Codethink::lvtqtw::TabWidget *>(sender());
    if (!tabWidget) {
        qDebug() << "Error, bookmark without tab widget";
        return;
    }

    auto *tabElement = qobject_cast<Codethink::lvtqtw::GraphTabElement *>(tabWidget->widget(idx));
    auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(tabElement->graphicsView()->scene());
    auto jsonObj = scene->toJson();

    QJsonObject mainObj{{"scene", jsonObj}, {"tabname", title}, {"id", title}};

    const cpp::result<void, ProjectFileError> ret =
        d_projectFile.saveBookmark(QJsonDocument(mainObj), Codethink::lvtprj::ProjectFile::Bookmark);
    if (ret.has_error()) {
        showMessage(tr("Error saving tab history."), KMessageWidget::MessageType::Error);
    }
}

void MainWindow::graphLoadStarted()
{
    // HACK: we are throwing two signals in sequence, hitting the assert.
    if (d_graphLoadRunning) {
        return;
    }

    disableWindow();

    QGuiApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    assert(!d_graphLoadRunning);
    d_graphLoadRunning = true;

    ui.topMessageWidget->clearActions();
    ui.topMessageWidget->animatedHide();
}

void MainWindow::graphLoadFinished()
{
    QGuiApplication::restoreOverrideCursor();

    enableWindow();

    assert(d_graphLoadRunning);
    d_graphLoadRunning = false;

    Q_EMIT databaseIdle();
}

QString MainWindow::progressDesc(Codethink::lvtqtc::GraphicsScene::GraphLoadProgress progress)
{
    using Progress = Codethink::lvtqtc::GraphicsScene::GraphLoadProgress;

    switch (progress) {
    case Progress::Start:
        // No description
        break;
    case Progress::CheckCache:
        return tr("Checking layout cache");
    case Progress::CdbLoad:
        return tr("Loading from code database");
    case Progress::QtEventLoop:
        return tr("Adding vertices");
    case Progress::VertexLayout:
        return tr("Laying out vertices");
    case Progress::PannelCollapse:
        return tr("Initialising container state");
    case Progress::FixRelations:
        return tr("Adjusting relation parents");
    case Progress::EdgesContainersLayout:
        return tr("Laying out edges and containers");
    case Progress::TransitiveReduction:
        return tr("Removing uneeded edges.");
    case Progress::Done:
        // no description
        break;
    }
    return {};
}

void MainWindow::graphLoadProgress(Codethink::lvtqtc::GraphicsScene::GraphLoadProgress progress)
{
    using Progress = Codethink::lvtqtc::GraphicsScene::GraphLoadProgress;
    Progress last;

    QString format = progressDesc(progress);

    switch (progress) {
    case Progress::Start:
        d_graphProgressTimer.start();
        d_graphProgressPartialTimer.start();
        // No description
        break;
    case Progress::Done:
        ui.graphLoadProgress->setVisible(false);
        qDebug() << "Graph load took" << d_graphProgressTimer.elapsed() << "ms";
        break;
    default:
        // no special handling
        break;
    }

    if (progress != Progress::Start) {
        assert(progress != Progress::Start);
        int iLast = static_cast<int>(progress) - 1;
        last = static_cast<Progress>(iLast);
        qDebug() << progressDesc(last) << "took" << d_graphProgressPartialTimer.restart() << "ms";
    }

    if (!format.isEmpty()) {
        qDebug() << "progress bar updated to" << format;

        format += ' ';
    }
    format += "%p%";

    ui.graphLoadProgress->setValue(static_cast<int>(progress));
    ui.graphLoadProgress->setFormat(format);

    ui.graphLoadProgress->setVisible(progress != Progress::Done);
}

void MainWindow::mousePressEvent(QMouseEvent *ev)
{
    if (!isEnabled()) {
        ev->ignore();
        return;
    }

    QMainWindow::mousePressEvent(ev);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    if (!isEnabled()) {
        ev->ignore();
        return;
    }

    QMainWindow::mousePressEvent(ev);
}

void MainWindow::enableWindow()
{
    qApp->processEvents();

    for (QWidget *child : qAsConst(d_disabledWidgets)) {
        child->setEnabled(true);
    }

    d_disabledWidgets.clear();
}

void MainWindow::disableWindow()
{
    // we don't want to disable thre graph load progress bar when we disable
    // widgets during a graph load. To achieve this we also have to not disable
    // its parent
    const QList<QWidget *> neverDisable{ui.graphLoadProgress, ui.centralarea};
    for (QWidget *child : findChildren<QWidget *>()) { // clazy:exclude=range-loop,range-loop-detach
        if (neverDisable.contains(child)) {
            continue;
        }
        if (child->isEnabled()) {
            d_disabledWidgets.append(child);
            child->setEnabled(false);
        }
    }

    qApp->processEvents();
}

void MainWindow::focusedGraphChanged(const QString& qualifiedName, Codethink::lvtshr::DiagramType type)
{
    m_currentQualifiedName = qualifiedName;

    const QString projectName =
        d_projectFile.location().empty() ? "Untitled" : QString::fromStdString(d_projectFile.location().string());

    setWindowTitle(qApp->applicationName() + " " + projectName + " " + qualifiedName);

    for (Codethink::lvtmdl::BaseTableModel *model : qAsConst(tableModels)) {
        model->setFocusedNode(qualifiedName.toStdString(), type);
    }
}

void MainWindow::openGenerateDatabase()
{
    using ParseCodebaseDialog = Codethink::lvtqtw::ParseCodebaseDialog;

    if (d_parseCodebaseDialog_p && d_parseCodebaseDialog_p->isVisible()) {
        return;
    }

    if (!d_parseCodebaseDialog_p) {
        d_parseCodebaseDialog_p = std::make_unique<ParseCodebaseDialog>(this);
        connect(d_parseCodebaseDialog_p.get(),
                &ParseCodebaseDialog::readyForDbUpdate,
                this,
                &MainWindow::generateDatabaseReadyForUpdate);
        connect(d_parseCodebaseDialog_p.get(),
                &ParseCodebaseDialog::parseFinished,
                this,
                &MainWindow::generateCodeDatabaseFinished);
        d_status_bar->setParseCodebaseWindow(*d_parseCodebaseDialog_p);
    }

    d_parseCodebaseDialog_p->setCodebasePath(QString::fromStdString(d_projectFile.openLocation().string()));
    d_parseCodebaseDialog_p->show();
}

void MainWindow::generateDatabaseReadyForUpdate()
{
    assert(d_parseCodebaseDialog_p);

    if (d_graphLoadRunning) {
        // call back to the dialog once the database is idle
        connect(this, &MainWindow::databaseIdle, this, &MainWindow::prepareForCodeDatabaseUpdate, Qt::UniqueConnection);
    } else {
        // the database is already idle; call directly
        prepareForCodeDatabaseUpdate();
    }
}

void MainWindow::prepareForCodeDatabaseUpdate()
// Database should now be idle
{
    /*
        // close the database so that we can replace the file
        // TODO: we need a proper way to close the database
        d_codeDatabase->setPath(":memory:");
        if (!d_codeDatabase->open(Codethink::lvtcdb::BaseDb::OpenType::NewDatabase)) {
            showErrorMessage(
                tr("Error preparing in-memory database for the current project file. Check the 'Error List' for
       details.")); return;
        }
    */

    // tell parseCodebaseDialog we are ready for it to do its thing
    d_parseCodebaseDialog_p->updateDatabase();
}

void MainWindow::generateCodeDatabaseFinished(Codethink::lvtqtw::ParseCodebaseDialog::State state)
{
    disconnect(this, &MainWindow::databaseIdle, this, &MainWindow::prepareForCodeDatabaseUpdate);

    if (state == ParseCodebaseDialog::State::Killed) {
        return;
    }

    // As soon as you parsed the whole codebase, that means that we need to copy all the
    // data to the Cad database, to show on the package tree. We might already have
    // things in the cad database, this might clash with the unique keys, so I can't
    // just dump the data from one db to another.
    // So, for the time being, let's just nuke the CadDb and recreate it.
    sharedNodeStorage.closeDatabase();
    const auto res = d_projectFile.resetCadDatabaseFromCodeDatabase();
    if (res.has_error()) {
        showErrorMessage(QString::fromStdString(res.error().errorMessage));
        return;
    }

    updateSessionPtr();

    using namespace Codethink::lvtldr;
    auto result = loadAllowedDependenciesFromDepFile(sharedNodeStorage, d_projectFile.sourceCodePath());
    if (result.has_error()) {
        switch (result.error().kind) {
        case ErrorLoadAllowedDependencies::Kind::AllowedDependencyFileCouldNotBeOpen:
            showWarningMessage(tr("Warning: Allowed dependencies file could not be open at '%1'")
                                   .arg(QString::fromStdString(d_projectFile.sourceCodePath().string())));
            break;
        case ErrorLoadAllowedDependencies::Kind::UnexpectedErrorAddingPhysicalDependency:
            showErrorMessage(tr("Unexpected error creating allowed dependencies (Could not add physical dependency)."));
            break;
        }
    }
}

void MainWindow::updateSessionPtr()
{
    sharedNodeStorage.setDatabaseSourcePath(d_projectFile.cadDatabasePath().string());
    packageModel->reload();

    // TODO: Properly populate GUI models from node storage
    //    for (auto *model : std::vector<Codethink::lvtmdl::BaseTreeModel *>{namespaceModel, packageModel}) {
    //        model->setDboSession(dboSessionPtr);
    //    }
    //
    //    for (Codethink::lvtmdl::BaseTableModel *model : qAsConst(tableModels)) {
    //        model->setDboSession(dboSessionPtr);
    //    }
    //
    //    d_errorModel_p->setDboSession(dboSessionPtr);
}

void MainWindow::showWelcomeScreen()
{
    ui.stackedWidget->setCurrentWidget(ui.welcomePage);
    ui.welcomePage->setEnabled(true);
}

void MainWindow::showProjectView()
{
    setProjectWidgetsEnabled(true);
    ui.stackedWidget->setCurrentWidget(ui.graphPage);
}

void MainWindow::openProjectSettings()
{
    auto projectSettingsDialog = ProjectSettingsDialog{d_projectFile};
    projectSettingsDialog.show();
    projectSettingsDialog.exec();
}

Codethink::lvtprj::ProjectFile& MainWindow::projectFile()
{
    return d_projectFile;
}

void MainWindow::requestMenuPackageView(const QModelIndex& idx, const QPoint& pos)
{
    QMenu menu;
    QAction *act = menu.addAction(tr("Load on Empty Scene"));
    connect(act, &QAction::triggered, this, [this, idx] {
        setCurrentGraph(idx);
    });

    act = menu.addAction(tr("Load on Current Scene"));
    connect(act, &QAction::triggered, this, [this, idx] {
        const QString qualName = idx.data(Codethink::lvtmdl::ModelRoles::e_QualifiedName).toString().toLocal8Bit();
        auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(currentGraphWidget->scene());
        scene->loadEntityByQualifiedName(qualName, QPoint());
        scene->reLayout();
    });

    act = menu.addAction("Open in Browser");
    connect(act, &QAction::triggered, this, [this, idx] {
        const NodeType::Enum type = static_cast<NodeType::Enum>(idx.data(ModelRoles::e_NodeType).toInt());
        if (type == NodeType::e_Package) {
            auto *node =
                sharedNodeStorage.findByQualifiedName(idx.data(ModelRoles::e_QualifiedName).toString().toStdString());
            auto *pkgNode = dynamic_cast<Codethink::lvtldr::PackageNode *>(node);
            QString filePath = QString::fromStdString(pkgNode->dirPath());
            filePath.replace("${SOURCE_DIR}", QString::fromStdString(projectFile().sourceCodePath().string()));
            const QFileInfo fInfo(filePath);
            if (!fInfo.exists()) {
                showErrorMessage(tr("No such file or directory: %1").arg(filePath));
                return;
            }

            const QUrl localFilePath = QUrl::fromLocalFile(filePath);
            QDesktopServices::openUrl(localFilePath);
        }
    });
    menu.exec(pos);
}

void MainWindow::requestMenuNamespaceView(const QModelIndex& idx, const QPoint& pos)
{
    const NodeType::Enum type = static_cast<NodeType::Enum>(idx.data(ModelRoles::e_NodeType).toInt());
    if (type != NodeType::e_Class) {
        return;
    }

    QMenu menu;
    QAction *act = menu.addAction(tr("Load on Empty Scene"));
    connect(act, &QAction::triggered, this, [this, idx] {
        setCurrentGraph(idx);
    });

    act = menu.addAction(tr("Load on Current Scene"));
    connect(act, &QAction::triggered, this, [this, idx] {
        const QString qualName = idx.data(Codethink::lvtmdl::ModelRoles::e_QualifiedName).toString().toLocal8Bit();
        auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(currentGraphWidget->scene());
        scene->loadEntityByQualifiedName(qualName, QPoint());
        scene->reLayout();
    });

    menu.exec(pos);
}

void MainWindow::bookmarksChanged()
{
    ui.menuBookmarks->clear();

    const auto bookmarks = d_projectFile.bookmarks();
    for (const auto& bookmark : bookmarks) {
        auto *bookmarkAction = ui.menuBookmarks->addAction(bookmark);
        connect(bookmarkAction, &QAction::triggered, this, [this, bookmark] {
            QJsonDocument doc = d_projectFile.getBookmark(bookmark);
            currentGraphTab->loadBookmark(doc);
        });
    }
}

void MainWindow::updateFocusedEntityOnTableModels(LakosEntity *entity)
{
    if (!entity) {
        return;
    }

    for (BaseTableModel *model : qAsConst(tableModels)) {
        model->setFocusedNode(entity->qualifiedName(), entity->instanceType());
    }
}

void MainWindow::configurePluginDocks()
{
    if (!d_pluginManager_p) {
        return;
    }

    auto createPluginDock = [this](std::string const& dockId, std::string const& title) {
        auto *pluginDock = new QDockWidget(this);
        pluginDock->setWindowTitle(QString::fromStdString(title));
        pluginDock->setObjectName(QString::fromStdString(dockId));
        addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pluginDock);
        auto *pluginDockWidget = new QWidget();
        pluginDockWidget->setLayout(new QFormLayout());
        pluginDock->setWidget(pluginDockWidget);
        d_pluginManager_p->registerPluginQObject(dockId, pluginDock);
    };
    auto addDockWdgFileField = [this](std::string const& dockId, std::string const& label, std::string& dataModel) {
        auto *pluginDockWidget = dynamic_cast<QDockWidget *>(d_pluginManager_p->getPluginQObject(dockId));
        auto *lineEdit = new QLineEdit();
        connect(lineEdit, &QLineEdit::textChanged, [&dataModel](QString const& newText) {
            dataModel = newText.toStdString();
        });
        auto *formLayout = dynamic_cast<QFormLayout *>(pluginDockWidget->widget()->layout());
        formLayout->addRow(new QLabel(QString::fromStdString(label)), lineEdit);
    };
    auto addTree = [this](std::string const& dockId, std::string const& treeId) {
        auto *pluginDockWidget = dynamic_cast<QDockWidget *>(d_pluginManager_p->getPluginQObject(dockId));
        auto *treeView = new QTreeView(pluginDockWidget);
        auto *treeModel = new QStandardItemModel{treeView};
        treeView->setHeaderHidden(true);
        treeView->setModel(treeModel);
        auto *formLayout = dynamic_cast<QFormLayout *>(pluginDockWidget->widget()->layout());
        formLayout->addRow(treeView);
        d_pluginManager_p->registerPluginQObject(treeId + "::view", treeView);
        d_pluginManager_p->registerPluginQObject(treeId + "::model", treeModel);
    };
    d_pluginManager_p->callHooksSetupDockWidget(createPluginDock, addDockWdgFileField, addTree);
}

WrappedUiMainWindow::WrappedUiMainWindow(NodeStorage& sharedNodeStorage,
                                         ProjectFile& projectFile,
                                         PluginManager *pluginManager):
    sharedNodeStorage(sharedNodeStorage), projectFile(projectFile), pluginManager(pluginManager)
{
}

void WrappedUiMainWindow::setupUi(QMainWindow *mw)
{
    Ui::MainWindow::setupUi(mw);

    mainSplitter = new Codethink::lvtqtw::SplitterView(sharedNodeStorage, projectFile, pluginManager, graphPage);
    mainSplitter->setObjectName(QString::fromUtf8("mainSplitter"));
    verticalLayout_10->addWidget(mainSplitter);
}
