#include "recordedpathmodel.h"

RecordedPathModel::RecordedPathModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void RecordedPathModel::addPath(int index, const QString &name)
{
    beginInsertRows(QModelIndex(), m_paths.count(), m_paths.count());
    m_paths.append({index, name});
    endInsertRows();
}

void RecordedPathModel::removePath(int row)
{
    if (row < 0 || row >= m_paths.count())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_paths.removeAt(row);
    endRemoveRows();
}

void RecordedPathModel::clear()
{
    if (m_paths.isEmpty())
        return;

    beginResetModel();
    m_paths.clear();
    endResetModel();
}

int RecordedPathModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_paths.count();
}

QVariant RecordedPathModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_paths.count())
        return QVariant();

    const PathRecord &path = m_paths[index.row()];

    switch (role) {
    case IndexRole:
        return path.index;
    case NameRole:
        return path.name;
    }

    return QVariant();
}

QHash<int, QByteArray> RecordedPathModel::roleNames() const
{
    return {
        {IndexRole, "pathIndex"},
        {NameRole, "pathName"}
    };
}
