// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef BLOCKAGEMODEL_H
#define BLOCKAGEMODEL_H

#include <QAbstractListModel>
#include <QVector>

class BlockageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        CountRole
    };
    Q_ENUM(Roles)

    struct Row {
        int index;
        int count;
    };

    explicit BlockageModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setRows(const QVector<Row> &new_rows);
    void addRow(const Row &row);
    void removeRowAt(int at_index);
    void clear();

    void setCount(int at_index, int count);
    void zeroCounts();

    // Utility
    int count() const { return rows.count(); }
    Row rowAt(int at_index) const;

    QVector<Row> rows;

private:
};

#endif // BLOCKAGEMODEL_H
