// ct_lvtmdl_basetreemodel.cpp                                       -*-C++-*-

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

#include <ct_lvtmdl_basetreemodel.h>

#include <ct_lvtmdl_modelhelpers.h>

#include <QDebug>
#include <QIODevice>
#include <QMimeData>

namespace Codethink::lvtmdl {

struct BaseTreeModel::BaseTreeModelPrivate {
    std::shared_ptr<lvtclr::ColorManagement> colorManagement;
};

BaseTreeModel::BaseTreeModel(): d(std::make_unique<BaseTreeModelPrivate>())
{
}

BaseTreeModel::~BaseTreeModel() noexcept = default;

void BaseTreeModel::setColorManagement(std::shared_ptr<lvtclr::ColorManagement> colorManagement)
{
    assert(colorManagement);
    d->colorManagement = std::move(colorManagement);
    reload();
}

bool BaseTreeModel::isLoaded(const QString& identifier, const QModelIndex& parent)
{
    constexpr int role = Qt::ItemDataRole::DisplayRole;

    const QModelIndex idx = indexForData({{identifier, role}}, parent);
    return idx.isValid();
}

bool BaseTreeModel::isLoaded(const std::string& identifier, const QModelIndex& parent)
{
    return isLoaded(QString::fromStdString(identifier), parent);
}

QModelIndex BaseTreeModel::indexForData(const std::vector<std::pair<QVariant, int>>& variantToRole,
                                        const QModelIndex& parent)
{
    QModelIndex p_idx = parent;

    const int size = rowCount(p_idx);
    for (int i = 0; i < size; i++) {
        const QModelIndex idx = index(i, 0, p_idx);
        if (!idx.isValid()) {
            return {};
        }

        bool found = true;
        for (const auto& [this_variant, this_role] : variantToRole) {
            // Check first for Qualified Name, then for Name.

            const auto this_role_info = idx.data(this_role);

            // makes cppcheck stop complaining that the value is unused.
            (void) this_role_info;

            if (this_role_info != this_variant) {
                found = false;
                continue;
            }
        }
        if (found) {
            return idx;
        }

        // if we are in a branch, recurse.
        if (idx.data(ModelRoles::e_IsBranch).value<bool>()) {
            const auto inner_idx = indexForData(variantToRole, idx);
            if (inner_idx.isValid()) {
                return inner_idx;
            }
        }
    }

    return {};
}

QModelIndex BaseTreeModel::indexForData(const std::vector<std::pair<std::string, int>>& strToRole,
                                        const QModelIndex& parent)
{
    std::vector<std::pair<QVariant, int>> strToRoleMapped;
    strToRoleMapped.reserve(strToRole.size());
    for (const auto& [str, role] : strToRole) {
        strToRoleMapped.emplace_back(QString::fromStdString(str), role);
    }

    return indexForData(strToRoleMapped, parent);
}

Qt::ItemFlags BaseTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags baseFlags = QStandardItemModel::flags(index);
    baseFlags &= ~Qt::ItemIsEditable;
    return baseFlags;
}

QStringList BaseTreeModel::mimeTypes() const
{
    return {"codevis/qualifiednames"};
}

QMimeData *BaseTreeModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    for (auto& idx : indexes) {
        const auto qualName = idx.data(lvtmdl::ModelRoles::e_QualifiedName).toByteArray();
        if (!encodedData.isEmpty()) {
            encodedData.push_back(";");
        }
        encodedData.push_back(qualName);
    }

    mimeData->setData("codevis/qualifiednames", encodedData);
    qDebug() << "encodedData: " << QString(encodedData);
    return mimeData;
}
} // end namespace Codethink::lvtmdl

#include "moc_ct_lvtmdl_basetreemodel.cpp"
