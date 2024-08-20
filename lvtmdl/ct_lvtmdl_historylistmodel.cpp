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
    std::vector<QString> bookmarkHistory;
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
    if (currentIndex() + 1 < rowCount()) {
        qDebug() << "Setting current index to" << currentIndex() + 1;
        setCurrentIndex(currentIndex() + 1);
    }
}

void HistoryListModel::previous()
{
    if (currentIndex() - 1 >= 0 && rowCount() > 0) {
        qDebug() << "Setting current index to" << currentIndex() - 1;
        setCurrentIndex(currentIndex() - 1);
    }
}

void HistoryListModel::append(const QString& bookmarkName)
{
    qDebug() << "Adding " << bookmarkName << "on the history model";
    beginInsertRows({}, rowCount(), rowCount());
    d->bookmarkHistory.push_back(bookmarkName);
    assert(d->bookmarkHistory.size() < (std::size_t) INT_MAX);
    d->currentIndex = static_cast<int>(d->bookmarkHistory.size()) - 1;
    endInsertRows();
}

void HistoryListModel::setMaximumHistory(int maximum)
{
    d->maximumHistory = maximum;
    if ((int) d->bookmarkHistory.size() > maximum) {
        beginResetModel();
        d->bookmarkHistory.resize(maximum);
        d->bookmarkHistory.shrink_to_fit();
        endResetModel();
    }
}

int HistoryListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    const std::size_t size = d->bookmarkHistory.size();
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

    return d->bookmarkHistory[idx.row()];
}

QString Codethink::lvtmdl::HistoryListModel::at(int idx) const
{
    if (idx >= rowCount()) {
        return {};
    }
    return d->bookmarkHistory[idx];
}

} // namespace Codethink::lvtmdl

#include "moc_ct_lvtmdl_historylistmodel.cpp"
