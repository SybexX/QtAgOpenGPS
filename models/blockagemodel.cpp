// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "blockagemodel.h"

BlockageModel::BlockageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BlockageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return rows.count();
}

QVariant BlockageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rows.count())
        return QVariant();

    const Row &row = rows[index.row()];

    switch (role) {
    case IndexRole:
        return row.index;
    case CountRole:
        return row.count;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> BlockageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "secNumber";
    roles[CountRole] = "count";
    return roles;
}

void BlockageModel::setRows(const QVector<Row> &new_rows)
{
    beginResetModel();

    rows = new_rows;
    //force secNumber to align with real index number
    for (int i = 0 ; i < rows.count(); i++) {
        rows[i].index = i;
    }
    endResetModel();
}

void BlockageModel::addRow(const Row &row)
{
    beginInsertRows(QModelIndex(), rows.count(), rows.count());

    //make the secNumber the same as the actual index number
    Row new_row = row;
    new_row.index = rows.count();
    rows.append(new_row);

    endInsertRows();
}

void BlockageModel::removeRowAt(int at_index)
{
    beginRemoveRows(QModelIndex(), at_index, at_index);
    rows.removeAt(at_index);
    endRemoveRows();

    //adjust secNumbers to match real index number
    for (int i = 0; i < rows.count(); i++) {
        if (rows[i].index != i) {
            rows[i].index = i;
            QModelIndex idx = index(at_index);
            emit dataChanged(idx, idx, {CountRole});
        }
    }
}

void BlockageModel::zeroCounts() {
    for (int i=0; i < rows.count(); i++) {
        setCount(i,0);
    }
}

void BlockageModel::clear()
{
    beginResetModel();
    rows.clear();
    endResetModel();
}

void BlockageModel::setCount(int at_index, int count)
{
    rows[at_index].index = at_index;
    rows[at_index].count = count;
    QModelIndex idx = index(at_index);
    emit dataChanged(idx, idx, {CountRole});
}

BlockageModel::Row BlockageModel::rowAt(int at_index) const
{
    if (rows.count() > at_index)
        return rows[at_index];
    return Row();
}
