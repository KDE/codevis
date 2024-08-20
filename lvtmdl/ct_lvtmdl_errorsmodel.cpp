// ct_lvtmdl_errorsmodel.h                                         -*-C++-*-

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

#include <ct_lvtmdl_errorsmodel.h>

#include <QDebug>
#include <QFontMetrics>
#include <QList>
#include <QString>

using namespace Codethink::lvtmdl;

struct DataInfo {
    enum Type {
        e_ErrorKind,
        e_QualifiedName,
        e_FileName,
        e_ErrorString,
    };

    int errorKind;
    QString qualifiedName;
    QString filename;
    QString errorString;
};

struct ErrorsModel::Private {
    QList<DataInfo> messages;
};

ErrorsModel::ErrorsModel(): d(std::make_unique<ErrorsModel::Private>())
{
}

ErrorsModel::~ErrorsModel() = default;

void ErrorsModel::reset()
{
    beginResetModel();
    d->messages.clear();

    // TODO: Retrieve errors from lvtldr
    //    if (d->session) {
    //        Transaction transaction(*d->session);
    //        const auto errorMessages = d->session->find<lvtcdb::ErrorMessages>();
    //        for (auto& errorMessage : errorMessages.resultList()) {
    //            d->messages.append(DataInfo{
    //                errorMessage->errorKind(),
    //                QString::fromStdString(errorMessage->fullyQualifiedName()),
    //                QString::fromStdString(errorMessage->fileName()),
    //                QString::fromStdString(errorMessage->errorMessage()),
    //            });
    //        }
    //    }

    endResetModel();
}

int ErrorsModel::rowCount(const QModelIndex& idx) const
{
    Q_UNUSED(idx);
    return d->messages.count();
}

int ErrorsModel::columnCount(const QModelIndex& idx) const
{
    Q_UNUSED(idx);
    return 4;
}

QVariant ErrorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Vertical) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case DataInfo::Type::e_ErrorKind:
        return tr("Type");
    case DataInfo::Type::e_QualifiedName:
        return tr("Qualified Name");
    case DataInfo::Type::e_FileName:
        return tr("File");
    case DataInfo::Type::e_ErrorString:
        return tr("Error Message");
    }

    return {};
}

QVariant ErrorsModel::data(const QModelIndex& idx, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::SizeHintRole) {
        return {};
    }

    const auto thisValue = d->messages.at(idx.row());
    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
        case DataInfo::Type::e_ErrorKind:
            return thisValue.errorKind == 0 ? tr("Compiler Messages") : tr("Parse Messages");

        case DataInfo::Type::e_ErrorString:
            return thisValue.errorString;
        case DataInfo::Type::e_FileName:
            return thisValue.filename;
        case DataInfo::Type::e_QualifiedName:
            return thisValue.qualifiedName;
        }
    } else if (role == Qt::ToolTipRole) {
        return thisValue.errorString;
    }

    return {};
}

struct ErrorModelFilter::Private {
    bool filterCompilerMessages = false;
    bool filterParseMessages = false;
    bool invertStringFilter = false;
    QAbstractItemModel *ignoreCategoriesModel = nullptr;
    QString filterString;
};

ErrorModelFilter::ErrorModelFilter(): d(std::make_unique<ErrorModelFilter::Private>())
{
}

ErrorModelFilter::~ErrorModelFilter() = default;

bool ErrorModelFilter::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_parent);

    auto *currModel = qobject_cast<ErrorsModel *>(sourceModel());

    const QModelIndex idx = currModel->index(source_row, DataInfo::Type::e_ErrorKind);
    const QVariant data = currModel->data(idx, Qt::DisplayRole);
    const QString type = data.toString();

    if (d->filterCompilerMessages && type == tr("Compiler Messages")) {
        return false;
    }
    if (d->filterParseMessages && type == tr("Parse Messages")) {
        return false;
    }

    if (d->filterString.size()) {
        const QModelIndex messageIdx = currModel->index(source_row, DataInfo::Type::e_ErrorString);
        const QString currMessage = currModel->data(messageIdx, Qt::DisplayRole).toString();
        const bool lineContainsFilter = currMessage.contains(d->filterString);

        if (d->invertStringFilter) {
            return !lineContainsFilter;
        }
        return lineContainsFilter;
    }

    return true;
}

bool ErrorModelFilter::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    Q_UNUSED(source_left);
    Q_UNUSED(source_right);
    return true;
}

void ErrorModelFilter::setFilterCompilerMessages(bool filter)
{
    if (d->filterCompilerMessages == filter) {
        return;
    }
    d->filterCompilerMessages = filter;
    invalidateFilter();
}

void ErrorModelFilter::setFilterParseMessages(bool filter)
{
    if (d->filterParseMessages == filter) {
        return;
    }
    d->filterParseMessages = filter;
    invalidateFilter();
}

void ErrorModelFilter::setIgnoreCategoriesModel(QAbstractItemModel *model)
{
    if (d->ignoreCategoriesModel) {
        disconnect(d->ignoreCategoriesModel, &QAbstractItemModel::rowsInserted, this, nullptr);
        disconnect(d->ignoreCategoriesModel, &QAbstractItemModel::rowsRemoved, this, nullptr);
    }

    d->ignoreCategoriesModel = model;
    connect(model, &QAbstractItemModel::rowsInserted, this, [this] {
        invalidateFilter();
    });

    connect(model, &QAbstractItemModel::rowsRemoved, this, [this] {
        invalidateFilter();
    });
}

void ErrorModelFilter::setFilterString(const QString& str)
{
    if (d->filterString == str) {
        return;
    }
    d->filterString = str;
    invalidateFilter();
}

void ErrorModelFilter::setInvertMessageFilter(bool value)
{
    if (d->invertStringFilter == value) {
        return;
    }
    d->invertStringFilter = value;
    invalidateFilter();
}

#include "moc_ct_lvtmdl_errorsmodel.cpp"
