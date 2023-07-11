// ct_lvtmdl_basetablemodel.h                                         -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_BASETABLEMODEL
#define DEFINED_CT_LVTMDL_BASETABLEMODEL

#include <lvtmdl_export.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtshr_graphenums.h>

#include <QAbstractTableModel>

#include <memory>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT BaseTableModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    BaseTableModel(int columnCount, QObject *parent = nullptr);
    ~BaseTableModel() override;

    void setFocusedNode(const std::string& fullyQualifiedName, lvtshr::DiagramType type);
    // sets the data we are displaying

    [[nodiscard]] std::string fullyQualifiedName() const;
    [[nodiscard]] lvtshr::DiagramType type() const;

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& unused = QModelIndex()) const override;

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void clear();

  protected:
    void addRow(const std::vector<QString>& row);
    // add a new row for this model's data.

    void addRow(const std::string& row);
    // overload for a common case

    void setHeader(const std::vector<QString>& header);
    // set the header information on this model.

    virtual void refreshData() = 0;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtmdl

#endif
