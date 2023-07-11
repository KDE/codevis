// ct_lvtmdl_circle_relationships_model.h                            -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_CIRCLE_RELATIONSHIPS_MODEL
#define DEFINED_CT_LVTMDL_CIRCLE_RELATIONSHIPS_MODEL

#include <lvtmdl_export.h>

#include <QStandardItemModel>
#include <vector>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT CircularRelationshipsModel : public QStandardItemModel
// A tree model that lazy loads inner data.
{
    Q_OBJECT
  public:
    static const auto CyclePathAsListRole = Qt::UserRole + 1;

    CircularRelationshipsModel(QObject *parent = nullptr);

    void setCircularRelationships(const std::vector<std::vector<std::pair<std::string, std::string>>>& cycles);

    [[nodiscard]] QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_SIGNAL void emptyState();
    Q_SIGNAL void initialState();
    Q_SIGNAL void relationshipsUpdated();
};

} // namespace Codethink::lvtmdl

#endif
