// ct_lvtqtwc_tool_add_physical_dependency.cpp                                            -*-C++-*-

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

#include <ct_lvtqtc_tool_add_physical_dependency.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_undo_add_edge.h>
#include <ct_lvtqtc_util.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtshr_functional.h>

#include <QDebug>
#include <QGraphicsLineItem>
#include <QMessageBox>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct ToolAddPhysicalDependency::Private {
    NodeStorage& nodeStorage;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

ToolAddPhysicalDependency::ToolAddPhysicalDependency(GraphicsView *gv, NodeStorage& nodeStorage):
    EdgeBasedTool(tr("Physical Dependency"),
                  tr("Creates a dependency between packages or components"),
                  IconHelpers::iconFrom(":/icons/new_dependency"),
                  gv),
    d(std::make_unique<Private>(nodeStorage))
{
}

ToolAddPhysicalDependency::~ToolAddPhysicalDependency() = default;

bool ToolAddPhysicalDependency::run(LakosEntity *fromItem, LakosEntity *toItem)
{
    bool needsParentDependencies = false;
    {
        auto result = d->nodeStorage.addPhysicalDependency(fromItem->internalNode(), toItem->internalNode());
        if (result.has_error()) {
            using Kind = ErrorAddPhysicalDependency::Kind;
            switch (result.error().kind) {
            case Kind::HierarchyLevelMismatch: {
                Q_EMIT sendMessage(tr("Cannot create dependency on different hierarchy levels"), KMessageWidget::Error);
                return false;
            }
            case Kind::InvalidType: {
                Q_EMIT sendMessage(tr("Only physical entities may be used with this tool"), KMessageWidget::Error);
                return false;
            }
            case Kind::SelfRelation: {
                Q_EMIT sendMessage(tr("Cannot create self-dependency"), KMessageWidget::Error);
                return false;
            }
            case Kind::DependencyAlreadyExists: {
                Q_EMIT sendMessage(tr("Those elements already have a dependency"), KMessageWidget::Error);
                return false;
            }
            case Kind::MissingParentDependency: {
                // do not use the message box in debug mode.  assume "yes".
                if (property("debug").toBool()) {
                    needsParentDependencies = true;
                } else {
                    auto ret = QMessageBox::question(
                        graphicsView(),
                        tr("Automatically add Dependencies?"),
                        tr("Parents are not connected, should we add the dependencies automatically?"));
                    if (ret == QMessageBox::No) {
                        Q_EMIT sendMessage(tr("We can only add a dependency if the parents are conected"),
                                           KMessageWidget::Error);
                        return false;
                    }
                    needsParentDependencies = true;
                }
            }
            }
        }
    }

    if (needsParentDependencies) {
        auto hierarchy = calculateHierarchy(fromItem, toItem);
        auto *macro = new QUndoCommand("Multiple Add Edges");
        auto forceAddPhysicalDep =
            [&](auto const& iFrom, auto const& iTo, auto const& pair, PhysicalDependencyType type) {
                auto ret = d->nodeStorage.addPhysicalDependency(iFrom, iTo, type);
                (void) ret;
                assert(!ret.has_error());
                new UndoAddEdge(pair.first->qualifiedName(),
                                pair.second->qualifiedName(),
                                type == PhysicalDependencyType::AllowedDependency
                                    ? lvtshr::LakosRelationType::AllowedDependency
                                    : lvtshr::LakosRelationType::PackageDependency,
                                QtcUtil::UndoActionType::e_Add,
                                d->nodeStorage,
                                macro);
            };

        for (const auto& pair : hierarchy) {
            const auto& iFrom = pair.first->internalNode();
            const auto& iTo = pair.second->internalNode();
            forceAddPhysicalDep(iFrom, iTo, pair, PhysicalDependencyType::AllowedDependency);
        }

        // try to add the edge again, now that the parents are ok.
        {
            auto result = d->nodeStorage.addPhysicalDependency(fromItem->internalNode(), toItem->internalNode());
            (void) result;
            assert(!result.has_error());
            new UndoAddEdge(fromItem->qualifiedName(),
                            toItem->qualifiedName(),
                            lvtshr::LakosRelationType::PackageDependency,
                            QtcUtil::UndoActionType::e_Add,
                            d->nodeStorage,
                            macro);
        }

        for (const auto& pair : hierarchy) {
            const auto& iFrom = pair.first->internalNode();
            const auto& iTo = pair.second->internalNode();
            forceAddPhysicalDep(iFrom, iTo, pair, PhysicalDependencyType::ConcreteDependency);
        }

        Q_EMIT undoCommandCreated(macro);
    } else {
        // TODO: This should be a Group because of the parent edges.
        Q_EMIT undoCommandCreated(new UndoAddEdge(fromItem->qualifiedName(),
                                                  toItem->qualifiedName(),
                                                  lvtshr::LakosRelationType::PackageDependency,
                                                  QtcUtil::UndoActionType::e_Add,
                                                  d->nodeStorage));
    }
    return true;
}

} // namespace Codethink::lvtqtc
