// ct_lvtwdg_packagetreemodel.h                                      -*-C++-*-

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

#ifndef INCLUDED_LVTMDL_PACKAGETREEEMODEL
#define INCLUDED_LVTMDL_PACKAGETREEEMODEL

//@PURPOSE: Defines a data model for tree views, containing the
// elements of all namespaces.
//
//@CLASSES:
//  lvtmdl::PackageTreeModel:
//
//@DESCRIPTION: This component prepares the data of a
// namespace to be displayed on a tree view
//
/// Usage:
///------
///  Create a model and pass the Dbo::Session to it,
///  then plug the model onto a view.
///
///  You should manually connect to expand and collapse
///  from the treeview Expanded() and Collapsed() signals
///  as we need that information to fetch the data from
///  the namespaces.

#include <ct_lvtmdl_basetreemodel.h>

#include <any>
#include <memory>

#include <lvtmdl_export.h>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT PackageTreeModel : public BaseTreeModel
// This class represents the model of a namespace tree containing
// all inner namespaces and classes.
{
    // DATA TYPES
    struct PackageTreeModelPrivate;

  public:
    // CREATORS
    explicit PackageTreeModel(lvtldr::NodeStorage& nodeStorage);
    // CONSTRUCTOR

    ~PackageTreeModel();

    void reload() override;

    void fetchMore(const QModelIndex& parent) override;

  private:
    QStandardItem *itemForLakosianNode(Codethink::lvtldr::LakosianNode *node);
    // returns the QStandardItem for the specific Physical Node, or null.

    // DATA
    std::unique_ptr<PackageTreeModelPrivate> d;
    friend PackageTreeModelPrivate;
};

} // end namespace Codethink::lvtmdl

#endif
