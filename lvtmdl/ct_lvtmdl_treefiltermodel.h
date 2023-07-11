// ct_lvtwdg_treefiltermodel.h                                       -*-C++-*-

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

#ifndef INCLUDED_LVTMDL_TREEFILTERMODEL
#define INCLUDED_LVTMDL_TREEFILTERMODEL

#include <lvtmdl_export.h>

#include <memory>
#include <string>

//@PURPOSE: Filters the data from the original model
//
//@CLASSES:
//  lvtmdl::TreeFilterModel:
//
//@DESCRIPTION: This component allows filtering of the.
// class model by a string.
//
/// Usage:
///------
///  auto sort_model = std::make_unique<TreeFilterModel>()
///  sort_model->setSourceModel(originalModel);
///  treeView->setModel(sort_model);
///  lineEdit->changed().connect(sort_model, [sort_model]{
///     sort_model->setFilter(lineEdit->text());
///  });

#include <QSortFilterProxyModel>
using FilterProxyModel = QSortFilterProxyModel;

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT TreeFilterModel : public QSortFilterProxyModel
// This class filters data from the TreeFilterModel
{
    // DATA TYPES
    struct TreeFilterModelPrivate;

  public:
    // CREATORS
    TreeFilterModel();
    // Constructor
    ~TreeFilterModel() noexcept override;
    // Destructor

    void setFilter(const QString& filter);
    // Sets the filter to act on filterAcceptRow

    [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

  private:
    std::unique_ptr<TreeFilterModelPrivate> d;
};

} // end namespace Codethink::lvtmdl

#endif
