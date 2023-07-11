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
#include <mainwindow.h>

#include <apptesting_fixture.h>

#include <utility>

#include <QComboBox>

CodeVisApplicationTestFixture::CodeVisApplicationTestFixture(): mainWindow{sharedNodeStorage, &undoManager}
{
    Q_INIT_RESOURCE(resources);
    mainWindow.show();
}

bool CodeVisApplicationTestFixture::hasDefaultTabWidget() const
{
    return mainWindow.currentGraphTab != nullptr;
}

bool CodeVisApplicationTestFixture::isShowingWelcomePage() const
{
    return mainWindow.ui.welcomePage->isVisible();
}

bool CodeVisApplicationTestFixture::isShowingGraphPage() const
{
    return mainWindow.ui.graphPage->isVisible();
}

void CodeVisApplicationTestFixture::forceRelayout()
{
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow.currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();
    view->graphicsScene()->relayout();
    QTest::qWait(500);
}

void CodeVisApplicationTestFixture::clickOn(ClickableFeature const& feature)
{
    auto *graphTab = qobject_cast<GraphTabElement *>(mainWindow.currentGraphTab->currentWidget());

    for (auto *tool : graphTab->tools()) {
        tool->setProperty("debug", true);
    }

    std::visit(
        visitor{[&](Sidebar::ManipulationTool::NewPackage const& info) {
                    auto *button = graphTab->ui->toolBox->getButtonNamed("Package");
                    auto *action = dynamic_cast<ToolAddPackage *>(graphTab->tools()[TOOL_ADD_PACKAGE_ID]);
                    auto *inputDialog = action->inputDialog();
                    inputDialog->overrideExec([inputDialog, info] {
                        auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
                        nameLineEdit->setText(info.name);
                        return true;
                    });
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::ManipulationTool::NewComponent const& info) {
                    auto *button = graphTab->ui->toolBox->getButtonNamed("Component");
                    auto *action = dynamic_cast<ToolAddComponent *>(graphTab->tools()[TOOL_ADD_COMPONENT_ID]);
                    auto *inputDialog = action->inputDialog();
                    inputDialog->overrideExec([inputDialog, info] {
                        auto *nameLineEdit = inputDialog->findChild<QLineEdit *>("name");
                        nameLineEdit->setText(info.name);
                        return true;
                    });
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::ManipulationTool::NewLogicalEntity const& info) {
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
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::ManipulationTool::AddDependency) {
                    auto *button = graphTab->ui->toolBox->getButtonNamed("Physical Dependency");
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::ManipulationTool::AddIsA) {
                    auto *button = graphTab->ui->toolBox->getButtonNamed("Is A");
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::VisualizationTool::ResetZoom) {
                    auto *button = graphTab->ui->toolBox->getButtonNamed("Reset Zoom");
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Sidebar::ToggleApplicationMode) {
                    auto *scene = qobject_cast<GraphicsScene *>(graphTab->graphicsView()->scene());
                    const auto *const btnText = (scene->isEditMode() ? "Edit" : "View");
                    auto *button = graphTab->ui->toolBox->getButtonNamed(btnText);
                    QTest::mouseClick(button,
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      QPoint(),
                                      DEFAULT_QT_DELAY);
                },
                [&](Menubar::File::NewProject) {
                    mainWindow.ui.actionNew_Project->activate(QAction::Trigger);
                },
                [&](Menubar::File::CloseProject) {
                    mainWindow.ui.actionClose_Project->activate(QAction::Trigger);
                },
                [&](CurrentGraph info) {
                    QTest::mousePress(graphTab->graphicsView()->viewport(),
                                      Qt::MouseButton::LeftButton,
                                      Qt::KeyboardModifiers(),
                                      {info.x, info.y});
                    QTest::qWait(5);
                    QTest::mouseRelease(graphTab->graphicsView()->viewport(),
                                        Qt::MouseButton::LeftButton,
                                        Qt::KeyboardModifiers(),
                                        {info.x, info.y});
                },
                [&](PackageTreeView::Package const& pkg) {
                    auto *model = mainWindow.packageModel;
                    auto *packageTreeView = mainWindow.ui.packagesTree;

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
                }},
        feature);
}

void CodeVisApplicationTestFixture::ctrlZ()
{
    QTest::keySequence(&mainWindow, QKeySequence(Qt::CTRL + Qt::Key_Z));
    QTest::qWait(100);
}

void CodeVisApplicationTestFixture::ctrlShiftZ()
{
#if defined(Q_OS_WINDOWS)
    QTest::keySequence(&mainWindow, QKeySequence(Qt::CTRL + Qt::Key_Y));
#else
    QTest::keySequence(&mainWindow, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
#endif
    QTest::qWait(100);
}

bool CodeVisApplicationTestFixture::isAnyToolSelected()
{
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow.currentGraphTab->currentWidget());
    return currentTab->graphicsView()->currentTool() != nullptr;
}

LakosEntity *CodeVisApplicationTestFixture::findElement(const std::string& qualifiedName)
{
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow.currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();
    auto *entity = view->graphicsScene()->entityByQualifiedName(qualifiedName);

    assert(entity);
    return entity;
}

QPoint CodeVisApplicationTestFixture::findElementTopLeftPosition(const std::string& qname)
{
    auto *entity = findElement(qname);
    auto *currentTab = qobject_cast<GraphTabElement *>(mainWindow.currentGraphTab->currentWidget());
    auto *view = currentTab->graphicsView();

    const auto offset = QPointF(5, 5);
    const auto p = entity->scenePos() + offset;
    return view->mapFromScene(p);
}
