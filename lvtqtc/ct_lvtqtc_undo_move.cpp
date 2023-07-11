// ct_lvtqtc_undo_move.cpp                                       -*-C++-*-

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
#include <ct_lvtqtc_undo_move.h>
#include <oup/observable_unique_ptr.hpp>

#include <QPointF>

using namespace Codethink::lvtqtc;

struct UndoMove::Private {
    oup::observer_ptr<GraphicsScene> scene;
    std::string nodeQualifiedName;
    QPointF originPos;
    QPointF newPos;
};

UndoMove::UndoMove(GraphicsScene *scene, const std::string& nodeId, const QPointF& origPos, const QPointF& newPos):
    d(std::make_unique<UndoMove::Private>(Private{scene->observer_from_this(), nodeId, origPos, newPos}))
{
    setText(QObject::tr("Undo move"));
}

UndoMove::~UndoMove() = default;

namespace {
void updateScene(LakosEntity *entity)
{
    // redraw all edges on the current position.
    entity->recursiveEdgeRelayout();

    // triggers a recalculation of the parent's boundaries.
    Q_EMIT entity->moving();

    // Tells the system tha the graph updated.
    Q_EMIT entity->graphUpdate();
}
} // namespace

void UndoMove::undo()
{
    if (d->scene == nullptr) {
        return;
    }

    auto *thisEntity = d->scene->entityByQualifiedName(d->nodeQualifiedName);
    if (thisEntity) {
        thisEntity->setPos(d->originPos);
        updateScene(thisEntity);
    }
}

void UndoMove::redo()
{
    if (d->scene == nullptr) {
        return;
    }

    auto *thisEntity = d->scene->entityByQualifiedName(d->nodeQualifiedName);
    if (thisEntity) {
        thisEntity->setPos(d->newPos);
        updateScene(thisEntity);
    }
}
