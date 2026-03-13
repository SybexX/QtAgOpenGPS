// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "fencelinemodel.h"

FenceLineModel::FenceLineModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FenceLineModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_boundaries.count();
}

QVariant FenceLineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_boundaries.count())
        return QVariant();

    const FenceLine &line = m_boundaries[index.row()];

    switch (role) {
    case IndexRole:
        return line.index;
    case ColorRole:
        return line.color;
    case WidthRole:
        return line.width;
    case PointsRole:
        return line.points;
    case DashedRole:
        return line.dashed;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FenceLineModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "boundaryIndex";
    roles[ColorRole] = "color";
    roles[WidthRole] = "width";
    roles[PointsRole] = "points";
    roles[DashedRole] = "dashed";
    return roles;
}

void FenceLineModel::setFenceLines(const QVector<FenceLine> &lines)
{
    beginResetModel();
    m_boundaries = lines;
    endResetModel();
}

void FenceLineModel::clear()
{
    beginResetModel();
    m_boundaries.clear();
    endResetModel();
}
