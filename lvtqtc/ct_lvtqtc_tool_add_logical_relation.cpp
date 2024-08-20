// ct_lvtqtwc_tool_add_logical_relation.cpp                                                                    -*-C++-*-

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

#include <ct_lvtqtc_tool_add_logical_relation.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_undo_add_edge.h>
#include <ct_lvtqtc_util.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_typenode.h>

#include <ct_lvtshr_functional.h>

#include <QDebug>
#include <QMessageBox>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct ToolAddLogicalRelation::Private {
    NodeStorage& nodeStorage;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

ToolAddLogicalRelation::ToolAddLogicalRelation(const QString& name,
                                               const QString& tooltip,
                                               const QIcon& icon,
                                               GraphicsView *gv,
                                               Codethink::lvtldr::NodeStorage& nodeStorage):
    EdgeBasedTool(name, tooltip, icon, gv), d(std::make_unique<Private>(nodeStorage))
{
}

ToolAddLogicalRelation::~ToolAddLogicalRelation() = default;

bool ToolAddLogicalRelation::doRun(LakosEntity *fromItem, LakosEntity *toItem, lvtshr::LakosRelationType type)
{
    auto *fromTypeNode = dynamic_cast<TypeNode *>(fromItem->internalNode());
    auto *toTypeNode = dynamic_cast<TypeNode *>(toItem->internalNode());

    auto result = d->nodeStorage.addLogicalRelation(fromTypeNode, toTypeNode, type);
    bool needsParentDependencies = false;
    if (result.has_error()) {
        using Kind = ErrorAddLogicalRelation::Kind;

        switch (result.error().kind) {
        case Kind::InvalidRelation: {
            Q_EMIT sendMessage(tr("Cannot create logical relation between those entities"), KMessageWidget::Error);
            return false;
        }
        case Kind::InvalidLakosRelationType: {
            assert(false && "Unexpected bad LogicalRelation Tool implementation");
            return false;
        }
        case Kind::SelfRelation: {
            Q_EMIT sendMessage(tr("Cannot create self-dependency"), KMessageWidget::Error);
            return false;
        }
        case Kind::AlreadyHaveDependency: {
            Q_EMIT sendMessage(tr("Those elements already have a dependency"), KMessageWidget::Error);
            return false;
        }
        case Kind::ParentDependencyRequired: {
            Q_EMIT sendMessage(
                tr("Cannot create logical entity dependency without the parent entities having a dependency"),
                KMessageWidget::Error);
            return false;
        }
        case Kind::ComponentDependencyRequired: {
            // do not show message box in debug mode. assume "yes".
            if (property("debug").isValid()) {
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
                break;
            }
        }
        }
    }

    if (needsParentDependencies) {
        auto hierarchy = calculateHierarchy(fromItem, toItem);
        auto *macro = new QUndoCommand("Multiple Add Edges");
        for (const auto& pair : hierarchy) {
            const auto& iFrom = pair.first->internalNode();
            const auto& iTo = pair.second->internalNode();
            if (iFrom == iTo) {
                // Ignore if we find a common ancestor. For instance, if two different components are inside a common
                // package, do not try to connect the package to itself.
                continue;
            }

            auto ret = d->nodeStorage.addPhysicalDependency(iFrom, iTo);
            if (ret.has_error()) {
                Q_EMIT sendMessage(tr("Error connecting toplevel parents"), KMessageWidget::Error);
                return false;
            }

            // See the `macro`  on the end the call, adding this on the child
            // list of the QUndoCommand parent. This is not leaking, Qt is just
            // weird sometimes.
            new UndoAddEdge(pair.first->qualifiedName(),
                            pair.second->qualifiedName(),
                            lvtshr::LakosRelationType::PackageDependency,
                            QtcUtil::UndoActionType::e_Add,
                            d->nodeStorage,
                            macro);
        }

        // try to add the edge again, now that the parents are ok.
        auto *fromNode = dynamic_cast<lvtldr::TypeNode *>(fromItem->internalNode());
        auto *toNode = dynamic_cast<lvtldr::TypeNode *>(toItem->internalNode());
        auto result = d->nodeStorage.addLogicalRelation(fromNode, toNode, type);

        // should not have an error, since we tested before, but we still need to test.
        if (result.has_error()) {
            Q_EMIT sendMessage(tr("Error connecting toplevel parents"), KMessageWidget::Error);
            return false;
        }

        new UndoAddEdge(fromNode->qualifiedName(),
                        toNode->qualifiedName(),
                        type,
                        QtcUtil::UndoActionType::e_Add,
                        d->nodeStorage,
                        macro);

        Q_EMIT undoCommandCreated(macro);
    } else {
        Q_EMIT undoCommandCreated(new UndoAddEdge(fromItem->qualifiedName(),
                                                  toItem->qualifiedName(),
                                                  type,
                                                  QtcUtil::UndoActionType::e_Add,
                                                  d->nodeStorage));
    }
    return true;
}

ToolAddIsARelation::ToolAddIsARelation(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage):
    ToolAddLogicalRelation(tr("Is A"),
                           tr("Creates an 'isA' relation between two elements"),
                           IconHelpers::iconFrom(":/icons/add_is_a"),
                           gv,
                           nodeStorage)
{
}

bool ToolAddIsARelation::run(LakosEntity *source, LakosEntity *target)
{
    return doRun(source, target, lvtshr::IsA);
}

ToolAddUsesInTheImplementation::ToolAddUsesInTheImplementation(GraphicsView *gv,
                                                               Codethink::lvtldr::NodeStorage& nodeStorage):
    ToolAddLogicalRelation(tr("Uses In The Impl"),
                           tr("Creates an 'Uses In The Implementation' relation between two elements"),
                           IconHelpers::iconFrom(":/icons/add_uses_in_the_implementation"),
                           gv,
                           nodeStorage)
{
}

bool ToolAddUsesInTheImplementation::run(LakosEntity *source, LakosEntity *target)
{
    return doRun(source, target, lvtshr::UsesInTheImplementation);
}

ToolAddUsesInTheInterface::ToolAddUsesInTheInterface(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage):
    ToolAddLogicalRelation(tr("Uses In The Interface"),
                           tr("Creates an 'Uses In The Interface' relation between two elements"),
                           IconHelpers::iconFrom(":/icons/add_uses_in_the_interface"),
                           gv,
                           nodeStorage)
{
}

bool ToolAddUsesInTheInterface::run(LakosEntity *source, LakosEntity *target)
{
    return doRun(source, target, lvtshr::UsesInTheInterface);
}

} // namespace Codethink::lvtqtc

#include "moc_ct_lvtqtc_tool_add_logical_relation.cpp"
