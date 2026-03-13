// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef FLAGMODEL_H
#define FLAGMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QVector>

class FlagModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        ColorRole,
        LatitudeRole,
        LongitudeRole,
        HeadingRole,
        EastingRole,
        NorthingRole,
        NotesRole
    };
    Q_ENUM(Roles)

    struct Flag {
        int id;
        int color;
        double latitude;
        double longitude;
        double heading;
        double easting;
        double northing;
        QString notes;
    };

    explicit FlagModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setFlags(const QVector<Flag> &flags);
    void addFlag(const Flag &flag);
    void removeFlag(int id);
    void clear();

    void setNotes(int id, QString notes);
    void setColor(int id, int color);

    // Utility
    int count() const { return flags.count(); }
    Flag flagAt(int id) const;

    QVector<Flag> flags;

private:
};

#endif // FLAGMODEL_H
