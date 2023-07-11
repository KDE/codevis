// ct_lvtmdl_errorsmodel.h                                         -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_ERRORSMODEL
#define DEFINED_CT_LVTMDL_ERRORSMODEL

#include <lvtmdl_export.h>

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QSortFilterProxyModel>

#include <memory>

namespace Codethink::lvtmdl {
class LVTMDL_EXPORT ErrorsModel : public QAbstractTableModel {
    /* Model that holds the data from Parsing Errors from the code
     * extractor, and compiler errors. This is used to show the user
     * possible mistakes that happened during the process of code
     * examination, while building the relationship database.
     */

    Q_OBJECT
  public:
    ErrorsModel();
    ~ErrorsModel() override;

    void reset();

    [[nodiscard]] int rowCount(const QModelIndex& idx) const override;
    [[nodiscard]] int columnCount(const QModelIndex& idx) const override;
    [[nodiscard]] QVariant data(const QModelIndex& idx, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

class LVTMDL_EXPORT ErrorModelFilter : public QSortFilterProxyModel {
  public:
    ErrorModelFilter();
    ~ErrorModelFilter() override;

    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    [[nodiscard]] bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

    void setFilterCompilerMessages(bool filter);
    void setFilterParseMessages(bool filter);

    void setIgnoreCategoriesModel(QAbstractItemModel *model);
    // Sets the model with the list of strings that will be used to filter
    // categories out

    void setFilterString(const QString& str);
    // Use this string to match the messages that will be filtered on the view

    void setInvertMessageFilter(bool value);
    // If true, return messages that don't match the string
    // if false, return messages that match the string.

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtmdl
#endif
