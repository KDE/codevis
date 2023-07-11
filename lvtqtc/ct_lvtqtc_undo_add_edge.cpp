// ct_lvtqtc_undo_add_edge.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_undo_add_edge.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_isa.h>
#include <ct_lvtqtc_lakosentity.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_typenode.h>
#include <ct_lvtshr_graphenums.h>

#include <QDebug>
#include <QPointF>

using namespace Codethink::lvtqtc;
using Codethink::lvtshr::LakosRelationType;

struct UndoAddEdge::Private {
    std::string fromQualifiedName;
    std::string toQualifiedName;
    lvtshr::LakosRelationType relationType;
    QtcUtil::UndoActionType undoActionType;
    lvtldr::NodeStorage& nodeStorage;
};

UndoAddEdge::UndoAddEdge(std::string fromQualifiedName,
                         std::string toQualifiedName,
                         lvtshr::LakosRelationType relationType,
                         QtcUtil::UndoActionType undoActionType,
                         lvtldr::NodeStorage& nodeStorage,
                         QUndoCommand *parent):
    QUndoCommand(parent),
    d(std::make_unique<UndoAddEdge::Private>(
        Private{std::move(fromQualifiedName), std::move(toQualifiedName), relationType, undoActionType, nodeStorage}))
{
    setText(
        QObject::tr("Undo %1 relationship").arg(undoActionType == QtcUtil::UndoActionType::e_Add ? "Add" : "Remove"));
}

UndoAddEdge::~UndoAddEdge() = default;

namespace {

using namespace Codethink::lvtldr;

std::pair<LakosianNode *, LakosianNode *>
findNodes(NodeStorage& storage, const std::string& fromStr, const std::string& toStr)
{
    using Codethink::lvtshr::DiagramType;

    LakosianNode *from = storage.findByQualifiedName(DiagramType::PackageType, fromStr);
    LakosianNode *to = storage.findByQualifiedName(DiagramType::PackageType, toStr);
    if (!from || !to) {
        from = storage.findByQualifiedName(DiagramType::ComponentType, fromStr);
        to = storage.findByQualifiedName(DiagramType::ComponentType, toStr);
    }
    if (!from || !to) {
        from = storage.findByQualifiedName(DiagramType::ClassType, fromStr);
        to = storage.findByQualifiedName(DiagramType::ClassType, toStr);
    }

    assert(from && to);
    return {from, to};
}

void removeRelationship(std::unique_ptr<UndoAddEdge::Private>& d)
{
    auto [from, to] = findNodes(d->nodeStorage, d->fromQualifiedName, d->toQualifiedName);

    auto isPhysicalDependency = d->relationType == LakosRelationType::PackageDependency
        || d->relationType == LakosRelationType::AllowedDependency;
    if (isPhysicalDependency) {
        auto depType = d->relationType == LakosRelationType::PackageDependency
            ? PhysicalDependencyType::ConcreteDependency
            : PhysicalDependencyType::AllowedDependency;
        d->nodeStorage.removePhysicalDependency(from, to, depType).expect("");
    }

    auto isLogicalRelation = d->relationType == LakosRelationType::IsA
        || d->relationType == LakosRelationType::UsesInTheInterface
        || d->relationType == LakosRelationType::UsesInTheImplementation;
    if (isLogicalRelation) {
        d->nodeStorage
            .removeLogicalRelation(dynamic_cast<TypeNode *>(from), dynamic_cast<TypeNode *>(to), d->relationType)
            .expect("Unexpected undo/redo error: Remove logical relation failed");
    }
}

void addRelationship(std::unique_ptr<UndoAddEdge::Private>& d)
{
    auto [from, to] = findNodes(d->nodeStorage, d->fromQualifiedName, d->toQualifiedName);

    auto isPhysicalDependency = d->relationType == LakosRelationType::PackageDependency
        || d->relationType == LakosRelationType::AllowedDependency;
    if (isPhysicalDependency) {
        auto depType = d->relationType == LakosRelationType::PackageDependency
            ? PhysicalDependencyType::ConcreteDependency
            : PhysicalDependencyType::AllowedDependency;
        d->nodeStorage.addPhysicalDependency(from, to, depType).expect("Unexpected undo/redo action");
    }

    auto isLogicalRelation = d->relationType == LakosRelationType::IsA
        || d->relationType == LakosRelationType::UsesInTheInterface
        || d->relationType == LakosRelationType::UsesInTheImplementation;
    if (isLogicalRelation) {
        d->nodeStorage.addLogicalRelation(dynamic_cast<TypeNode *>(from), dynamic_cast<TypeNode *>(to), d->relationType)
            .expect("Unexpected undo/redo error: Add logical relation failed");
    }
}

} // namespace

void UndoAddEdge::undo()
{
    switch (d->undoActionType) {
    case QtcUtil::UndoActionType::e_Add:
        removeRelationship(d);
        break;
    case QtcUtil::UndoActionType::e_Remove:
        addRelationship(d);
        break;
    }
}

void UndoAddEdge::redo()
{
    IGNORE_FIRST_CALL

    switch (d->undoActionType) {
    case QtcUtil::UndoActionType::e_Add:
        addRelationship(d);
        break;
    case QtcUtil::UndoActionType::e_Remove:
        removeRelationship(d);
        break;
    }
}
