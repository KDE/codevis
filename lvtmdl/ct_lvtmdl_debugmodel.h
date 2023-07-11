// ct_lvtmdl_debugmodel.h                                -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_DEBUGMODEL
#define DEFINED_CT_LVTMDL_DEBUGMODEL

#include <lvtmdl_export.h>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <QString>
#include <memory>

#include <ct_lvtshr_graphenums.h>

namespace Codethink::lvtmdl {

struct LVTMDL_EXPORT DebugData {
    QtMsgType type = QtSystemMsg;
    QString file;
    QString line;
    QString category;
    QString function;
    int version = 0;
    QString message;
};

class LVTMDL_EXPORT DebugModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    enum Columns { e_TYPE, e_FILE, e_LINE, e_CATEGORY, e_FUNCTION, e_VERSION, e_MESSAGE, COUNT };

    DebugModel();
    ~DebugModel() override;

    void clear();
    // clear the model.

    void addData(const DebugData& data);
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& idx, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] bool saveAs(const QString& filename) const;
    static void debugMessageHandler(QtMsgType msgType, const QMessageLogContext& context, const QString& message);
    struct Private;
    std::unique_ptr<Private> d;
};

class LVTMDL_EXPORT DebugModelFilter : public QSortFilterProxyModel {
  public:
    DebugModelFilter();
    ~DebugModelFilter() override;

    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    [[nodiscard]] bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

    void setFilterDebug(bool filter);
    void setFilterWarning(bool filter);
    void setFilterInfo(bool filter);
    void setFilterCrtical(bool filter);
    void setFilterFatal(bool filter);

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
