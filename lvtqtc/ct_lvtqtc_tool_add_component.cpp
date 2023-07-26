// ct_lvtqtw_tool_add_component.cpp                                            -*-C++-*-

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

#include <ct_lvtqtc_tool_add_component.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtc_inputdialog.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtqtc_undo_add_component.h>
#include <ct_lvtqtc_util.h>
#include <ct_lvtshr_functional.h>

#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

#include <preferences.h>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct ToolAddComponent::Private {
    NodeStorage& nodeStorage;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

ToolAddComponent::ToolAddComponent(GraphicsView *gv, NodeStorage& nodeStorage):
    BaseAddEntityTool(tr("Component"), tr("Creates a Component"), IconHelpers::iconFrom(":/icons/component"), gv),
    d(std::make_unique<Private>(nodeStorage))
{
    auto *inputDataDialog = inputDialog();
    inputDataDialog->addTextField("name", tr("Name:"));
    inputDataDialog->finish();
}

ToolAddComponent::~ToolAddComponent() = default;

template<typename T>
cpp::result<void, InvalidComponentError>
checkNameError(bool hasParent, const std::string& name, const std::string& parentName)
{
    return T::checkName(hasParent, name, parentName);
}

void ToolAddComponent::activate()
{
    Q_EMIT sendMessage(tr("Click on a Package or Package Group to add a new Component"), KMessageWidget::Information);
    BaseAddEntityTool::activate();
}

void ToolAddComponent::mousePressEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Press Event";

    using Codethink::lvtshr::ScopeExit;
    ScopeExit _([&]() {
        deactivate();
    });

    auto *parent = graphicsView()->itemByTypeAt<PackageEntity>(event->pos());
    if (!parent) {
        Q_EMIT sendMessage(tr("You can't create a component outside of packages."), KMessageWidget::Error);
        return;
    }

    m_nameDialog->setTextFieldValue("name", QString::fromStdString(parent->name()) + "_");
    if (m_nameDialog->exec() == QDialog::Rejected) {
        qCDebug(LogTool) << "Add component rejected by the user";
        return;
    }

    const std::string name = std::any_cast<QString>(m_nameDialog->fieldValue("name")).toStdString();

    // Verify if the name meets Lakosian rules
    if (Preferences::self()->document()->useLakosianRules()) {
        const auto result = checkNameError<LakosianComponentNameRules>(true, name, parent->name());
        if (result.has_error()) {
            Q_EMIT sendMessage(result.error().what, KMessageWidget::Error);
            return;
        }
    }

    auto qualifiedName = parent->qualifiedName() + "/" + name;
    auto result = d->nodeStorage.addComponent(name, qualifiedName, parent->internalNode());
    if (result.has_error()) {
        using Kind = ErrorAddComponent::Kind;
        switch (result.error().kind) {
        case Kind::MissingParent: {
            assert(false && "Unexpected missing parent");
        }

        case Kind::QualifiedNameAlreadyRegistered: {
            Q_EMIT sendMessage(tr("You can't create a new package with the same name of an existing package"),
                               KMessageWidget::Error);
            return;
        }
        case Kind::CannotAddComponentToPkgGroup: {
            Q_EMIT sendMessage(tr("Cannot add a component to a package group"), KMessageWidget::Error);
            return;
        }
        }
    }

    Q_EMIT sendMessage(QString(), KMessageWidget::Information);

    auto *newNode = result.value();
    const QPointF scenePos = graphicsView()->mapToScene(event->pos());
    const QPointF itemPos = parent->mapFromScene(scenePos);
    auto *scene = qobject_cast<GraphicsScene *>(graphicsView()->scene());
    scene->setEntityPos(newNode->uid(), itemPos);
    scene->updateBoundingRect();
    Q_EMIT undoCommandCreated(new UndoAddComponent(scene,
                                                   itemPos,
                                                   name,
                                                   qualifiedName,
                                                   parent->qualifiedName(),
                                                   QtcUtil::UndoActionType::e_Add,
                                                   d->nodeStorage));
}

cpp::result<void, InvalidComponentError>
LakosianComponentNameRules::checkName(bool hasParent, const std::string& name, const std::string& parentName)
{
    const auto header = QObject::tr("BDE Guidelines Enforced: ");
    if (!hasParent) {
        return cpp::fail(InvalidComponentError{header + QObject::tr("Components must be added within packages.")});
    }

    // rule: the company can add a few letters to the filename to identify that the
    // file is from the company, but that's not required.
    // and the package name *must* be written before the component name.
    auto numUnderscore = std::count_if(std::begin(name), std::end(name), [](char a) {
        return a == '_';
    });
    if (numUnderscore == 0) {
        return cpp::fail(InvalidComponentError{header
                                               + QObject::tr("Invalid name. name must be in the form %1_component")
                                                     .arg(QString::fromStdString(parentName))});
    }

    // here we know that the start of the string *needs* to be the parent package.
    if (numUnderscore == 1) {
        if (!QString::fromStdString(name).startsWith(QString::fromStdString(parentName))) {
            return cpp::fail(InvalidComponentError{header
                                                   + QObject::tr("Invalid name. name must be in the form %1_component")
                                                         .arg(QString::fromStdString(parentName))});
        }
    }

    // the first, or the second values must be the name of the parent package.
    if (numUnderscore >= 2) {
        std::vector<std::string> split_tmp;
        auto qtVector = QString::fromStdString(name).split(QString::fromStdString("_"));
        auto stdVector = std::vector<std::string>{};
        std::transform(qtVector.begin(), qtVector.end(), std::back_inserter(split_tmp), [](auto&& qtString) {
            return qtString.toStdString();
        });

        if (!QString::fromStdString(split_tmp[0]).startsWith(QString::fromStdString(parentName))
            && !QString::fromStdString(split_tmp[1]).startsWith(QString::fromStdString(parentName))) {
            return cpp::fail(InvalidComponentError{header
                                                   + QObject::tr("Invalid name. name must be in the form %1_component")
                                                         .arg(QString::fromStdString(parentName))});
        }
    }

    return {};
}

} // namespace Codethink::lvtqtc
