// ct_lvtmdl_basetreemodel.h                                         -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_BASETREEMODEL
#define DEFINED_CT_LVTMDL_BASETREEMODEL

#include <lvtmdl_export.h>

#include <ct_lvtclr_colormanagement.h>

#include <QStandardItemModel>

#include <utility>
#include <vector>

namespace Codethink::lvtldr {
class LakosianNode;
}

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT BaseTreeModel : public QStandardItemModel
// A tree model that lazy loads inner data.
{
    Q_OBJECT
    // DATA TYPES
    struct BaseTreeModelPrivate;

  public:
    BaseTreeModel();
    ~BaseTreeModel() noexcept override;

    virtual void reload() = 0;
    // should refresh the data on the model.

    [[nodiscard]] bool isLoaded(const QString& identifier, const QModelIndex& parent = {});
    // checks if there is a branch or a leaf with the specified text.
    // TODO: Check if we hit false positives.

    [[nodiscard]] bool isLoaded(const std::string& identifier, const QModelIndex& parent = {});
    // converts to QString and calls the method above.

    [[nodiscard]] QModelIndex indexForData(const std::vector<std::pair<QVariant, int>>& variantToRole,
                                           const QModelIndex& parent = {});

    [[nodiscard]] QModelIndex indexForData(const std::vector<std::pair<std::string, int>>& strToRole,
                                           const QModelIndex& parent = {});
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

    // returns the index of the first element that satisfies the
    // data for the specified role.

    void setColorManagement(std::shared_ptr<lvtclr::ColorManagement> colorManagement);

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList& indexes) const override;

  private:
    std::unique_ptr<BaseTreeModelPrivate> d;
};

} // end namespace Codethink::lvtmdl

#endif
