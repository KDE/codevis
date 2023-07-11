// ct_lvtqtc_undo_notes.cpp                                                                            -*-C++-*-

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

#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_undo_notes.h>

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

struct UndoNotes::Private {
    std::string qualifiedName;
    lvtshr::DiagramType type;
    std::string oldNotes;
    std::string newNotes;
    GraphicsScene *scene;
};

UndoNotes::UndoNotes(std::string qualifiedName,
                     lvtshr::DiagramType type,
                     std::string oldNotes,
                     std::string newNotes,
                     GraphicsScene *scene):
    d(std::make_unique<Private>(
        Private{std::move(qualifiedName), type, std::move(oldNotes), std::move(newNotes), scene}))
{
    setText(QObject::tr("Set Notes on Entity"));
}

UndoNotes::~UndoNotes() = default;

void UndoNotes::redo()
{
    IGNORE_FIRST_CALL

    auto *thisEntity = d->scene->entityByQualifiedName(d->qualifiedName);
    if (thisEntity) {
        thisEntity->internalNode()->setNotes(d->newNotes);
    }
}

void UndoNotes::undo()
{
    auto *thisEntity = d->scene->entityByQualifiedName(d->qualifiedName);
    if (thisEntity) {
        thisEntity->internalNode()->setNotes(d->oldNotes);
    }
}
