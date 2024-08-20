// ct_lvtqtw_tool_add_package.cpp                                            -*-C++-*-

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

#include <ct_lvtqtc_tool_add_logical_entity.h>

#include <ct_lvtqtc_componententity.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_inputdialog.h>
#include <ct_lvtqtc_logicalentity.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtqtc_undo_add_logicalentity.h>

#include <ct_lvtqtc_util.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtshr_functional.h>

#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

#include <preferences.h>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtshr;

namespace Codethink::lvtqtc {

struct ToolAddLogicalEntity::Private {
    NodeStorage& nodeStorage;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

ToolAddLogicalEntity::ToolAddLogicalEntity(GraphicsView *gv, NodeStorage& nodeStorage):
    BaseAddEntityTool(tr("Logical Entity"),
                      tr("Creates a Logical Entity (such as classes or structs)"),
                      IconHelpers::iconFrom(":/icons/class"),
                      gv),
    d(std::make_unique<Private>(nodeStorage))
{
    auto *inputDataDialog = inputDialog();
    inputDataDialog->addTextField("name", tr("Name:"));
    inputDataDialog->addComboBoxField("kind", tr("Kind:"), {"Class", "Enum", "Struct", "TypeAlias", "Union"});
    inputDataDialog->finish();
}

ToolAddLogicalEntity::~ToolAddLogicalEntity() = default;

template<typename T>
cpp::result<void, InvalidLogicalEntityError>
checkNameError(bool hasParent, const std::string& name, const std::string& parentName)
{
    return T::checkName(hasParent, name, parentName);
}

void ToolAddLogicalEntity::activate()
{
    Q_EMIT sendMessage(tr("Click on a Component, Class or Struct to add a new Logical Entity"),
                       KMessageWidget::Information);
    BaseAddEntityTool::activate();
}

void ToolAddLogicalEntity::mousePressEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Press Event";

    using Codethink::lvtshr::ScopeExit;
    ScopeExit _([&]() {
        deactivate();
    });

    auto *parentView = [&]() -> LakosEntity * {
        auto *parentClass = graphicsView()->itemByTypeAt<LogicalEntity>(event->pos());
        if (parentClass != nullptr) {
            return parentClass;
        }
        return graphicsView()->itemByTypeAt<ComponentEntity>(event->pos());
    }();

    if (!parentView) {
        Q_EMIT sendMessage(
            tr("This is not a valid parent for a user defined type. Parent must be a 'Component', a 'Class' or a "
               "'Struct'."),
            KMessageWidget::Error);
        return;
    }

    if (m_nameDialog->exec() == QDialog::Rejected) {
        return;
    }
    assert(parentView);

    auto *parent = parentView->internalNode();
    auto parentName = parent->name();
    auto name = std::any_cast<QString>(m_nameDialog->fieldValue("name")).toStdString();
    auto qualifiedName = parentName + "::" + name;

    auto kindInput = std::any_cast<QString>(m_nameDialog->fieldValue("kind"));
    auto kind = ([&kindInput]() {
        if (kindInput == "Class") {
            return UDTKind::Class;
        }
        if (kindInput == "Enum") {
            return UDTKind::Enum;
        }
        if (kindInput == "Struct") {
            return UDTKind::Struct;
        }
        if (kindInput == "TypeAlias") {
            return UDTKind::TypeAlias;
        }
        if (kindInput == "Union") {
            return UDTKind::Union;
        }
        return UDTKind::Unknown;
    })();

    auto result = d->nodeStorage.addLogicalEntity(name, qualifiedName, parent, kind);
    if (result.has_error()) {
        using Kind = ErrorAddUDT::Kind;
        switch (result.error().kind) {
        case Kind::BadParentType: {
            Q_EMIT sendMessage(
                tr("This is not a valid parent for a user defined type. Parent must be a 'Component', a 'Class' or a "
                   "'Struct'."),
                KMessageWidget::Error);
            return;
        }
        }
    }

    // Success, clean the message on the view.
    Q_EMIT sendMessage(QString(), KMessageWidget::Information);

    auto *newLogicalEntity = result.value();

    auto *scene = qobject_cast<GraphicsScene *>(graphicsView()->scene());
    const QPointF scenePos = graphicsView()->mapToScene(event->pos());
    const QPointF itemPos = parentView->mapFromScene(scenePos);
    scene->setEntityPos(newLogicalEntity->uid(), itemPos);
    scene->updateBoundingRect();
    Q_EMIT undoCommandCreated(new UndoAddLogicalEntity(scene,
                                                       itemPos,
                                                       name,
                                                       qualifiedName,
                                                       parent->qualifiedName(),
                                                       QtcUtil::UndoActionType::e_Add,
                                                       d->nodeStorage));
}

cpp::result<void, InvalidLogicalEntityError>
LogicalEntityNameRules::checkName(bool hasParent, const std::string& name, const std::string& parentName)
{
    return {};
}

} // namespace Codethink::lvtqtc

#include "moc_ct_lvtqtc_tool_add_logical_entity.cpp"
