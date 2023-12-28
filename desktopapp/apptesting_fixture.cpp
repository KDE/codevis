// apptesting_fixture.cpp                                      -*-C++-*-

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

#include <ct_lvtmdl_packagetreemodel.h>
#include <kstandardaction.h>
#include <mainwindow.h>

#include <apptesting_fixture.h>

#include <utility>

#include <KActionCollection>
#include <QComboBox>

CodeVisApplicationTestFixture::CodeVisApplicationTestFixture()
{
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(desktopapp);

    // Resources must be initialized before mainwindow, so I can't initialize this on
    // the initialization list.
    mainWindow = new TestMainWindow(sharedNodeStorage, &undoManager);
    mainWindow->show();
}

bool CodeVisApplicationTestFixture::hasDefaultTabWidget() const
{
    return mainWindow->currentGraphTab != nullptr;
}

bool CodeVisApplicationTestFixture::isShowingWelcomePage() const
{
    return mainWindow->ui.welcomePage->isVisible();
}

bool CodeVisApplicationTestFixture::isShowingGraphPage() const
{
    return mainWindow->ui.graphPage->isVisible();
}

void CodeVisApplicationTestFixture::forceRelayout()
{
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();
    view->graphicsScene()->reLayout();
    QTest::qWait(500);
}

void CodeVisApplicationTestFixture::graphPressMoveRelease(QPoint source, QPoint destiny)
{
    auto *graphTab = qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget());

    QTest::mousePress(graphTab->graphicsView()->viewport(),
                      Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifier::NoModifier,
                      source);
    QTest::qWait(1000);
    QTest::mouseMove(graphTab->graphicsView()->viewport(), destiny);
    QTest::qWait(1000);
    QTest::mouseRelease(graphTab->graphicsView()->viewport(),
                        Qt::MouseButton::LeftButton,
                        Qt::KeyboardModifier::NoModifier,
                        destiny);
    QTest::qWait(1000);
}

void CodeVisApplicationTestFixture::clickOn(ClickableFeature const& feature)
{
    auto *graphTab = qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget());

    for (auto *tool : graphTab->tools()) {
        tool->setProperty("debug", true);
    }

    auto newPackageVisitor = [&](Sidebar::ManipulationTool::NewPackage const& info) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Package");
        auto *action = dynamic_cast<ToolAddPackage *>(graphTab->tools()[TOOL_ADD_PACKAGE_ID]);
        auto *inputDialog = action->inputDialog();
        inputDialog->overrideExec([inputDialog, info] {
            auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
            nameLineEdit->setText(info.name);
            return true;
        });
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto newComponentVisitor = [&](Sidebar::ManipulationTool::NewComponent const& info) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Component");
        auto *action = dynamic_cast<ToolAddComponent *>(graphTab->tools()[TOOL_ADD_COMPONENT_ID]);
        auto *inputDialog = action->inputDialog();
        inputDialog->overrideExec([inputDialog, info] {
            auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
            nameLineEdit->setText(info.name);
            return true;
        });
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto newLogicalEntityVisitor = [&](Sidebar::ManipulationTool::NewLogicalEntity const& info) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Logical Entity");
        auto *action = dynamic_cast<ToolAddLogicalEntity *>(graphTab->tools()[TOOL_ADD_LOGICAL_ENTITY_ID]);
        auto *inputDialog = action->inputDialog();
        inputDialog->overrideExec([inputDialog, info] {
            auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
            nameLineEdit->setText(info.name);
            auto *kindLineEdit = inputDialog->findChild<QComboBox *>("kind");
            kindLineEdit->setCurrentText(info.kind);
            return true;
        });
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto addDependencyVisitor = [&](Sidebar::ManipulationTool::AddDependency) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Physical Dependency");
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto addIsAVisitor = [&](Sidebar::ManipulationTool::AddIsA) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Is A");
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto resetZoomVisitor = [&](Sidebar::VisualizationTool::ResetZoom) {
        auto *button = graphTab->ui->toolBox->getButtonNamed("Reset Zoom");
        QTest::mouseClick(button, Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), QPoint(), DEFAULT_QT_DELAY);
    };

    auto newProjectVisitor = [&](Menubar::File::NewProject) {
        mainWindow->actionCollection()->action("file_new")->trigger();
    };

    auto closeProjectVisitor = [&](Menubar::File::CloseProject) {
        mainWindow->actionCollection()->action("file_close")->trigger();
    };

    auto currentGraphInfoVisitor = [&](CurrentGraph info) {
        QTest::mousePress(graphTab->graphicsView()->viewport(),
                          Qt::MouseButton::LeftButton,
                          Qt::KeyboardModifiers(),
                          {info.x, info.y});
        QTest::qWait(5);
        QTest::mouseRelease(graphTab->graphicsView()->viewport(),
                            Qt::MouseButton::LeftButton,
                            Qt::KeyboardModifiers(),
                            {info.x, info.y});
    };

    auto packageVisitor = [&](PackageTreeView::Package const& pkg) {
        auto *model = mainWindow->packageModel;
        auto *packageTreeView = mainWindow->ui.packagesTree;

        // look for the item on the model.:
        QModelIndex idx;
        for (int i = 0; i < model->invisibleRootItem()->rowCount(); i++) {
            auto *item = model->invisibleRootItem()->child(i, 0);
            if (item->data(Codethink::lvtmdl::ModelRoles::e_QualifiedName).toString()
                == QString::fromStdString(pkg.name)) {
                idx = item->index();
            }
        }
        assert(idx.isValid());
        Q_EMIT packageTreeView->branchSelected(idx);
    };

    /* The Std::visit spec for the call:
    std::variant<Sidebar::ManipulationTool::NewPackage,
        Sidebar::ManipulationTool::NewComponent,
        Sidebar::ManipulationTool::NewLogicalEntity,
        Sidebar::ManipulationTool::AddDependency,
        Sidebar::ManipulationTool::AddIsA,
        Sidebar::VisualizationTool::ResetZoom,
        Menubar::File::NewProject,
        Menubar::File::CloseProject,
        PackageTreeView::Package,
        CurrentGraph
    >;
    */
    std::visit(
        visitor{
            newPackageVisitor,
            newComponentVisitor,
            newLogicalEntityVisitor,
            addDependencyVisitor,
            addIsAVisitor,
            resetZoomVisitor,
            newProjectVisitor,
            closeProjectVisitor,
            packageVisitor,
            currentGraphInfoVisitor,
        },
        feature);
}

void CodeVisApplicationTestFixture::ctrlZ()
{
    QTest::keySequence(mainWindow, QKeySequence(Qt::CTRL | Qt::Key_Z));
    QTest::qWait(100);
}

void CodeVisApplicationTestFixture::ctrlShiftZ()
{
#if defined(Q_OS_WINDOWS)
    QTest::keySequence(mainWindow, QKeySequence(Qt::CTRL | Qt::Key_Y));
#else
    QTest::keySequence(mainWindow, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
#endif
    QTest::qWait(100);
}

bool CodeVisApplicationTestFixture::isAnyToolSelected()
{
    auto *tool =
        qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget())->graphicsView()->currentTool();
    qDebug() << "The current tool is:" << (tool ? tool->name() : nullptr) << "View ptr: "
             << qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget())->graphicsView();
    return tool != nullptr;
}

LakosEntity *CodeVisApplicationTestFixture::findElement(const std::string& qualifiedName)
{
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();
    auto *entity = view->graphicsScene()->entityByQualifiedName(qualifiedName);

    assert(entity);
    return entity;
}

QPoint CodeVisApplicationTestFixture::findElementTopLeftPosition(const std::string& qname)
{
    auto *entity = findElement(qname);
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow->currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();

    const auto offset = QPointF(10, 10);
    const auto p = entity->scenePos() + offset;
    return view->mapFromScene(p);
}
