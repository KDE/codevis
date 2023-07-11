// ct_lvtwdg_namespacetreemodel.h                                    -*-C++-*-

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

#ifndef INCLUDED_LVTMDL_NAMESPACETREEMODEL
#define INCLUDED_LVTMDL_NAMESPACETREEMODEL

//@PURPOSE: Defines a data model for tree views, containing the
// elements of all namespaces.
//
//@CLASSES:
//  lvtmdl::NamespaceTreeModel:
//
//@DESCRIPTION: This component prepares the data of a
// namespace to be displayed on a tree view
//
/// Usage:
///------
///  Create a model and pass the Dbo::Session to it,
///  then plug the model onto a view.
///
///  auto model = std::make_unique<NamespaceTreeModel>();
///  model->setDboSession(session);
///  view->setModel(model);
///
///  d->treeView->collapsed().connect(this,
///     [this](const WModelIndex& idx) {
///         d->model->collapse(idx);
///     }
///  );
///
///  d->treeView->expanded().connect(this,
///     [this](const WModelIndex& idx) {
///         d->model->expand(idx);
///     }
///  );
///
///  You should manually connect to expand and collapse
///  from the treeview Expanded() and Collapsed() signals
///  as we need that information to fetch the data from
///  the namespaces.

#include <ct_lvtmdl_basetreemodel.h>

#include <any>
#include <memory>
#include <optional>

#include <lvtmdl_export.h>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT NamespaceTreeModel : public BaseTreeModel
// This class represents the model of a namespace tree containing
// all inner namespaces and classes.
{
    Q_OBJECT
    // DATA TYPES
    struct NamespaceTreeModelPrivate;

  public:
    // CREATORS
    NamespaceTreeModel();
    // CONSTRUCTOR

    ~NamespaceTreeModel() override;
    // DESTRUCTOR

    void setRootNamespace(const std::optional<QString>& rootNamespace);
    // Will only load the namespaces that are internal to this namespace.

    [[nodiscard]] std::optional<QString> rootNamespace() const;
    // returns the current Root Namespace

    void reload() override;

    [[nodiscard]] QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QString errorString() const;
    [[nodiscard]] bool hasError() const;

  private:
    // MEMBERS
    std::unique_ptr<NamespaceTreeModelPrivate> d;
};

} // end namespace Codethink::lvtmdl

#endif
