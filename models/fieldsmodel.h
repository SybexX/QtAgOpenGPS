// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef FIELDSMODEL_H
#define FIELDSMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVector>

#ifdef Q_OS_ANDROID
#include <QTimer>
#else
#include <QFileSystemWatcher>
#endif

class FieldsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        LatitudeRole,
        LongitudeRole,
        HasBoundaryRole,
        BoundaryAreaRole
    };
    Q_ENUM(Roles)

    struct Field {
        QString name;
        double latitude;
        double longitude;
        bool hasBoundary;
        double boundaryArea;
    };

    explicit FieldsModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setFields(const QVector<Field> &fields);
    void addField(const Field &field);
    void removeField(int index);
    void clear();

    // Utility
    int count() const { return m_fields.count(); }
    Field fieldAt(int index) const;

    // Directory watching
    void watchDirectory(const QString &path);
    void refreshDirectories();

private slots:
    void checkForChanges();
    void onBoundaryFileChanged(const QString &path);
#ifndef Q_OS_ANDROID
    void onSubdirectoryChanged(const QString &path);
#endif

private:
    void onDirectoryAdded(const QString &path);
    void watchBoundaryFile(const QString &directoryPath);
    void onBoundaryFileCreated(const QString &path);
    QString findBoundaryFile(const QString &directoryPath) const;

    QVector<Field> m_fields;
    QString m_watchedPath;
    QStringList m_knownDirectories;
    QStringList m_dirsWithoutBoundary;

#ifdef Q_OS_ANDROID
    QTimer m_pollTimer;
    static constexpr int POLL_INTERVAL_MS = 2000;
    QHash<QString, QDateTime> m_boundaryFileTimestamps;
#else
    QFileSystemWatcher m_watcher;
    QFileSystemWatcher m_boundaryWatcher;
    QFileSystemWatcher m_subdirWatcher;
#endif
};

#endif // FIELDSMODEL_H
