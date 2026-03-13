// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef TOOLSWITHSECTIONSMODEL_H
#define TOOLSWITHSECTIONSMODEL_H

#include <QAbstractListModel>
#include <QVector>

class ToolsWithSectionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ToolIndexRole = Qt::UserRole + 1
    };
    Q_ENUM(Roles)

    explicit ToolsWithSectionsModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management - maintains sorted order
    void addToolIndex(int toolIndex);
    void removeToolIndex(int toolIndex);
    bool containsToolIndex(int toolIndex) const;
    void clear();

    // Utility
    int count() const { return m_toolIndices.count(); }
    int toolIndexAt(int row) const;

signals:
    void toolAdded(int toolIndex);
    void toolRemoved(int toolIndex);

private:
    // Returns the insertion position to maintain sorted order
    int findInsertPosition(int toolIndex) const;

    QVector<int> m_toolIndices;
};

#endif // TOOLSWITHSECTIONSMODEL_H
