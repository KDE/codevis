// ct_lvtqtc_undo_manager.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_undo_manager.h>

#include <QDockWidget>
#include <QMainWindow>
#include <QUndoCommand>
#include <QUndoView>

#include <QDebug>
#include <QGraphicsView>
#include <QObject>

#include <unordered_map>

using namespace Codethink::lvtqtc;

struct UndoManager::Private {
    QUndoStack undoStack;
    QWidget *debugWidget = nullptr;
    QDockWidget *dock = nullptr;
};

UndoManager::UndoManager(): d(std::make_unique<UndoManager::Private>())
{
    d->undoStack.clear();
    d->undoStack.setUndoLimit(1000);
}

UndoManager::~UndoManager() = default;

void UndoManager::undoGroupRequested(const QString& groupName)
{
    d->undoStack.beginMacro(groupName);
}

void UndoManager::addUndoCommand(QUndoCommand *command)
{
    qDebug() << "Registering command" << command->actionText() << "on the undo stack";
    d->undoStack.push(command);
}

void UndoManager::undoGroupFinished()
{
    d->undoStack.endMacro();
}

void UndoManager::undo()
{
    if (d->undoStack.canUndo()) {
        const auto *command = d->undoStack.command(d->undoStack.index() - 1);
        Q_EMIT onBeforeUndo(command);
        qDebug() << "Undoing" << command->actionText();
        d->undoStack.undo();
    }
}

void UndoManager::redo()
{
    if (d->undoStack.canRedo()) {
        const auto *command = d->undoStack.command(d->undoStack.index());
        Q_EMIT onBeforeRedo(command);
        qDebug() << "Redoing" << command->actionText();
        d->undoStack.redo();
    }
}

void UndoManager::clear()
{
    d->undoStack.clear();
}

void UndoManager::createDock(QMainWindow *mainWindow)
{
    d->dock = new QDockWidget();
    d->dock->setWindowTitle("Undo Stack");
    d->dock->setObjectName("UndoStack");
    mainWindow->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, d->dock);
    auto *stackView = new QUndoView(&d->undoStack);
    d->dock->setWidget(stackView);
    d->debugWidget = stackView;
    d->dock->setVisible(false);
}
