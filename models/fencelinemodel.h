// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef FENCELINEMODEL_H
#define FENCELINEMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QVariantList>
#include <QVector>

class FenceLineModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        ColorRole,
        WidthRole,
        PointsRole,
        DashedRole,
    };
    Q_ENUM(Roles)

    struct FenceLine {
        int index;
        QColor color;
        int width;
        QVariantList points; // List of QPoint
        bool dashed = false;
    };

    explicit FenceLineModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setFenceLines(const QVector<FenceLine> &lines);
    void clear();

private:
    QVector<FenceLine> m_boundaries;
};

#endif // FENCELINEMODEL_H
