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

#include <ct_lvtqtc_tool_add_package.h>

#include <ct_lvtqtc_undo_add_package.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtshr_functional.h>

#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtc_util.h>

#include <ct_lvtldr_nodestorage.h>

#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

#include <preferences.h>

using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

struct ToolAddPackage::Private {
    NodeStorage& nodeStorage;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

ToolAddPackage::ToolAddPackage(GraphicsView *gv, NodeStorage& nodeStorage):
    BaseAddEntityTool(tr("Package"), tr("Creates a Package"), IconHelpers::iconFrom(":/icons/package"), gv),
    d(std::make_unique<Private>(nodeStorage))
{
    auto *inputDataDialog = inputDialog();
    inputDataDialog->addTextField("name", tr("Name:"));
    inputDataDialog->finish();
}

ToolAddPackage::~ToolAddPackage() = default;

template<typename T>
cpp::result<void, InvalidNameError>
checkNameError(bool hasParent, const std::string& name, const std::string& parentName)
{
    return T::checkName(hasParent, name, parentName);
}

void ToolAddPackage::activate()
{
    Q_EMIT sendMessage(tr("Click on an empty spot to add a new package group,"
                          "or on a package group to add a new package."),
                       KMessageWidget::Information);

    BaseAddEntityTool::activate();
}

void ToolAddPackage::mouseReleaseEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Release Event";

    using Codethink::lvtshr::ScopeExit;
    ScopeExit _([&]() {
        deactivate();
    });

    event->accept();

    auto *parentView = graphicsView()->itemByTypeAt<PackageEntity>(event->pos());
    auto *parent = parentView ? parentView->internalNode() : nullptr;
    auto parentName = parent ? parent->name() : "";
    auto parentQualifiedName = parent ? parent->qualifiedName() : "";

    m_nameDialog->setTextFieldValue("name", QString::fromStdString(parentName));
    if (m_nameDialog->exec() == QDialog::Rejected) {
        return;
    }

    // Verify if the names are correct / sane.
    const std::string name = std::any_cast<QString>(m_nameDialog->fieldValue("name")).toStdString();
    if (Preferences::self()->useLakosianRules()) {
        const auto result = checkNameError<LakosianNameRules>(parent != nullptr, name, parentName);
        if (result.has_error()) {
            Q_EMIT sendMessage(result.error().what, KMessageWidget::Error);
            return;
        }
    }

    auto qualifiedName = parent ? parentName + "/" + name : name;
    auto *scene = qobject_cast<GraphicsScene *>(graphicsView()->scene());

    auto result = d->nodeStorage.addPackage(name, qualifiedName, parent, scene);
    if (result.has_error()) {
        using Kind = ErrorAddPackage::Kind;
        switch (result.error().kind) {
        case Kind::QualifiedNameAlreadyRegistered: {
            Q_EMIT sendMessage(
                tr("Qualified name already registered %1").arg(QString::fromStdString(result.error().what)),
                KMessageWidget::Error);
            return;
        }
        case Kind::CannotAddPackageToStandalonePackage: {
            Q_EMIT sendMessage(
                tr("Cannot add a package to a standalone package (because it already contains a component)"),
                KMessageWidget::Error);
            return;
        }
        case Kind::CantAddChildren: {
            Q_EMIT sendMessage(tr("Cannot add Children on the package"), KMessageWidget::Error);
            return;
        }
        }
    }
    auto *newPackage = result.value();
    Q_EMIT sendMessage(QString(), KMessageWidget::Information);

    const QPointF scenePos = graphicsView()->mapToScene(event->pos());
    const QPointF itemPos = parentView ? parentView->mapFromScene(scenePos) : scenePos;
    scene->setEntityPos(newPackage->uid(), itemPos);
    scene->updateBoundingRect();

    Q_EMIT undoCommandCreated(new UndoAddPackage(scene,
                                                 itemPos,
                                                 name,
                                                 qualifiedName,
                                                 parentQualifiedName,
                                                 QtcUtil::UndoActionType::e_Add,
                                                 d->nodeStorage));
}

cpp::result<void, InvalidNameError>
LakosianNameRules::checkName(bool hasParent, const std::string& name, const std::string& parentName)
{
    const auto header = QObject::tr("BDE Guidelines Enforced: ");
    if (hasParent) {
        if (name.find('_') != std::string::npos) {
            return cpp::fail(InvalidNameError{header + QObject::tr("Package names should not contain underscore")});
        }
        if (name.size() < 4) {
            return cpp::fail(InvalidNameError{
                header + QObject::tr("Name too short, must be at least four letters (parent package(3) + name(1-5))")});
        }

        if (name.size() > 8) {
            return cpp::fail(InvalidNameError{
                header + QObject::tr("Name too long, must be at most eight letters (parent package(3) + name(1-5))")});
        }
        if (!QString::fromStdString(name).startsWith(QString::fromStdString(parentName))) {
            return cpp::fail(InvalidNameError{header
                                              + QObject::tr("the package name must start with the parent's name (%1)")
                                                    .arg(QString::fromStdString(parentName))});
        }
    } else if (name.size() != 3) {
        return cpp::fail(InvalidNameError{header + QObject::tr("top level packages must be three letters long.")});
    }
    return {};
}

} // namespace Codethink::lvtqtc
