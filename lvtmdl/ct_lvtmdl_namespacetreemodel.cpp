// ct_lvtmdl_namespacetreemodel.cpp                                  -*-C++-*-

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

#include <ct_lvtmdl_namespacetreemodel.h>

#include <ct_lvtmdl_modelhelpers.h>

namespace Codethink::lvtmdl {

// --------------------------------------------
// struct NamespaceTreeModelPrivate
// --------------------------------------------
struct NamespaceTreeModel::NamespaceTreeModelPrivate {
    std::optional<QString> rootNamespace;
    bool hasError = true;
    QString error;
};

// --------------------------------------------
// class NamespaceTreeModel
// --------------------------------------------
NamespaceTreeModel::NamespaceTreeModel(): d(std::make_unique<NamespaceTreeModel::NamespaceTreeModelPrivate>())
{
}

NamespaceTreeModel::~NamespaceTreeModel() = default;

void NamespaceTreeModel::setRootNamespace(const std::optional<QString>& rootNamespace)
{
    d->rootNamespace = rootNamespace;
    reload();
}

namespace {

} // namespace
void NamespaceTreeModel::reload()
{
    auto *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());

    // TODO: Retrieve namespace list from lvtldr
    //    Transaction transaction(*session());
    //    lvtcdb::CdbUtil::NamespaceDeclarationList cxx_namespaces;
    //    if (!d->rootNamespace) {
    //        cxx_namespaces =
    //            session()->find<lvtcdb::NamespaceDeclaration>().where("parent_id IS
    //            NULL").orderBy("name").resultList();
    //
    //        qDebug() << "Repopulating with global namespace that contains" << cxx_namespaces.size() << "children";
    //    } else {
    //        // TODO: Communicate errors to the interface.
    //        auto root_namespace = find_namespace(session(), d->rootNamespace);
    //        if (!root_namespace) {
    //            d->error = "Requested namespace does not exist";
    //            d->hasError = true;
    //            return;
    //        }
    //
    //        auto id = root_namespace.id();
    //        cxx_namespaces = session()
    //                             ->find<lvtcdb::NamespaceDeclaration>()
    //                             .where("parent_id IS ?")
    //                             .bind(id)
    //                             .orderBy("name")
    //                             .resultList();
    //
    //        qDebug() << "Repopulating with root namespace" << root_namespace->name() << "That contains"
    //                 << cxx_namespaces.size() << "children";
    //    }
    //
    //    using NamespacePtr = ptr<lvtcdb::NamespaceDeclaration>;
    //    for (const NamespacePtr& cxx_namespace : cxx_namespaces) {
    //        auto *item = ModelUtil::createTreeItem(cxx_namespace, NodeType::e_Namespace);
    //        fillNamespace(item, cxx_namespace);
    //        root->appendRow(item);
    //    }
    //
    //    lvtcdb::CdbUtil::UserDefinedTypeList cxx_classes;
    //    cxx_classes = session()
    //                      ->find<lvtcdb::UserDefinedType>()
    //                      .where("parent_namespace_id IS NULL")
    //                      .orderBy("qualified_name")
    //                      .resultList();
    //
    //    using ClassPtr = ptr<lvtcdb::UserDefinedType>;
    //    for (const ClassPtr& cxx_class : cxx_classes) {
    //        auto *item = ModelUtil::createLeafItem(cxx_class, NodeType::e_Class);
    //        root->appendRow(item);
    //    }
}

QVariant NamespaceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    (void) orientation;

    if (section != 0) {
        return {};
    }
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (d->rootNamespace) {
        return *d->rootNamespace;
    }
    return "Global";
}

std::optional<QString> NamespaceTreeModel::rootNamespace() const
{
    return d->rootNamespace;
}

QString NamespaceTreeModel::errorString() const
{
    return d->error;
}

bool NamespaceTreeModel::hasError() const
{
    return d->hasError;
}

} // end namespace Codethink::lvtmdl
