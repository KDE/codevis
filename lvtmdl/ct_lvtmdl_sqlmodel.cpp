#include <ct_lvtmdl_sqlmodel.h>
#include <iostream>
#include <qnamespace.h>

namespace Codethink::lvtmdl {
SqlModel::SqlModel(lvtldr::NodeStorage& nodeStorage): nodeStorage(nodeStorage)
{
}

int SqlModel::columnCount(const QModelIndex& parent) const
{
    if (tables.size()) {
        return tables[0].size();
    }

    return 0;
}

int SqlModel::rowCount(const QModelIndex& parent) const
{
    return tables.size();
}

QVariant SqlModel::data(const QModelIndex& idx, int role) const
{
    if (idx.row() >= rowCount()) {
        return {};
    }
    if (idx.column() >= columnCount()) {
        return {};
    }
    if (role != Qt::DisplayRole) {
        return {};
    }

    auto data = tables[idx.row()][idx.column()];

    return QString::fromStdString(data);
}

QVariant SqlModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section > columnCount()) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    return QString::fromStdString(hData[section]);
}

void SqlModel::setQuery(const QString& query)
{
    auto res = nodeStorage.rawDbQuery(query.toStdString());
    if (res.has_error()) {
        Q_EMIT invalidQueryTriggered(QString::fromStdString(res.error().what), query);
        std::cout << res.error().what << std::endl;
        return;
    }

    auto values = res.value();
    beginResetModel();
    tables = values.data;
    hData = values.columns;
    endResetModel();

    for (int i = 0, end = columnCount(); i < end; i++) {
        auto colName = QString::fromStdString(values.columns[i]);
        std::cout << colName.toStdString() << std::endl;
        setHeaderData(i, Qt::Horizontal, colName);
    }
}
} // namespace Codethink::lvtmdl
