// ct_lvtqtc_undo_reparent_entity.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_undo_reparent_entity.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_isa.h>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtqtc;

struct UndoReparentEntity::Private {
    NodeStorage& nodeStorage;
    LakosianNode *entity = nullptr;
    LakosianNode *oldParent = nullptr;
    LakosianNode *newParent = nullptr;
    std::string oldName;
    std::string newName;
};

UndoReparentEntity::UndoReparentEntity(NodeStorage& nodeStorage,
                                       LakosianNode *entity,
                                       LakosianNode *oldParent,
                                       LakosianNode *newParent,
                                       std::string const& oldName,
                                       std::string const& newName):
    d(std::make_unique<UndoReparentEntity::Private>(
        Private{nodeStorage, entity, oldParent, newParent, oldName, newName}))
{
    setText(QObject::tr("Reparent Entity"));
}

UndoReparentEntity::~UndoReparentEntity() = default;

void UndoReparentEntity::undo()
{
    d->nodeStorage.reparentEntity(d->entity, d->oldParent).expect("Unexpected error on undo/redo");
    d->entity->setName(d->oldName);
}

void UndoReparentEntity::redo()
{
    // Qt calls redo() when pushing the UndoCommand for the first time, but we do not want it to be run, as the first
    // run can lead to failures.
    if (isFirstRun) {
        isFirstRun = false;
        return;
    }

    d->nodeStorage.reparentEntity(d->entity, d->newParent).expect("Unexpected error on undo/redo");
    d->entity->setName(d->newName);
}
