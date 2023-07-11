// ct_lvtqtc_undo_add_entity_common.cpp                               -*-C++-*-

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

#include <ct_lvtqtc_undo_add_entity_common.h>

namespace Codethink::lvtqtc {

UndoAddEntityBase::UndoAddEntityBase(GraphicsScene *scene,
                                     const QPointF& pos,
                                     const std::string& name,
                                     const std::string& qualifiedName,
                                     const std::string& parentQualifiedName,
                                     QtcUtil::UndoActionType undoActionType,
                                     lvtldr::NodeStorage& storage):
    m_scene(scene->observer_from_this()),
    m_undoActionType(undoActionType),
    m_addInfo{pos, name, qualifiedName, parentQualifiedName, std::string{}},
    m_removeInfo{nullptr},
    m_storage(storage)
{
}

void UndoAddEntityBase::undo()
{
    switch (m_undoActionType) {
    case QtcUtil::UndoActionType::e_Add:
        removeEntity();
        break;
    case QtcUtil::UndoActionType::e_Remove:
        addEntity();
        break;
    }
}

void UndoAddEntityBase::redo()
{
    IGNORE_FIRST_CALL

    switch (m_undoActionType) {
    case QtcUtil::UndoActionType::e_Add:
        addEntity();
        break;
    case QtcUtil::UndoActionType::e_Remove:
        removeEntity();
        break;
    }
}
} // namespace Codethink::lvtqtc
