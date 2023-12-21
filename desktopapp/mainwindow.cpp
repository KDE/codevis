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

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>

#include <ct_lvtmdl_errorsmodel.h>
#include <ct_lvtmdl_fieldstablemodel.h>
#include <ct_lvtmdl_methodstablemodel.h>
#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtmdl_namespacetreemodel.h>
#include <ct_lvtmdl_packagetreemodel.h>
#include <ct_lvtmdl_physicaltablemodels.h>
#include <ct_lvtmdl_usesintheimpltablemodel.h>
#include <ct_lvtmdl_usesintheinterfacetablemodel.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentitypluginutils.h>
#include <ct_lvtqtc_undo_manager.h>

#include <ct_lvtqtd_packageviewdelegate.h>

#include <ct_lvtqtw_configurationdialog.h>
#include <ct_lvtqtw_exportmanager.h>
#include <ct_lvtqtw_graphtabelement.h>
#include <ct_lvtqtw_parse_codebase.h>
#include <ct_lvtqtw_splitterview.h>
#include <ct_lvtqtw_tabwidget.h>

#include <ct_lvtcgn_app_adapter.h>

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

#include <KActionCollection>
#include <KLocalizedString>
#include <KStandardAction>
#include <kwidgetsaddons_version.h>

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

void MainWindow::initializeResource()
{
    static auto initialized = false;
    if (!initialized) {
        Q_INIT_RESOURCE(desktopapp);
    }
}

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
    connect(ui.namespaceFilter, &QLineEdit::textChanged, ui.namespaceTree, &TreeView::setFilterText);
    connect(ui.packagesFilter, &QLineEdit::textChanged, ui.packagesTree, &TreeView::setFilterText);

    connect(ui.namespaceTree, &TreeView::leafSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.namespaceTree,
            &TreeView::leafMiddleClicked,
            this,
            qOverload<const QModelIndex&>(&MainWindow::newTabRequested));
    connect(ui.namespaceTree, &TreeView::branchRightClicked, this, &MainWindow::requestMenuNamespaceView);
    connect(ui.namespaceTree, &TreeView::leafRightClicked, this, &MainWindow::requestMenuNamespaceView);

    connect(ui.packagesTree, &TreeView::leafSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.packagesTree,
            &TreeView::leafMiddleClicked,
            this,
            qOverload<const QModelIndex&>(&MainWindow::newTabRequested));
    connect(ui.packagesTree, &TreeView::branchSelected, this, &MainWindow::setCurrentGraph);
    connect(ui.packagesTree,
            &TreeView::branchMiddleClicked,
            this,
            qOverload<const QModelIndex&>(&MainWindow::newTabRequested));
    connect(ui.packagesTree, &TreeView::branchRightClicked, this, &MainWindow::requestMenuPackageView);
    connect(ui.packagesTree, &TreeView::leafRightClicked, this, &MainWindow::requestMenuPackageView);

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
    ui.mainSplitter->setUndoManager(d_undoManager_p);
    d_undoManager_p->createDock(this);

    configurePluginDocks();

#ifdef Q_OS_MACOS
    setDocumentMode(true);
#endif

    ui.packagesTree->setFocus();

    // Always open with the welcome page on. When the welcomePage triggers a signal, or a
    // signal happens, we hide it.
    showWelcomeScreen();

    connect(ui.welcomeWidget, &WelcomeScreen::requestNewProject, this, &MainWindow::newProject);
    connect(ui.welcomeWidget, &WelcomeScreen::requestParseProject, this, &MainWindow::newProjectFromSource);
    connect(ui.welcomeWidget, &WelcomeScreen::requestExistingProject, this, &MainWindow::openProjectAction);

    // NOLINTNEXTLINE
    currentGraphTab = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(0));
    // reason for the no-lint. cppcoreguidelines wants us to initialize everything on the initalization
    // list, but we can't have the value of ui.mainspliter->widget(0) there.

    changeCurrentGraphWidget(0);

    QObject::connect(&sharedNodeStorage, &NodeStorage::storageChanged, this, [this] {
        d_projectFile.requestAutosave(Preferences::autoSaveBackupIntervalMsecs());
        Preferences::setLastDocument(QString::fromStdString(d_projectFile.backupPath().string()));
        Preferences::self()->save();
    });

    setStatusBar(d_status_bar);
    connect(d_status_bar, &CodeVisStatusBar::mouseInteractionLabelClicked, this, [&]() {
        openPreferencesAt(tr("Mouse"));
    });

    ui.errorDock->setVisible(false);

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

    ui.pluginEditorView->setPluginManager(d_pluginManager_p);

    setupActions();
    setProjectWidgetsEnabled(false);
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::setupActions()
{
    auto *action = new QAction(this);
    action->setText(tr("New from source"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("new_project_from_source", action);
    actionCollection()->setDefaultShortcut(action,
                                           static_cast<QKeySequence>(static_cast<int>(Qt::CTRL)
                                                                     | static_cast<int>(Qt::SHIFT)
                                                                     | static_cast<int>(Qt::Key_N)));
    connect(action, &QAction::triggered, this, &MainWindow::newProjectFromSource);

    action = new QAction(this);
    action->setText(tr("Parse Aditional Source"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("parse_aditional", action);
    actionCollection()->setDefaultShortcut(
        action,
        static_cast<QKeySequence>(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_P)));
    connect(action, &QAction::triggered, this, &MainWindow::openGenerateDatabase);

    action = new QAction(this);
    action->setText(tr("Dump usage log"));
    actionCollection()->addAction("dump_usage_log", action);
    connect(action, &QAction::triggered, this, [this] {
        const QString fileName = QFileDialog::getSaveFileName();
        if (fileName.isEmpty()) {
            return;
        }

        const bool ret = this->debugModel->saveAs(fileName);
        if (!ret) {
            showMessage(tr("Could not save dump file"), KMessageWidget::MessageType::Error);
        }
    });

    action = new QAction(this);
    action->setText(tr("Reset usage log"));
    connect(action, &QAction::triggered, this, [this] {
        debugModel->clear();
    });

    action = new QAction(this);
    action->setText(tr("Generate Code"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("generate_code", action);
    actionCollection()->setDefaultShortcut(
        action,
        static_cast<QKeySequence>(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_G)));
    connect(action, &QAction::triggered, this, &MainWindow::openCodeGenerationWindow);

    action = new QAction(this);
    action->setText(tr("Svg"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("export_svg", action);
    connect(action, &QAction::triggered, this, &MainWindow::exportSvg);

    action = new QAction(this);
    action->setCheckable(true);
    action->setText(tr("Toggle split view"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("toggle_split_view", action);
    connect(action, &QAction::toggled, this, &MainWindow::toggleSplitView);

    action = new QAction(this);
    action->setText(tr("New Tab"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("new_tab", action);
    actionCollection()->setDefaultShortcut(
        action,
        static_cast<QKeySequence>(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_T)));
    connect(action, &QAction::triggered, this, &MainWindow::newTab);

    action = new QAction(this);
    action->setText(tr("Close current tab"));
    action->setIcon(QIcon::fromTheme("document-new"));
    actionCollection()->addAction("close_current_tab", action);
    actionCollection()->setDefaultShortcut(action,
                                           static_cast<QKeySequence>(static_cast<int>(Qt::CTRL)
                                                                     | static_cast<int>(Qt::SHIFT)
                                                                     | static_cast<int>(Qt::Key_W)));
    connect(action, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    // Common Set of Actions that most applications have. Those *do not* need to be
    // specified in the codevisui.rc
    KStandardAction::find(this, &MainWindow::requestSearch, actionCollection());
    KStandardAction::openNew(this, &MainWindow::newProject, actionCollection());
    KStandardAction::close(this, &MainWindow::closeProject, actionCollection());
    KStandardAction::undo(this, &MainWindow::triggerUndo, actionCollection());
    KStandardAction::redo(this, &MainWindow::triggerRedo, actionCollection());
    KStandardAction::preferences(this, &MainWindow::openPreferences, actionCollection());
    KStandardAction::save(this, &MainWindow::saveProject, actionCollection());
    KStandardAction::saveAs(this, &MainWindow::saveProjectAs, actionCollection());
    KStandardAction::open(this, &MainWindow::openProjectAction, actionCollection());
    KStandardAction::quit(qApp, &QCoreApplication::quit, actionCollection());

    setupGUI(Default, QStringLiteral(":/ui_files/codevisui.rc"));
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
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

    // Uncomment this if you want to see all names of configured actions.
    // for (const auto *action : actionCollection()->actions()) {
    //    std::cout << action->objectName().toStdString() << std::endl;
    //}

    actionCollection()->action("close_current_tab")->setEnabled(enabled);
    actionCollection()->action("file_close")->setEnabled(enabled);
    actionCollection()->action("generate_code")->setEnabled(enabled);
    actionCollection()->action("parse_aditional")->setEnabled(enabled);
    actionCollection()->action("export_svg")->setEnabled(enabled);
    actionCollection()->action("new_tab")->setEnabled(enabled);
    actionCollection()->action("file_save_as")->setEnabled(enabled);
    actionCollection()->action("file_save")->setEnabled(enabled);
    actionCollection()->action("edit_find")->setEnabled(enabled);
    actionCollection()->action("toggle_split_view")->setEnabled(enabled);
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
    Preferences::setLastDocument(QString());
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

void MainWindow::saveTabsOnProject()
{
    auto *tabWidget = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(0));
    if (tabWidget) {
        tabWidget->saveTabsOnProject(ProjectFile::BookmarkType::LeftPane);
    }

    tabWidget = qobject_cast<Codethink::lvtqtw::TabWidget *>(ui.mainSplitter->widget(1));
    if (tabWidget) {
        tabWidget->saveTabsOnProject(ProjectFile::BookmarkType::RightPane);
    }
}

void MainWindow::saveProject()
{
    if (d_projectFile.location().empty()) {
        saveProjectAs();
        return;
    }

    d_projectFile.prepareSave();
    saveTabsOnProject();

    cpp::result<void, Codethink::lvtprj::ProjectFileError> saved = d_projectFile.save();
    if (saved.has_error()) {
        showErrorMessage(tr("Error saving project: %1").arg(QString::fromStdString(saved.error().errorMessage)));
        return;
    }

    Preferences::setLastDocument(QString::fromStdString(d_projectFile.location().string()));
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
    saveTabsOnProject();

    cpp::result<void, Codethink::lvtprj::ProjectFileError> saved =
        d_projectFile.saveAs(saveProjectPath.toStdString(),
                             Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    if (saved.has_error()) {
        showErrorMessage(tr("Error saving project: %1").arg(QString::fromStdString(saved.error().errorMessage)));
        return;
    }

    Preferences::setLastDocument(QString::fromStdString(d_projectFile.location().string()));
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
    Preferences::setLastDocument(project);
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

void MainWindow::openPreferences()
{
    Codethink::lvtqtw::ConfigurationDialog confDialog(d_pluginManager_p, this);
    confDialog.exec();
}

void MainWindow::openPreferencesAt(std::optional<QString> preferredPage)
{
    Codethink::lvtqtw::ConfigurationDialog confDialog(d_pluginManager_p, this);
    if (preferredPage) {
        confDialog.changeCurrentWidgetByString(*preferredPage);
    }
    confDialog.exec();
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
    // TODO: Fix This

    // QString qualifiedName = idx.data(ModelRoles::e_QualifiedName).toString();
    // NodeType::Enum type = static_cast<NodeType::Enum>(idx.data(ModelRoles::e_NodeType).toInt());
    // currentGraphTab->setCurrentGraphTab(TabWidget::GraphInfo{qualifiedName, type});
    d_projectFile.setDirty();
}

void MainWindow::newTabRequested(const QModelIndex& idx)
{
    newTabRequested(QModelIndexList({idx}));
}

void MainWindow::newTabRequested(const QModelIndexList& idxList)
{
    QSet<QString> qualifiedNames;
    for (const auto idx : idxList) {
        qualifiedNames.insert(idx.data(ModelRoles::e_QualifiedName).toString());
    }
    currentGraphTab->openNewGraphTab(QSet<QString>({qualifiedNames}));
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
    addGWdgConnection(&Codethink::lvtqtc::GraphicsView::graphLoadStarted, &MainWindow::graphLoadStarted);
    addGWdgConnection(&Codethink::lvtqtc::GraphicsView::graphLoadFinished, &MainWindow::graphLoadFinished);
    addGWdgConnection(&Codethink::lvtqtc::GraphicsView::errorMessage, &MainWindow::showErrorMessage);

    // connect everything related to the new graph scene and the window
    auto *graphicsScene = qobject_cast<GraphicsScene *>(currentGraphWidget->scene());
    if (!graphicsScene) {
        return;
    }
    auto addGSConnection = [this, &graphicsScene](auto signal, auto slot) {
        connect(graphicsScene, signal, this, slot, Qt::UniqueConnection);
    };
    addGSConnection(&Codethink::lvtqtc::GraphicsScene::errorMessage, &MainWindow::showErrorMessage);
    addGSConnection(&Codethink::lvtqtc::GraphicsScene::requestEnableWindow, &MainWindow::enableWindow);
    addGSConnection(&Codethink::lvtqtc::GraphicsScene::requestDisableWindow, &MainWindow::disableWindow);
    addGSConnection(&Codethink::lvtqtc::GraphicsScene::createReportActionClicked, &MainWindow::createReport);

    if (d_pluginManager_p) {
        auto getSceneName = [&graphicsScene]() {
            return graphicsScene->objectName().toStdString();
        };
        d_pluginManager_p->callHooksActiveSceneChanged(getSceneName);
    }

    addGSConnection(&GraphicsScene::graphLoadFinished, &MainWindow::updatePluginData);
}

void MainWindow::updatePluginData()
{
    if (!d_pluginManager_p) {
        return;
    }

    auto *graphicsScene = qobject_cast<GraphicsScene *>(currentGraphWidget->scene());

    auto getSceneName = [&graphicsScene]() {
        return graphicsScene->objectName().toStdString();
    };

    auto getVisibleEntities = [&graphicsScene]() {
        auto entities = std::vector<Entity>{};
        for (auto&& e : graphicsScene->allEntities()) {
            entities.push_back(createWrappedEntityFromLakosEntity(e));
        }
        return entities;
    };

    auto getEdgeByQualifiedName = [graphicsScene](std::string const& fromQualifiedName,
                                                  std::string const& toQualifiedName) -> std::optional<Edge> {
        auto *fromEntity = graphicsScene->entityByQualifiedName(fromQualifiedName);
        if (!fromEntity) {
            return std::nullopt;
        }
        auto *toEntity = graphicsScene->entityByQualifiedName(toQualifiedName);
        if (!toEntity) {
            return std::nullopt;
        }
        return createWrappedEdgeFromLakosEntity(fromEntity, toEntity);
    };

    auto getProjectData = [this]() {
        auto getSourceCodePath = [this]() {
            return this->d_projectFile.sourceCodePath().string();
        };
        return ProjectData{getSourceCodePath};
    };

    d_pluginManager_p->callHooksGraphChanged(getSceneName, getVisibleEntities, getEdgeByQualifiedName, getProjectData);
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

        // object name setup by the codevisui.rc
        auto *menuView = menuBar()->findChild<QMenu *>("view");
        if (!menuView) {
            menuView = menuBar()->addMenu("View");
        }

        for (auto *dock : dockWidgets) {
            auto *action = new QAction();
            action->setText(dock->windowTitle());
            action->setCheckable(true);
            action->setChecked(dock->isVisible());
            connect(action, &QAction::toggled, dock, &QDockWidget::setVisible);
            menuView->addAction(action);
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
    changeCurrentGraphWidget(tabWidget->currentIndex());
}

void MainWindow::graphLoadStarted()
{
    // HACK: we are throwing two signals in sequence, hitting the assert.
    if (d_graphLoadRunning) {
        return;
    }

    disableWindow();

    QGuiApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    ui.topMessageWidget->clearActions();
#else
    const auto ourActions = ui.topMessageWidget->actions();
    for (auto *action : ourActions) {
        ui.topMessageWidget->removeAction(action);
    }
#endif

    ui.topMessageWidget->animatedHide();
}

void MainWindow::graphLoadFinished()
{
    QGuiApplication::restoreOverrideCursor();

    enableWindow();

    Q_EMIT databaseIdle();
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

void MainWindow::focusedGraphChanged(const QString& qualifiedName)
{
    m_currentQualifiedName = qualifiedName;

    const QString projectName =
        d_projectFile.location().empty() ? "Untitled" : QString::fromStdString(d_projectFile.location().string());

    setWindowTitle(qApp->applicationName() + " " + projectName + " " + qualifiedName);
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

        if (d_pluginManager_p) {
            d_parseCodebaseDialog_p->setPluginManager(*d_pluginManager_p);
        }
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
    d_projectFile.setSourceCodePath(d_parseCodebaseDialog_p->sourcePath());
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

void MainWindow::requestMenuPackageView(const QModelIndexList& multiSelection,
                                        const QModelIndex& clickedOn,
                                        const QPoint& pos)
{
    QMenu menu;
    QAction *act = menu.addAction(tr("Open in New Tab"));
    connect(act, &QAction::triggered, this, [this, multiSelection] {
        newTabRequested(multiSelection);
    });

    act = menu.addAction(tr("Load on Current Scene"));
    connect(act, &QAction::triggered, this, [this, multiSelection] {
        auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(currentGraphWidget->scene());
        for (const auto idx : multiSelection) {
            const QString qualName = idx.data(Codethink::lvtmdl::ModelRoles::e_QualifiedName).toString().toLocal8Bit();
            scene->loadEntityByQualifiedName(qualName, QPoint());
        }
        scene->reLayout();
    });

    const NodeType::Enum type = static_cast<NodeType::Enum>(clickedOn.data(ModelRoles::e_NodeType).toInt());
    if (type == NodeType::e_Package) {
        auto *node =
            sharedNodeStorage.findByQualifiedName(clickedOn.data(ModelRoles::e_QualifiedName).toString().toStdString());
        auto *pkgNode = dynamic_cast<Codethink::lvtldr::PackageNode *>(node);
        QString filePath = QString::fromStdString(pkgNode->dirPath());
        filePath.replace("${SOURCE_DIR}", QString::fromStdString(projectFile().sourceCodePath().string()));
        const QFileInfo fInfo(filePath);

        act = menu.addAction("Open Locally");
        if (!fInfo.exists()) {
            act->setToolTip(tr("Couldn't find folder for this package."));
            act->setEnabled(false);
        }

        connect(act, &QAction::triggered, this, [filePath] {
            const QUrl localFilePath = QUrl::fromLocalFile(filePath);
            QDesktopServices::openUrl(localFilePath);
        });
    }

    menu.exec(pos);
}

void MainWindow::requestMenuNamespaceView([[maybe_unused]] const QModelIndexList& multiSelection,
                                          const QModelIndex& clickedOn,
                                          const QPoint& pos)
{
    const NodeType::Enum type = static_cast<NodeType::Enum>(clickedOn.data(ModelRoles::e_NodeType).toInt());
    if (type != NodeType::e_Class) {
        return;
    }

    QMenu menu;
    QAction *act = menu.addAction(tr("Load on Empty Scene"));
    connect(act, &QAction::triggered, this, [this, clickedOn] {
        setCurrentGraph(clickedOn);
    });

    act = menu.addAction(tr("Load on Current Scene"));
    connect(act, &QAction::triggered, this, [this, clickedOn] {
        const QString qualName =
            clickedOn.data(Codethink::lvtmdl::ModelRoles::e_QualifiedName).toString().toLocal8Bit();
        auto *scene = qobject_cast<Codethink::lvtqtc::GraphicsScene *>(currentGraphWidget->scene());
        scene->loadEntityByQualifiedName(qualName, QPoint());
        scene->reLayout();
    });

    menu.exec(pos);
}

void MainWindow::bookmarksChanged()
{
    QList<QAction *> actions;
    for (const auto& bookmark : d_projectFile.bookmarks()) {
        auto *bookmarkAction = new QAction(bookmark);
        connect(bookmarkAction, &QAction::triggered, this, [this, bookmark] {
            QJsonDocument doc = d_projectFile.getBookmark(bookmark);
            currentGraphTab->loadBookmark(doc, Codethink::lvtshr::HistoryType::History);
        });

        actions.append(bookmarkAction);
    }

    unplugActionList("bookmark_actionlist");
    plugActionList("bookmark_actionlist", actions);
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
