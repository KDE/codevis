// ct_lvtqtc_undo_add_edge.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_ADD_RELATIONSHIP_H
#define DEFINED_CT_LVTQTW_UNDO_ADD_RELATIONSHIP_H

#include <lvtqtc_export.h>

#include <QUndoCommand>

#include <memory>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtqtc_undo_manager.h>
#include <ct_lvtqtc_util.h>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {
class GraphicsScene;

class LVTQTC_EXPORT UndoAddEdge : public QUndoCommand, public lvtqtc::UndoManager::IgnoreFirstRunMixin {
  public:
    UndoAddEdge(std::string fromQualifiedName,
                std::string toQualifiedName,
                lvtshr::LakosRelationType relationType,
                QtcUtil::UndoActionType undoActionType,
                lvtldr::NodeStorage& nodeStorage,
                QUndoCommand *parent = nullptr);
    ~UndoAddEdge() override;

    void undo() override;
    void redo() override;

    // definition of a private struct should be public so that
    // I can access via unnamed namespaces.
    struct Private;

  private:
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
