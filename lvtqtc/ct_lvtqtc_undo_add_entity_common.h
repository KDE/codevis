// ct_lvtqtc_undo_add_entity_common.h                               -*-C++-*-

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
#ifndef CT_LVTQTC_UNDO_ADD_ENTITY_COMMON_H
#define CT_LVTQTC_UNDO_ADD_ENTITY_COMMON_H

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_util.h>

#include <QPointF>
#include <QUndoCommand>

#include <ct_lvtqtc_undo_manager.h>
#include <lvtqtc_export.h>
#include <string>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT UndoAddEntityBase : public QUndoCommand, public lvtqtc::UndoManager::IgnoreFirstRunMixin {
  public:
    UndoAddEntityBase(GraphicsScene *scene,
                      const QPointF& pos,
                      const std::string& name,
                      const std::string& qualifiedName,
                      const std::string& parentQualifiedName,
                      QtcUtil::UndoActionType undoActionType,
                      lvtldr::NodeStorage& storage);

    // constructor for Add actions.
    void undo() override;
    void redo() override;

  protected:
    virtual void addEntity() = 0;
    virtual void removeEntity() = 0;

    oup::observer_ptr<GraphicsScene> m_scene;
    QtcUtil::UndoActionType m_undoActionType = QtcUtil::UndoActionType::e_Add;

    struct {
        QPointF pos;
        std::string name;
        std::string qualifiedName;
        // uniqueid does not work here because it changes between add / removal of the db.
        std::string parentQualifiedName;
        std::string currentNotes;
    } m_addInfo;

    struct {
        lvtldr::LakosianNode *nodeToRemove;
    } m_removeInfo;

    lvtldr::NodeStorage& m_storage;
};
} // namespace Codethink::lvtqtc
#endif
