// ct_lvtmdl_basetablemodel.cpp                                         -*-C++-*-

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

#include <ct_lvtmdl_basetablemodel.h>

#include <cassert>
#include <climits>

namespace {

[[nodiscard]] inline int sizeToInt(std::size_t i)
// it is annoying that Qt uses int to measure sizes
{
    assert(i <= (std::size_t) INT_MAX);
    return static_cast<int>(i);
}

} // namespace

namespace Codethink::lvtmdl {

struct BaseTableModel::Private {
    std::string fullyQualifiedName;
    lvtshr::DiagramType type = lvtshr::DiagramType::NoneType;
    int columnCount = 0;

    // outer vector: rows, inner vector: cols.
    std::vector<std::vector<QString>> elements;
    std::vector<QString> headerData;
};

BaseTableModel::BaseTableModel(int columnCount, QObject *parent):
    QAbstractTableModel(parent), d(std::make_unique<BaseTableModel::Private>())
{
    d->columnCount = columnCount;
}

BaseTableModel::~BaseTableModel() = default;

int BaseTableModel::columnCount(const QModelIndex& unused) const
{
    (void) unused;
    return d->columnCount;
}

QVariant BaseTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || (int) orientation != Qt::Horizontal) {
        return {};
    }

    if (section >= columnCount()) {
        return {};
    }

    return d->headerData[section];
}

QVariant BaseTableModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (!index.isValid()) {
        return {};
    }

    return d->elements[index.row()][index.column()];
}

std::string BaseTableModel::fullyQualifiedName() const
{
    return d->fullyQualifiedName;
}

lvtshr::DiagramType BaseTableModel::type() const
{
    return d->type;
}

int BaseTableModel::rowCount(const QModelIndex& parent) const
{
    (void) parent;

    return sizeToInt(d->elements.size());
}

void BaseTableModel::clear()
{
    beginResetModel();
    d->elements = {};
    endResetModel();
}

void BaseTableModel::addRow(const std::vector<QString>& row)
{
    beginInsertRows(QModelIndex(), sizeToInt(d->elements.size()), sizeToInt(d->elements.size()));
    d->elements.push_back(row);
    endInsertRows();
}

void BaseTableModel::addRow(const std::string& row)
{
    addRow({QString::fromStdString(row)});
}

void BaseTableModel::setHeader(const std::vector<QString>& header)
{
    d->headerData = header;
    Q_EMIT headerDataChanged(Qt::Orientation::Horizontal, 0, columnCount(QModelIndex()) - 1);
}

} // end namespace Codethink::lvtmdl

#include "moc_ct_lvtmdl_basetablemodel.cpp"
