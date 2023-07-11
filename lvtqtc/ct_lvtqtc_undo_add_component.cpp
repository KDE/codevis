// ct_lvtqtc_undo_add_component.cpp                                                                            -*-C++-*-

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

#include <ct_lvtqtc_undo_add_component.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_isa.h>
#include <ct_lvtqtc_lakosentity.h>

#include <ct_lvtqtc_packageentity.h>

#include <ct_lvtshr_graphenums.h>

#include <optional>

#include <QPointF>

using namespace Codethink::lvtqtc;
using namespace Codethink;

namespace Codethink::lvtldr {
class LakosianNode;
}

UndoAddComponent::UndoAddComponent(GraphicsScene *scene,
                                   const QPointF& pos,
                                   const std::string& name,
                                   const std::string& qualifiedName,
                                   const std::string& parentQualifiedName,
                                   QtcUtil::UndoActionType undoActionType,
                                   lvtldr::NodeStorage& storage):
    UndoAddEntityBase{scene, pos, name, qualifiedName, parentQualifiedName, undoActionType, storage}
{
    if (undoActionType == QtcUtil::UndoActionType::e_Add) {
        setText(QObject::tr("Add Component '%1'").arg(QString::fromStdString(name)));
    }

    if (undoActionType == QtcUtil::UndoActionType::e_Remove) {
        setText(QObject::tr("Remove Component '%1'").arg(QString::fromStdString(name)));
    }
}

UndoAddComponent::~UndoAddComponent() = default;

void UndoAddComponent::removeEntity()
{
    auto *node = m_storage.findByQualifiedName(lvtshr::DiagramType::ComponentType, m_addInfo.qualifiedName);
    m_addInfo.currentNotes = node->notes();

    if (m_scene) {
        LakosEntity *visualEntity = m_scene->findLakosEntityFromUid(node->uid());
        m_addInfo.pos = visualEntity->pos();
    }
    m_storage.removeComponent(node).expect("Unexpected error on UndoRedo action");

    if (m_scene) {
        m_scene->updateBoundingRect();
    }
}

void UndoAddComponent::addEntity()
{
    auto *parent = m_storage.findByQualifiedName(lvtshr::DiagramType::PackageType, m_addInfo.parentQualifiedName);
    auto result = m_storage.addComponent(m_addInfo.name, m_addInfo.qualifiedName, parent);
    assert(!result.has_error());
    auto *newComponent = result.value();
    newComponent->setNotes(m_addInfo.currentNotes);

    if (m_scene) {
        m_scene->setEntityPos(newComponent->uid(), m_addInfo.pos);
    }
}
