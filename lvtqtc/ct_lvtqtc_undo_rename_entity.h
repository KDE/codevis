// ct_lvtqtw_undo_rename_entity.h                                                                              -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_UNDO_RENAME_ENTITY_H
#define DEFINED_CT_LVTQTW_UNDO_RENAME_ENTITY_H

#include <lvtqtc_export.h>

#include <ct_lvtshr_graphenums.h>

#include <QUndoCommand>
#include <ct_lvtqtc_undo_manager.h>
#include <memory>
#include <string>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT UndoRenameEntity : public QUndoCommand, public lvtqtc::UndoManager::IgnoreFirstRunMixin {
  public:
    UndoRenameEntity(std::string qualifiedName,
                     std::string oldQualifiedName,
                     lvtshr::DiagramType type,
                     std::string oldName,
                     std::string newName,
                     lvtldr::NodeStorage& nodeStorage);
    ~UndoRenameEntity() override;

    void undo() override;
    void redo() override;

    struct Private;

  private:
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
