// ct_lvtmdl_modelhelpers.cpp                                        -*-C++-*-

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

#include <QApplication>
#include <QStyledItemDelegate>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtmdl_modelhelpers.h>

namespace Codethink::lvtmdl {

using namespace Codethink::lvtldr;
using namespace Codethink::lvtmdl;
using lvtshr::DiagramType;

NodeType::Enum NodeType::fromDiagramType(lvtshr::DiagramType type)
{
    using lvtshr::DiagramType;
    switch (type) {
    case DiagramType::ClassType:
        return NodeType::e_Class;
    case DiagramType::ComponentType:
        return NodeType::e_Component;
    case DiagramType::PackageType:
        return NodeType::e_Package;
    case DiagramType::RepositoryType:
        return NodeType::e_Repository;
    case DiagramType::NoneType:
        break;
    }
    return NodeType::e_Invalid;
}

lvtshr::DiagramType NodeType::toDiagramType(NodeType::Enum type)
{
    switch (type) {
    case NodeType::e_Class:
        return DiagramType::ClassType;
    case NodeType::e_Component:
        return DiagramType::ComponentType;
    case NodeType::e_Package:
        return DiagramType::PackageType;
    case e_Namespace:
        break;
    case e_Repository:
        return DiagramType::RepositoryType;
    case e_Invalid:
        break;
    }
    return DiagramType::NoneType;
}

std::tuple<LakosianNode::IsLakosianResult, std::optional<QString>> nodeIsRecursivelyLakosian(LakosianNode& node)
{
    auto isLakosianResult = node.isLakosian();
    auto notLakosianMessage = [&]() -> std::optional<QString> {
        switch (isLakosianResult) {
        case LakosianNode::IsLakosianResult::IsLakosian: {
            return std::nullopt;
        }
        case LakosianNode::IsLakosianResult::ComponentHasNoPackage: {
            return QObject::tr("Component %1 has no package.").arg(QString::fromStdString(node.name()));
        }
        case LakosianNode::IsLakosianResult::ComponentDoesntStartWithParentName: {
            return QObject::tr("Component %1 does not starts with the parent name %2")
                .arg(QString::fromStdString(node.name()),
                     QString::fromStdString(dynamic_cast<PackageNode *>(node.parent())->canonicalName()));
        }
        case LakosianNode::IsLakosianResult::PackageParentIsNotGroup: {
            return QObject::tr("Parent package is not a package group");
        }
        case LakosianNode::IsLakosianResult::PackagePrefixDiffersFromGroup: {
            return QObject::tr("The package %1 prefix differs from the package group")
                .arg(QString::fromStdString(node.name()));
        }
        case LakosianNode::IsLakosianResult::PackageNameInvalidNumberOfChars: {
            return QObject::tr("The package %1 name is not between 3 and 6 characters")
                .arg(QString::fromStdString(node.name()));
        }
        case LakosianNode::IsLakosianResult::PackageGroupNameInvalidNumberOfChars: {
            return QObject::tr("Package groups must be three letter long, but %1 doesn't")
                .arg(QString::fromStdString(node.name()));
        }
        }
        return std::nullopt;
    }();

    if (isLakosianResult != LakosianNode::IsLakosianResult::IsLakosian) {
        return {isLakosianResult, notLakosianMessage};
    }

    if (node.type() == DiagramType::PackageType) {
        // Only check for children for packages and package groups, since we assume that all classes inside any
        // Lakosian Component are also Lakosian.
        for (auto *child : node.children()) {
            auto isChildLakosianResultAndMessage = nodeIsRecursivelyLakosian(*child);
            if (std::get<0>(isChildLakosianResultAndMessage) != LakosianNode::IsLakosianResult::IsLakosian) {
                return isChildLakosianResultAndMessage;
            }
        }
    }

    return {LakosianNode::IsLakosianResult::IsLakosian, std::nullopt};
}

QIcon getIconFor(LakosianNode& node)
{
    if (node.type() == DiagramType::RepositoryType) {
        static const auto iconRepo = QIcon(":/icons/repository");
        return iconRepo;
    }

    if (node.type() == DiagramType::ClassType) {
        static const auto iconClass = QIcon(":/icons/class");
        return iconClass;
    }

    if (node.isLakosian() == LakosianNode::IsLakosianResult::IsLakosian) {
        if (node.type() == DiagramType::PackageType) {
            static const auto iconPkg = QIcon(":/icons/package");
            return iconPkg;
        }
        if (node.type() == DiagramType::ComponentType) {
            static const auto iconComponent = QIcon(":/icons/component");
            return iconComponent;
        }
    } else {
        if (node.type() == DiagramType::PackageType) {
            static const auto iconFolder = QIcon(":/icons/folder");
            return iconFolder;
        }
        if (node.type() == DiagramType::ComponentType) {
            static const auto iconFile = QIcon(":/icons/file");
            return iconFile;
        }
    }

    static const auto iconHelp = QIcon(":/icons/help");
    return iconHelp;
}

QStandardItem *
ModelUtil::createTreeItemFromLakosianNode(LakosianNode& node,
                                          std::optional<ShouldPopulateChildren_f> const& shouldPopulateChildren)
// Creates an element that can hold inner elements
// like a package or a component.
{
    const auto [isLakosianResult, notLakosianMessage] = nodeIsRecursivelyLakosian(node);
    auto *item = new QStandardItem();
    item->setText(QString::fromStdString(node.name()));
    item->setIcon(getIconFor(node));
    item->setData(node.id(), ModelRoles::e_Id);
    item->setData(true, ModelRoles::e_IsBranch);
    item->setData((int) NodeType::fromDiagramType(node.type()), ModelRoles::e_NodeType);
    item->setData(QString::fromStdString(node.qualifiedName()), ModelRoles::e_QualifiedName);
    item->setData(isLakosianResult == LakosianNode::IsLakosianResult::IsLakosian, ModelRoles::e_RecursiveLakosian);
    if (notLakosianMessage) {
        item->setData(*notLakosianMessage, Qt::ToolTipRole);
    }

    if (shouldPopulateChildren.has_value() && (*shouldPopulateChildren)(node)) {
        ModelUtil::populateTreeItemChildren(node, *item, shouldPopulateChildren);
        item->setData(true, ModelRoles::e_ChildItemsLoaded);
    } else {
        // Dummy item so that Qt creates an "expandable" parent
        item->appendRow(new QStandardItem());
        item->setData(false, ModelRoles::e_ChildItemsLoaded);
    }

    return item;
}

void ModelUtil::populateTreeItemChildren(lvtldr::LakosianNode& node,
                                         QStandardItem& item,
                                         std::optional<ShouldPopulateChildren_f> const& shouldPopulateChildren)
{
    std::vector<LakosianNode *> children = node.children();
    if (children.empty()) {
        item.setData(false, ModelRoles::e_IsBranch);
        return;
    }

    std::sort(children.begin(), children.end(), [](LakosianNode *l, LakosianNode *r) {
        if (l->name() == "non-lakosian group") {
            return false;
        }
        if (r->name() == "non-lakosian group") {
            return true;
        }
        return l->name() < r->name();
    });
    if (node.type() == DiagramType::RepositoryType) {
        // In case of a repository tree item, we do not want to show the inner packages as children of the repository,
        // we only want the toplevel items to be children of the repository, so we need to unconsider everything else.
        children.erase(std::remove_if(children.begin(),
                                      children.end(),
                                      [](auto&& child) {
                                          if (!child->parent()) {
                                              return true;
                                          }

                                          if (child->parent()->type() != lvtshr::DiagramType::RepositoryType) {
                                              return true;
                                          }

                                          return false;
                                      }),
                       children.end());
    }
    QList<QStandardItem *> childItems;
    for (auto *child : children) {
        childItems.push_back(ModelUtil::createTreeItemFromLakosianNode(*child, shouldPopulateChildren));
    }
    item.appendRows(childItems);
}

} // namespace Codethink::lvtmdl
