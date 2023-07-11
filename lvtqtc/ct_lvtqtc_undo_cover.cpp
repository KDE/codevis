// ct_lvtqtc_undo_cover.cpp                                       -*-C++-*-

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
#include <ct_lvtqtc_undo_cover.h>

#include <QPointF>

using namespace Codethink::lvtqtc;

struct UndoCover::Private {
    GraphicsScene *scene;
    std::string nodeId;
};

UndoCover::UndoCover(GraphicsScene *scene, const std::string& nodeId):
    d(std::make_unique<UndoCover::Private>(Private{scene, nodeId}))
{
    setText(QObject::tr("Toggle Cover on %1").arg(QString::fromStdString(nodeId)));
}

UndoCover::~UndoCover() = default;

namespace {

void toggleEntity(QGraphicsItem *item)
{
    auto *thisEntity = qgraphicsitem_cast<LakosEntity *>(item);
    if (thisEntity) {
        thisEntity->toggleCover(LakosEntity::ToggleContentBehavior::Single, QtcUtil::CreateUndoAction::e_No);
    }
}

} // namespace

void UndoCover::undo()
{
    toggleEntity(d->scene->entityById(d->nodeId));
}

void UndoCover::redo()
{
    toggleEntity(d->scene->entityById(d->nodeId));
}
