// ct_lvtqtc_undo_load_children.cpp                               -*-C++-*-

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

#include <ct_lvtqtc_undo_load_entity.h>

using namespace Codethink::lvtqtc;

struct UndoLoadEntity::Private {
    GraphicsScene *scene;
    lvtshr::UniqueId uuid;
    GraphicsScene::UnloadDepth type;
    QtcUtil::UndoActionType undoActionType;
    Private(GraphicsScene *scene,
            lvtshr::UniqueId uuid,
            GraphicsScene::UnloadDepth type,
            QtcUtil::UndoActionType undoActionType):
        scene(scene), uuid(uuid), type(type), undoActionType(undoActionType)
    {
    }
};

UndoLoadEntity::UndoLoadEntity(GraphicsScene *scene,
                               lvtshr::UniqueId entity_id,
                               GraphicsScene::UnloadDepth type,
                               QtcUtil::UndoActionType undoType,
                               QUndoCommand *parent):
    QUndoCommand(
        QObject::tr(undoType == QtcUtil::UndoActionType::e_Add
                        ? (type == GraphicsScene::UnloadDepth::Entity ? "Load Entity" : "Load Entity Children")
                        : (type == GraphicsScene::UnloadDepth::Entity ? "Unload Entity" : "Load Entity Children")),
        parent),
    d(std::make_unique<UndoLoadEntity::Private>(scene, entity_id, type, undoType))
{
}

UndoLoadEntity::~UndoLoadEntity() = default;

void UndoLoadEntity::undo()
{
    if (d->undoActionType == QtcUtil::UndoActionType::e_Add) {
        d->scene->unloadEntity(d->uuid, d->type);
    } else {
        d->scene->loadEntity(d->uuid, d->type);
    }
}

void UndoLoadEntity::redo()
{
    if (d->undoActionType == QtcUtil::UndoActionType::e_Add) {
        d->scene->loadEntity(d->uuid, d->type);
    } else {
        d->scene->unloadEntity(d->uuid, d->type);
    }
}
