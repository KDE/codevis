// ct_lvtqtw_tabwidget.h                                             -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#ifndef INCLUDED_LVTMDL_SQLMODEL
#define INCLUDED_LVTMDL_SQLMODEL

#include <ct_lvtldr_nodestorage.h>
#include <lvtmdl_export.h>

#include <QAbstractTableModel>
#include <qnamespace.h>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT SqlModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    SqlModel(lvtldr::NodeStorage& s);
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void setQuery(const QString& query);

    Q_SIGNAL void invalidQueryTriggered(const QString& errorMessage, const QString& query);

  private:
    lvtldr::NodeStorage& nodeStorage;
    std::vector<std::vector<std::string>> tables;
    std::vector<std::string> hData;
};

} // namespace Codethink::lvtmdl

#endif
