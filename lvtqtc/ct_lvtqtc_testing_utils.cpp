// ct_lvtqtc_testing_utils.cpp                                                                                 -*-C++-*-

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

#include <ct_lvtqtc_testing_utils.h>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

namespace Codethink::lvtqtc {

GraphicsViewWrapperForTesting::GraphicsViewWrapperForTesting(NodeStorage& nodeStorage):
    GraphicsView(nodeStorage, projectFileForTesting)
{
    setColorManagement(std::make_shared<ColorManagement>(false));
}

QPoint GraphicsViewWrapperForTesting::getEntityPosition(UniqueId uid) const
{
    auto result = QPoint{0, 0};
    findEntityAndRun(uid, [&](LakosEntity& entity) {
        result = mapFromScene(entity.scenePos());
    });
    return result;
}

void GraphicsViewWrapperForTesting::moveEntityTo(UniqueId uid, QPointF newPos) const
{
    findEntityAndRun(uid, [&](LakosEntity& entity) {
        entity.setPos(newPos);
    });
}

bool GraphicsViewWrapperForTesting::hasEntityWithId(lvtshr::UniqueId uid) const
{
    bool found = false;
    findEntityAndRun(
        uid,
        [&](LakosEntity& entity) {
            found = true;
        },
        false);
    return found;
}

bool GraphicsViewWrapperForTesting::hasRelationWithId(lvtshr::UniqueId uidFrom, lvtshr::UniqueId uidTo) const
{
    bool found = false;
    findRelationAndRun(
        uidFrom,
        uidTo,
        [&](LakosRelation& entity) {
            found = true;
        },
        false);
    return found;
}

int GraphicsViewWrapperForTesting::countEntityChildren(UniqueId uid) const
{
    int size = 0;
    findEntityAndRun(uid, [&](LakosEntity const& entity) {
        size = entity.lakosEntities().size();
    });
    return size;
}

void GraphicsViewWrapperForTesting::findEntityAndRun(UniqueId uid,
                                                     std::function<void(LakosEntity&)> const& f,
                                                     bool assertOnNotFound) const
{
    for (const auto& item : items()) { // clazy:exclude=range-loop,range-loop-detach
        if (auto *entity = qgraphicsitem_cast<LakosEntity *>(item)) {
            if (entity->uniqueId() == uid) {
                f(*entity);
                return;
            }
        }
    }
    if (assertOnNotFound) {
        assert(false && "Couldn't find entity");
    }
}

void GraphicsViewWrapperForTesting::findRelationAndRun(UniqueId fromUid,
                                                       UniqueId toUid,
                                                       std::function<void(LakosRelation&)> const& f,
                                                       bool assertOnNotFound) const
{
    for (const auto& item : items()) { // clazy:exclude=range-loop,range-loop-detach
        if (auto *relation = qgraphicsitem_cast<LakosRelation *>(item)) {
            if (relation->from()->uniqueId() == fromUid && relation->to()->uniqueId() == toUid) {
                f(*relation);
                return;
            }
        }
    }
    if (assertOnNotFound) {
        assert(false && "Couldn't find relation");
    }
}

bool mousePressAt(ITool& tool, QPointF pos)
{
    auto e = QMouseEvent{QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier};
    tool.mousePressEvent(&e);
    return e.isAccepted();
}

bool mouseMoveTo(ITool& tool, QPointF pos)
{
    auto e = QMouseEvent{QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier};
    tool.mouseMoveEvent(&e);
    return e.isAccepted();
}

bool mouseReleaseAt(ITool& tool, QPointF pos)
{
    auto e = QMouseEvent{QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier};
    tool.mouseReleaseEvent(&e);
    return e.isAccepted();
}

} // namespace Codethink::lvtqtc
