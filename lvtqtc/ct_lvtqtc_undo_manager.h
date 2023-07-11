// ct_lvtqtw_undo_manager.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_MANAGER_H
#define DEFINED_CT_LVTQTW_UNDO_MANAGER_H

#include <lvtqtc_export.h>

#include <QObject>
#include <memory>

class QUndoCommand;
class QUndoStack;
class QMainWindow;

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT UndoManager : public QObject {
    // manages a collection of views and their undo / redo stack.
    Q_OBJECT
  public:
    class LVTQTC_EXPORT IgnoreFirstRunMixin {
        // Mixin to be used with the UndoManager with helper method to ignore the first run.
        // Qt calls redo() when pushing the UndoCommand for the first time, but we do not want it to be run, as the
        // first run can lead to failures. To use this class, simply extend the undoCommand with IgnoreFirstRunMixin
        // and write 'IGNORE_FIRST_CALL' as the first command of your redo() method.

      public:
#define IGNORE_FIRST_CALL                                                                                              \
    if (isFirstRun) {                                                                                                  \
        isFirstRun = false;                                                                                            \
        return;                                                                                                        \
    }

      protected:
        bool isFirstRun = true;
    };

    UndoManager();
    ~UndoManager() override;

    void undoGroupRequested(const QString& groupName);
    void addUndoCommand(QUndoCommand *command);
    void undoGroupFinished();
    void undo();
    void redo();
    void clear();

    void createDock(QMainWindow *mainWindow);

    Q_SIGNAL void onBeforeUndo(const QUndoCommand *);
    Q_SIGNAL void onBeforeRedo(const QUndoCommand *);

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
