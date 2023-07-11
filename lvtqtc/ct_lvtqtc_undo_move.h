// ct_lvtqtw_undo_move.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_MOVE_H
#define DEFINED_CT_LVTQTW_UNDO_MOVE_H

#include <lvtqtc_export.h>

#include <QUndoCommand>
#include <memory>
#include <string>

class QUndoCommand;
class QPointF;

namespace Codethink::lvtqtc {
class GraphicsScene;

class LVTQTC_EXPORT UndoMove : public QUndoCommand {
    // Provides undo and redo control for move operations.

  public:
    UndoMove(GraphicsScene *scene, const std::string& nodeId, const QPointF& origPos, const QPointF& newPos);
    ~UndoMove() override;

    void undo() override;
    void redo() override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
