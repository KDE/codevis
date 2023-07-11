// ct_lvtmdl_circle_relationships_model.cpp                       -*-C++-*-

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

#include <ct_lvtmdl_circular_relationships_model.h>

#include <filesystem>

using namespace Codethink::lvtmdl;

CircularRelationshipsModel::CircularRelationshipsModel(QObject *parent): QStandardItemModel(parent)
{
}

QVariant CircularRelationshipsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return {};
}

void CircularRelationshipsModel::setCircularRelationships(
    const std::vector<std::vector<std::pair<std::string, std::string>>>& cycles)
{
    clear();

    if (cycles.empty()) {
        Q_EMIT emptyState();
    }

    auto *rootItem = invisibleRootItem();
    for (const auto& cycle : cycles) {
        auto cycleName = std::string{};
        cycleName += std::filesystem::path(cycle[0].first).filename().string();
        cycleName += " -> ... -> ";
        cycleName += std::filesystem::path(cycle[cycle.size() - 1].first).filename().string();
        auto *cycleItem = new QStandardItem(tr("Cycle %1").arg(QString::fromStdString(cycleName)));
        auto cyclePathData = QList<QString>{};
        for (const auto& namePair : cycle) {
            auto const lhsName = QString::fromStdString(std::filesystem::path(namePair.first).filename().string());
            auto const rhsName = QString::fromStdString(std::filesystem::path(namePair.second).filename().string());
            auto *item = new QStandardItem(tr("%1 to %2").arg(lhsName, rhsName));
            cycleItem->appendRow(item);
            cyclePathData.push_back(lhsName);
        }
        cycleItem->setData(QVariant::fromValue<QList<QString>>(cyclePathData),
                           CircularRelationshipsModel::CyclePathAsListRole);
        cycleItem->setCheckable(true);
        cycleItem->setCheckState(Qt::Unchecked);
        rootItem->appendRow(cycleItem);
    }

    Q_EMIT relationshipsUpdated();
}
