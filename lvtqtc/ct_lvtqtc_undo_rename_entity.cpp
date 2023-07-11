// ct_lvtqtc_undo_rename_entity.cpp                                                                            -*-C++-*-

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

#include <ct_lvtqtc_undo_rename_entity.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_isa.h>
#include <ct_lvtshr_graphenums.h>

#include <optional>

#include <QPointF>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink;

struct UndoRenameEntity::Private {
    std::string qualifiedName;
    std::string oldQualifiedName;
    lvtshr::DiagramType type;
    std::string oldName;
    std::string newName;
    NodeStorage& nodeStorage;
};

UndoRenameEntity::UndoRenameEntity(std::string qualifiedName,
                                   std::string oldQualifiedName,
                                   lvtshr::DiagramType type,
                                   std::string oldName,
                                   std::string newName,
                                   NodeStorage& nodeStorage):
    d(std::make_unique<Private>(Private{std::move(qualifiedName),
                                        std::move(oldQualifiedName),
                                        type,
                                        std::move(oldName),
                                        std::move(newName),
                                        nodeStorage}))
{
    setText(QObject::tr("Rename entity"));
}

UndoRenameEntity::~UndoRenameEntity() = default;

void UndoRenameEntity::redo()
{
    IGNORE_FIRST_CALL

    auto *node = d->nodeStorage.findByQualifiedName(d->type, d->oldQualifiedName);
    node->setName(d->newName);
}

void UndoRenameEntity::undo()
{
    auto *node = d->nodeStorage.findByQualifiedName(d->type, d->qualifiedName);
    node->setName(d->oldName);
}
