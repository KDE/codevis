#include <ct_lvtmdl_debugmodel.h>

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
#include <QDir>
#include <QMutex>

#include <preferences.h>

#include <ct_lvtshr_debug_categories.h>

CODEVIS_LOGGING_CATEGORIES(debugModel, "org.codevis.debugmodel", Codethink::lvtshr::LoggingCategory::DebugModel)

namespace {

QString msgTypeToString(int type)
{
    switch (type) {
    case QtMsgType::QtCriticalMsg:
        return "Critical";
    case QtMsgType::QtDebugMsg:
        return "Debug";
    case QtMsgType::QtFatalMsg:
        return "Fatal";
    case QtMsgType::QtInfoMsg:
        return "Info";
    case QtMsgType::QtWarningMsg:
        return "Warning";
    }
    return {};
}

} // namespace
namespace Codethink::lvtmdl {

static DebugModel *self = nullptr; // NOLINT

struct DebugModel::Private {
    constexpr static int BUFFER_ELEMENTS = 5000;

    std::vector<DebugData> data;
    QMutex mtx;

    Private(): data(BUFFER_ELEMENTS){};
};

DebugModel::DebugModel(): d(std::make_unique<DebugModel::Private>())
{
    if (self) {
        qCDebug(debugModel) << "Only one Debug Model should exist at a given time";
    }
    self = this;
}

DebugModel::~DebugModel()
{
    self = nullptr;
}

void DebugModel::addData(const DebugData& data)
{
    QMutexLocker locker(&self->d->mtx);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    d->data.push_back(data);
    endInsertRows();
}

int DebugModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return (int) d->data.size();
}

int DebugModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return DebugModel::Columns::COUNT;
}

QVariant DebugModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Vertical) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case DebugModel::Columns::e_CATEGORY:
        return tr("Category");
    case DebugModel::Columns::e_MESSAGE:
        return tr("Message");
    case DebugModel::Columns::e_FILE:
        return tr("File");
    case DebugModel::Columns::e_FUNCTION:
        return tr("Function");
    case DebugModel::Columns::e_LINE:
        return tr("Line");
    case DebugModel::Columns::e_VERSION:
        return tr("Version");
    case DebugModel::Columns::e_TYPE:
        return tr("Type");
    }
    return {};
}

QVariant DebugModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid()) {
        return {};
    }

    if (idx.row() >= rowCount()) {
        return {};
    }

    const DebugData data = d->data[idx.row()];
    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
        case DebugModel::Columns::e_CATEGORY:
            return data.category;
        case DebugModel::Columns::e_MESSAGE:
            return data.message;
        case DebugModel::Columns::e_FILE:
            return data.file;
        case DebugModel::Columns::e_FUNCTION:
            return data.function;
        case DebugModel::Columns::e_LINE:
            return data.line;
        case DebugModel::Columns::e_VERSION:
            return data.version;
        case DebugModel::Columns::e_TYPE:
            return msgTypeToString(data.type);
        }
    }

    return {};
}

void DebugModel::clear()
{
    beginResetModel();
    d->data.clear();
    d->data.resize(0);
    endResetModel();
}

bool DebugModel::saveAs(const QString& filename) const
{
    QFile dump(filename);
    QTextStream textStream(&dump);

    if (!dump.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    for (const auto& line : d->data) {
        if (line.message.isEmpty()) {
            continue;
        }

        const QString type = msgTypeToString(line.type);
        QString output = line.category + " " + type + ": ";
        if (!line.file.isEmpty()) {
            output += line.file + " ";
        }

        if (!line.function.isEmpty()) {
            output += line.function + " ";
        }

        if (!line.line.isEmpty() && line.line != "0") {
            output += line.line + " ";
        }

        output += line.message + "\n";
        textStream << output;
    }

    dump.close();
    return true;
}

void DebugModel::debugMessageHandler(QtMsgType msgType, const QMessageLogContext& context, const QString& message)
{
    if (self && Preferences::storeDebugOutput()) {
        DebugData d;
        d.category = context.category;
        d.file = context.file;
        d.function = context.function;
        d.line = QString::number(context.line);
        d.message = message;
        d.type = msgType;
        d.version = context.version;
        self->addData(d);
    }
    if (message.trimmed().isEmpty()) {
        return;
    }
    if (context.category != nullptr) {
        QString categoryString = QString::fromStdString(context.category);
        switch (msgType) {
        case QtMsgType::QtCriticalMsg:
            if (!Preferences::enabledCriticalCategories().contains(categoryString)) {
                return;
            }
            break;
        case QtMsgType::QtWarningMsg:
            if (!Preferences::enabledWarningCategories().contains(categoryString)) {
                return;
            }
            break;
        case QtMsgType::QtInfoMsg:
            if (!Preferences::enabledInfoCategories().contains(categoryString)) {
                return;
            }
            break;
        case QtMsgType::QtDebugMsg:
            if (!Preferences::enabledDebugCategories().contains(categoryString)) {
                return;
            }
            break;
        default:
            break;
        }
    }
    // clang-tidy cert-err33-c requires us to check the return value of printf
    auto checkRet = [](int ret) {
        assert(ret > 0);
        (void) ret;
    };

    QString extra;
    if (context.file) {
        QString file(context.file);
        extra = file.split(QDir::separator()).last() + ":" + QString::number(context.line);
    }

    QByteArray localMsg = message.toLocal8Bit();
    QByteArray extraMsg = extra.toLocal8Bit();
    switch (msgType) {
    case QtDebugMsg:
        checkRet(fprintf(stderr, "Debug: %s %s\n", extraMsg.constData(), localMsg.constData()));
        break;
    case QtInfoMsg:
        checkRet(fprintf(stderr, "Info: %s%s\n", extraMsg.constData(), localMsg.constData()));
        break;
    case QtWarningMsg:
        checkRet(fprintf(stderr, "Warning: %s%s\n", extraMsg.constData(), localMsg.constData()));
        break;
    case QtCriticalMsg:
        checkRet(fprintf(stderr, "Critical: %s%s\n", extraMsg.constData(), localMsg.constData()));
        break;
    case QtFatalMsg:
        checkRet(fprintf(stderr, "Fatal: %s%s\n", extraMsg.constData(), localMsg.constData()));
        abort();
    }
}

struct DebugModelFilter::Private {
    bool filterDebug = false;
    bool filterWarning = false;
    bool filterInfo = false;
    bool filterCritical = false;
    bool filterFatal = false;
    bool invertStringFilter = false;
    QAbstractItemModel *ignoreCategoriesModel = nullptr;
    QString filterString;
};

DebugModelFilter::DebugModelFilter(): d(std::make_unique<DebugModelFilter::Private>())
{
}

DebugModelFilter::~DebugModelFilter() = default;

bool DebugModelFilter::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_parent);

    auto *currModel = qobject_cast<DebugModel *>(sourceModel());

    // TODO: Less String comparisons;
    const QString type =
        currModel->data(currModel->index(source_row, DebugModel::Columns::e_TYPE), Qt::DisplayRole).toString();
    if (d->filterDebug && type == tr("Debug")) {
        return false;
    }
    if (d->filterCritical && type == tr("Critical")) {
        return false;
    }
    if (d->filterFatal && type == tr("Fatal")) {
        return false;
    }
    if (d->filterInfo && type == tr("Info")) {
        return false;
    }
    if (d->filterWarning && type == tr("Warning")) {
        return false;
    }

    if (d->ignoreCategoriesModel) {
        for (int i = 0, end = d->ignoreCategoriesModel->rowCount(); i < end; i++) {
            const QModelIndex ignoreIdx = d->ignoreCategoriesModel->index(i, 0);
            const QString categoryToIgnore = d->ignoreCategoriesModel->data(ignoreIdx, Qt::DisplayRole).toString();
            const QModelIndex categoryIdx = currModel->index(source_row, DebugModel::e_CATEGORY);
            const QString currCategory = currModel->data(categoryIdx, Qt::DisplayRole).toString();

            // TODO: Allow Partial Matches?
            if (categoryToIgnore == currCategory) {
                return false;
            }
        }
    }

    if (d->filterString.size()) {
        const QModelIndex messageIdx = currModel->index(source_row, DebugModel::Columns::e_MESSAGE);
        const QString currMessage = currModel->data(messageIdx, Qt::DisplayRole).toString();
        const bool lineContainsFilter = currMessage.contains(d->filterString);

        if (d->invertStringFilter) {
            return !lineContainsFilter;
        }
        return lineContainsFilter;
    }

    return true;
}

bool DebugModelFilter::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    Q_UNUSED(source_left);
    Q_UNUSED(source_right);
    return true;
}

void DebugModelFilter::setFilterDebug(bool filter)
{
    if (d->filterDebug == filter) {
        return;
    }
    d->filterDebug = filter;
    invalidateFilter();
}

void DebugModelFilter::setFilterWarning(bool filter)
{
    if (d->filterWarning == filter) {
        return;
    }
    d->filterWarning = filter;
    invalidateFilter();
}

void DebugModelFilter::setFilterInfo(bool filter)
{
    if (d->filterInfo == filter) {
        return;
    }
    d->filterInfo = filter;
    invalidateFilter();
}

void DebugModelFilter::setFilterCrtical(bool filter)
{
    if (d->filterCritical == filter) {
        return;
    }
    d->filterCritical = filter;
    invalidateFilter();
}

void DebugModelFilter::setFilterFatal(bool filter)
{
    if (d->filterFatal == filter) {
        return;
    }
    d->filterFatal = filter;
    invalidateFilter();
}

void DebugModelFilter::setIgnoreCategoriesModel(QAbstractItemModel *model)
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

void DebugModelFilter::setFilterString(const QString& str)
{
    if (d->filterString == str) {
        return;
    }
    d->filterString = str;
    invalidateFilter();
}

void DebugModelFilter::setInvertMessageFilter(bool value)
{
    if (d->invertStringFilter == value) {
        return;
    }
    d->invertStringFilter = value;
    invalidateFilter();
}

} // namespace Codethink::lvtmdl
