// ct_lvtqtc_undo_expand.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_undo_expand.h>
#include <ct_lvtqtc_util.h>

#include <QPointF>

using namespace Codethink::lvtqtc;

struct UndoExpand::Private {
    GraphicsScene *scene;
    std::string nodeId;
    QPointF newPosition;
    QPointF originalPosition;
    LakosEntity::RelayoutBehavior behavior;
};

UndoExpand::UndoExpand(GraphicsScene *scene,
                       const std::string& nodeId,
                       std::optional<QPointF> moveToPosition,
                       LakosEntity::RelayoutBehavior behavior):
    d(std::make_unique<UndoExpand::Private>(Private{scene,
                                                    nodeId,
                                                    moveToPosition ? *moveToPosition : scene->entityById(nodeId)->pos(),
                                                    scene->entityById(nodeId)->pos(),
                                                    behavior}))
{
    setText(QObject::tr("Toggle Expand %1").arg(QString::fromStdString(nodeId)));
}

UndoExpand::~UndoExpand() = default;

namespace {

void toggleEntity(LakosEntity *item, QPointF moveToPosition, LakosEntity::RelayoutBehavior behavior)
{
    if (item == nullptr) {
        return;
    }
    item->toggleExpansion(QtcUtil::CreateUndoAction::e_No, moveToPosition, behavior);
}

} // namespace

void UndoExpand::undo()
{
    toggleEntity(d->scene->entityById(d->nodeId), d->originalPosition, d->behavior);
}

void UndoExpand::redo()
{
    toggleEntity(d->scene->entityById(d->nodeId), d->newPosition, d->behavior);
}
