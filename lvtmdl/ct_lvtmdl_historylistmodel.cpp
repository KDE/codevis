// ct_lvtmdl_historylistmodel.cpp                                         -*-C++-*-

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
#include <ct_lvtmdl_historylistmodel.h>

#include <QString>

#include <QDebug>

namespace Codethink::lvtmdl {

struct HistoryListModel::Private {
    int currentIndex = -1;
    int maximumHistory = 10;
    std::vector<std::pair<QString, lvtshr::DiagramType>> history;
};

HistoryListModel::HistoryListModel(QObject *parent):
    QAbstractListModel(parent), d(std::make_unique<HistoryListModel::Private>())
{
}

HistoryListModel::~HistoryListModel() noexcept = default;

int HistoryListModel::currentIndex() const
{
    return d->currentIndex;
}

void HistoryListModel::setCurrentIndex(int idx)
{
    qDebug() << "Trying to set the current index";
    if (d->currentIndex == idx) {
        qDebug() << "Current index is the same, returning";
        return;
    }
    qDebug() << "Current index changed to " << idx;
    d->currentIndex = idx;
    Q_EMIT currentIndexChanged(idx);
}

void HistoryListModel::next()
{
    qDebug() << "Trying to call next";
    if (currentIndex() + 1 < rowCount()) {
        setCurrentIndex(currentIndex() + 1);
    } else {
        qDebug() << "Can't advance." << currentIndex() << rowCount();
    }
}

void HistoryListModel::previous()
{
    qDebug() << "Trying to call prev";
    if (currentIndex() - 1 >= 0 && rowCount() > 0) {
        setCurrentIndex(currentIndex() - 1);
    } else {
        qDebug() << "Cant go back" << currentIndex() << rowCount();
    }
}

void HistoryListModel::append(const std::pair<QString, lvtshr::DiagramType>& qualifiedNameToType)
{
    beginInsertRows({}, rowCount(), rowCount());
    d->history.push_back(qualifiedNameToType);
    assert(d->history.size() <= (std::size_t) INT_MAX);
    d->currentIndex = static_cast<int>(d->history.size()) - 1;
    endInsertRows();
}

void HistoryListModel::setMaximumHistory(int maximum)
{
    d->maximumHistory = maximum;
    if ((int) d->history.size() > maximum) {
        beginResetModel();
        d->history.resize(maximum);
        d->history.shrink_to_fit();
        endResetModel();
    }
}

int HistoryListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    const std::size_t size = d->history.size();
    assert(size <= (std::size_t) INT_MAX);
    return static_cast<int>(size);
}

QVariant HistoryListModel::data(const QModelIndex& idx, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (!idx.isValid()) {
        return {};
    }

    if (idx.row() > rowCount()) {
        return {};
    }

    return d->history[idx.row()].first;
}

std::pair<QString, lvtshr::DiagramType> Codethink::lvtmdl::HistoryListModel::at(int idx) const
{
    if (idx >= rowCount()) {
        return {};
    }
    return d->history[idx];
}

} // namespace Codethink::lvtmdl
