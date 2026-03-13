// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "toolswithsectionsmodel.h"
#include <algorithm>

ToolsWithSectionsModel::ToolsWithSectionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ToolsWithSectionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_toolIndices.count();
}

QVariant ToolsWithSectionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_toolIndices.count())
        return QVariant();

    if (role == ToolIndexRole || role == Qt::DisplayRole)
        return m_toolIndices[index.row()];

    return QVariant();
}

QHash<int, QByteArray> ToolsWithSectionsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ToolIndexRole] = "toolIndex";
    return roles;
}

int ToolsWithSectionsModel::findInsertPosition(int toolIndex) const
{
    // Binary search for insertion position to maintain sorted order
    auto it = std::lower_bound(m_toolIndices.begin(), m_toolIndices.end(), toolIndex);
    return static_cast<int>(it - m_toolIndices.begin());
}

void ToolsWithSectionsModel::addToolIndex(int toolIndex)
{
    // Check if already exists
    if (containsToolIndex(toolIndex))
        return;

    int pos = findInsertPosition(toolIndex);

    beginInsertRows(QModelIndex(), pos, pos);
    m_toolIndices.insert(pos, toolIndex);
    endInsertRows();

    emit toolAdded(toolIndex);
}

void ToolsWithSectionsModel::removeToolIndex(int toolIndex)
{
    int pos = findInsertPosition(toolIndex);

    // Check if the tool index actually exists at this position
    if (pos >= m_toolIndices.count() || m_toolIndices[pos] != toolIndex)
        return;

    beginRemoveRows(QModelIndex(), pos, pos);
    m_toolIndices.removeAt(pos);
    endRemoveRows();

    // Decrement all subsequent indices (they shift down when a tool is removed)
    for (int i = pos; i < m_toolIndices.count(); ++i) {
        m_toolIndices[i]--;
    }

    // Notify that data changed for all remaining items after the removed position
    if (pos < m_toolIndices.count()) {
        emit dataChanged(index(pos), index(m_toolIndices.count() - 1), {ToolIndexRole});
    }

    emit toolRemoved(toolIndex);
}

bool ToolsWithSectionsModel::containsToolIndex(int toolIndex) const
{
    // Use binary search since the list is sorted
    return std::binary_search(m_toolIndices.begin(), m_toolIndices.end(), toolIndex);
}

void ToolsWithSectionsModel::clear()
{
    beginResetModel();
    m_toolIndices.clear();
    endResetModel();
}

int ToolsWithSectionsModel::toolIndexAt(int row) const
{
    if (row < 0 || row >= m_toolIndices.count())
        return -1;
    return m_toolIndices[row];
}
