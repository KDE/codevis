// ct_lvtmdl_historylistmodel.h                                -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_HISTORYLISTMODEL
#define DEFINED_CT_LVTMDL_HISTORYLISTMODEL

#include <lvtmdl_export.h>

#include <QAbstractListModel>
#include <QString>
#include <memory>

#include <ct_lvtshr_graphenums.h>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT HistoryListModel : public QAbstractListModel {
    Q_OBJECT
  public:
    HistoryListModel(QObject *parent = nullptr);
    ~HistoryListModel() noexcept override;

    [[nodiscard]] int currentIndex() const;
    void setCurrentIndex(int idx);
    void next();
    void previous();
    Q_SIGNAL void currentIndexChanged(int idx);

    void append(const std::pair<QString, lvtshr::DiagramType>& qualifiedNameToType);
    void setMaximumHistory(int maximum);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& idx, int role) const override;
    [[nodiscard]] std::pair<QString, lvtshr::DiagramType> at(int idx) const;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtmdl

#endif
