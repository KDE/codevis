// ct_lvtqtw_undo_expand.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_EXPAND_H
#define DEFINED_CT_LVTQTW_UNDO_EXPAND_H

#include <lvtqtc_export.h>

#include <ct_lvtqtc_lakosentity.h>

#include <QPointF>
#include <QUndoCommand>

#include <memory>
#include <optional>

namespace Codethink::lvtqtc {
class GraphicsScene;

class LVTQTC_EXPORT UndoExpand : public QUndoCommand {
    // Provides undo and redo control for expand operations.

  public:
    UndoExpand(GraphicsScene *scene,
               const std::string& nodeId,
               std::optional<QPointF> moveToPosition = std::nullopt,
               LakosEntity::RelayoutBehavior behavior = LakosEntity::RelayoutBehavior::e_RequestRelayout);
    ~UndoExpand() override;

    void undo() override;
    void redo() override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
