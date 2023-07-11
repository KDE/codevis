// ct_lvtqtc_undo_load_children.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_LOAD_ENTITY_H
#define DEFINED_CT_LVTQTW_UNDO_LOAD_ENTITY_H

#include <lvtqtc_export.h>

#include <QUndoCommand>

#include <memory>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtqtc_undo_manager.h>
#include <ct_lvtqtc_util.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {
class GraphicsScene;

class LVTQTC_EXPORT UndoLoadEntity : public QUndoCommand {
    // This class handles loading and unloading of single entities
    // and children entities. The other loads are more complex and not easy
    // to do in a generic way, so they don't have an undo command yet.
  public:
    UndoLoadEntity(GraphicsScene *scene,
                   lvtshr::UniqueId entity_id,
                   GraphicsScene::UnloadDepth type,
                   QtcUtil::UndoActionType undoType,
                   QUndoCommand *parent = nullptr);

    ~UndoLoadEntity() override;

    void undo() override;
    void redo() override;

    struct Private;

  private:
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
