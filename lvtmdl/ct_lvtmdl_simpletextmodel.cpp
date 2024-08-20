#include <ct_lvtmdl_simpletextmodel.h>

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

#include <QDebug>
#include <QSettings>

namespace Codethink::lvtmdl {

struct SimpleTextModel::Private {
    QStringList strings;
    QString storageGroup;
};

SimpleTextModel::SimpleTextModel(QObject *parent):
    QAbstractListModel(parent), d(std::make_unique<SimpleTextModel::Private>())
{
}

SimpleTextModel::~SimpleTextModel()
{
    saveData();
}

QStringList SimpleTextModel::stringList() const
{
    return d->strings;
}

void SimpleTextModel::addString(const QString& str)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    d->strings.push_back(str);
    endInsertRows();
}

void SimpleTextModel::setStorageGroup(const QString& group)
{
    d->storageGroup = group;
    loadData();
}

bool SimpleTextModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = row; i < row + count; i++) {
        d->strings.removeAt(row);
    }
    endRemoveRows();
    return true;
}

int SimpleTextModel::rowCount(const QModelIndex& idx) const
{
    Q_UNUSED(idx);
    return d->strings.count();
}

QVariant SimpleTextModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    return d->strings[idx.row()];
}

void SimpleTextModel::saveData()
{
    qDebug() << "Saving data";

    if (d->storageGroup.isEmpty()) {
        return;
    }

    QSettings settings;
    settings.beginGroup(d->storageGroup);
    settings.setValue("ModelData", d->strings);
}

void SimpleTextModel::loadData()
{
    qDebug() << "Loading data";
    if (d->storageGroup.isEmpty()) {
        return;
    }

    QSettings settings;
    settings.beginGroup(d->storageGroup);
    beginResetModel();
    d->strings = settings.value("ModelData", QStringList()).toStringList();
    endResetModel();
}

} // namespace Codethink::lvtmdl

#include "moc_ct_lvtmdl_simpletextmodel.cpp"
