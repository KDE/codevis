// ct_lvtmdl_simpletextmodel.h                                         -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_SIMPLETEXTMODEL
#define DEFINED_CT_LVTMDL_SIMPLETEXTMODEL

#include <lvtmdl_export.h>

#include <QAbstractListModel>

#include <memory>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT SimpleTextModel : public QAbstractListModel {
    // This is a simple text model, use this if you
    // need a list of items that can be used on a QComboBox / QListView
    // and that doesn't need other data carried around.
    // it supports addition and deletion of items
    Q_OBJECT
  public:
    SimpleTextModel(QObject *parent = nullptr);
    ~SimpleTextModel() override;

    [[nodiscard]] QStringList stringList() const;

    void addString(const QString& str);

    bool removeRows(int row, int count, const QModelIndex& parent) override;
    [[nodiscard]] int rowCount(const QModelIndex& idx = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& idx, int role) const override;

    void setStorageGroup(const QString& group);
    // if set, the model will use this QSettings Group file to save / load data.

    void saveData();
    void loadData();

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtmdl
#endif
