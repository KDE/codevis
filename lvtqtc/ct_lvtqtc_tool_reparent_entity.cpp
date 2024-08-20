// ct_lvtqtw_tool_reparent_entity.cpp                                                                          -*-C++-*-

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

#include <ct_lvtqtc_tool_reparent_entity.h>

#include <ct_lvtqtc_componententity.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_undo_reparent_entity.h>

#include <ct_lvtqtc_util.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtshr_functional.h>

#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <preferences.h>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtshr;

namespace Codethink::lvtqtc {

struct ToolReparentEntity::Private {
    enum class ToolReparentEntityState { Browsing, MovingEntity, NoEntitySelected, Finished };

    NodeStorage& nodeStorage;
    QCursor currentCursor;
    LakosEntity *currentItem = nullptr;
    LakosEntity *targetItem = nullptr;
    QGraphicsItem *originalCurrentItemParent = nullptr;
    qreal originalCurrentItemZValue = 0;
    ToolReparentEntityState state = ToolReparentEntityState::Browsing;
    QPointF originalCurrentItemPos = {0.0, 0.0};
};

ToolReparentEntity::ToolReparentEntity(GraphicsView *gv, NodeStorage& nodeStorage):
    ITool(tr("Reparent entity"),
          tr("Moves an entity from one parent to another"),
          IconHelpers::iconFrom(":/icons/reparent_entity"),
          gv),
    d(std::make_unique<Private>(Private{nodeStorage, Qt::OpenHandCursor}))
{
}

ToolReparentEntity::~ToolReparentEntity() = default;

void ToolReparentEntity::mousePressEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Press Event";

    switch (d->state) {
    case Private::ToolReparentEntityState::Browsing: {
        if (d->currentItem == nullptr) {
            d->state = Private::ToolReparentEntityState::NoEntitySelected;
            graphicsView()->setCursor(Qt::ForbiddenCursor);
            return;
        }
        d->state = Private::ToolReparentEntityState::MovingEntity;

        d->originalCurrentItemParent = d->currentItem->parentItem();
        d->currentItem->setParentItem(nullptr);

        d->originalCurrentItemPos = d->currentItem->pos();
        graphicsView()->setCursor(Qt::ClosedHandCursor);
        return;
    }
    case Private::ToolReparentEntityState::MovingEntity: {
        assert(false && "Unexpected state");
    }
    case Private::ToolReparentEntityState::NoEntitySelected: {
        assert(false && "Unexpected state");
    }
    case Private::ToolReparentEntityState::Finished: {
        assert(false && "Unexpected state");
    }
    }
}

void ToolReparentEntity::mouseMoveEvent(QMouseEvent *event)
{
    switch (d->state) {
    case Private::ToolReparentEntityState::Browsing: {
        graphicsView()->setCursor(Qt::OpenHandCursor);
        auto extractComponentFromMousePosition = [&]() -> LakosEntity * {
            const auto qItems = graphicsView()->itemsByType<LakosEntity>(event->pos());
            for (auto const& item : qItems) {
                if (item->instanceType() == DiagramType::ComponentType) {
                    return item;
                }
            }
            return nullptr;
        };

        auto *item = extractComponentFromMousePosition();
        if (item != d->currentItem) {
            updateCurrentItemTo(item);
        }
        return;
    }
    case Private::ToolReparentEntityState::MovingEntity: {
        assert(d->currentItem && "Unexpected empty item with moving state");

        d->currentItem->setPos(graphicsView()->mapToScene(event->pos()));

        auto extractPackageFromMousePosition = [&]() -> LakosEntity * {
            const auto qItems = graphicsView()->itemsByType<LakosEntity>(event->pos());
            for (auto const& item : qItems) {
                if (item->instanceType() == DiagramType::PackageType) {
                    return item;
                }
            }
            return nullptr;
        };

        auto *item = extractPackageFromMousePosition();
        if (item != d->targetItem) {
            updateTargetItemTo(item);
        }
        return;
    }
    case Private::ToolReparentEntityState::NoEntitySelected: {
        // Noop
        return;
    }
    case Private::ToolReparentEntityState::Finished: {
        assert(false && "Unexpected state");
    }
    }
}

void ToolReparentEntity::mouseReleaseEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Release Event";

    using Codethink::lvtshr::ScopeExit;
    ScopeExit _([&]() {
        deactivate();
    });

    switch (d->state) {
    case Private::ToolReparentEntityState::Browsing: {
        assert(false && "Unexpected state");
    }
    case Private::ToolReparentEntityState::MovingEntity: {
        assert(d->currentItem && "Unexpected empty item with moving state");

        if (d->targetItem == nullptr || d->targetItem == d->originalCurrentItemParent) {
            d->currentItem->setPos(d->originalCurrentItemPos);
            return;
        }

        auto *entity = d->currentItem->internalNode();
        auto *oldParent = entity->parent();
        auto *newParent = d->targetItem->internalNode();

        auto oldName = entity->name();
        auto newName = [&]() {
            if (Preferences::useLakosianRules()) {
                auto oldPrefix = oldParent->name();
                auto prefixIndex = oldName.find(oldPrefix);
                if (prefixIndex != 0) {
                    // If the prefix isn't found at the beginning of the string, avoid changing anything
                    return entity->name();
                }

                auto result = oldName;
                result.replace(0, oldPrefix.size(), newParent->name());
                return result;
            }
            return entity->name();
        }();
        entity->setName(newName);

        auto result = d->nodeStorage.reparentEntity(entity, newParent);
        if (result.has_error()) {
            switch (result.error().kind) {
            case lvtldr::ErrorReparentEntity::Kind::InvalidEntity: {
                assert(false && "Unexpected invalid kind of node");
                break;
            }
            case lvtldr::ErrorReparentEntity::Kind::InvalidParent: {
                assert(false && "Unexpected invalid parent");
                break;
            }
            }

            d->state = Private::ToolReparentEntityState::Finished;
            return;
        }

        Q_EMIT undoCommandCreated(
            new lvtqtc::UndoReparentEntity(d->nodeStorage, entity, oldParent, newParent, oldName, newName));

        d->state = Private::ToolReparentEntityState::Finished;
    }
    case Private::ToolReparentEntityState::NoEntitySelected: {
        // Noop
        return;
    }
    case Private::ToolReparentEntityState::Finished: {
        assert(false && "Unexpected state");
    }
    }
}

void ToolReparentEntity::deactivate()
{
    graphicsView()->unsetCursor();
    if (d->state == Private::ToolReparentEntityState::MovingEntity) {
        d->currentItem->setParentItem(d->originalCurrentItemParent);
    }
    updateCurrentItemTo(nullptr);
    updateTargetItemTo(nullptr);
    d->originalCurrentItemPos = {0.0, 0.0};
    d->state = Private::ToolReparentEntityState::Browsing;
    ITool::deactivate();
}

void ToolReparentEntity::updateCurrentItemTo(LakosEntity *newItem)
{
    auto setFocusedStyle = [&](LakosEntity *item, bool enabled) {
        if (enabled) {
            item->setOpacity(0.5);
            item->setPen(Qt::PenStyle::DashLine);
            d->originalCurrentItemZValue = item->zValue();
            item->setZValue(std::numeric_limits<qreal>::max());
        } else {
            item->setOpacity(1.0);
            item->setPen(Qt::PenStyle::SolidLine);
            item->setZValue(d->originalCurrentItemZValue);
        }
    };

    if (d->currentItem) {
        setFocusedStyle(d->currentItem, false);
    }
    d->currentItem = newItem;
    if (d->currentItem) {
        setFocusedStyle(d->currentItem, true);
    }
}

void ToolReparentEntity::updateTargetItemTo(LakosEntity *newItem)
{
    static auto setFocusedStyle = [](LakosEntity *item, bool enabled) {
        if (enabled) {
            item->setOpacity(0.5);
            item->setPen(Qt::PenStyle::DashLine);
        } else {
            item->setOpacity(1.0);
            item->setPen(Qt::PenStyle::SolidLine);
        }
    };

    if (d->targetItem) {
        setFocusedStyle(d->targetItem, false);
    }
    d->targetItem = newItem;
    if (d->targetItem) {
        setFocusedStyle(d->targetItem, true);
    }
}

} // namespace Codethink::lvtqtc

#include "moc_ct_lvtqtc_tool_reparent_entity.cpp"
