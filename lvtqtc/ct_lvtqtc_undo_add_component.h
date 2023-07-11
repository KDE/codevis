// ct_lvtqtw_undo_add_entity.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_ADD_COMPONENT_H
#define DEFINED_CT_LVTQTW_UNDO_ADD_COMPONENT_H

#include <lvtqtc_export.h>

#include <QUndoCommand>

#include <memory>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtqtc_undo_add_entity_common.h>
#include <ct_lvtqtc_util.h>

namespace Codethink::lvtldr {
class NodeStorage;
class LakosianNode;
} // namespace Codethink::lvtldr

namespace Codethink::lvtqtc {
class GraphicsScene;
class LakosEntity;

class LVTQTC_EXPORT UndoAddComponent : public UndoAddEntityBase {
  public:
    UndoAddComponent(GraphicsScene *scene,
                     const QPointF& pos,
                     const std::string& name,
                     const std::string& qualifiedName,
                     const std::string& parentQualifiedName,
                     QtcUtil::UndoActionType undoActionType,
                     lvtldr::NodeStorage& storage);

    ~UndoAddComponent() override;

  private:
    void addEntity() override;
    void removeEntity() override;
};

} // namespace Codethink::lvtqtc

#endif
