// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "fieldsmodel.h"
#include <QDir>
#include <QFileInfo>
#include "cboundary.h"

FieldsModel::FieldsModel(QObject *parent)
    : QAbstractListModel(parent)
{
#ifdef Q_OS_ANDROID
    connect(&m_pollTimer, &QTimer::timeout,
            this, &FieldsModel::checkForChanges);
#else
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FieldsModel::checkForChanges);
    connect(&m_boundaryWatcher, &QFileSystemWatcher::fileChanged,
            this, &FieldsModel::onBoundaryFileChanged);
    connect(&m_subdirWatcher, &QFileSystemWatcher::directoryChanged,
            this, &FieldsModel::onSubdirectoryChanged);
#endif
}

int FieldsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_fields.count();
}

QVariant FieldsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_fields.count())
        return QVariant();

    const Field &field = m_fields[index.row()];

    switch (role) {
    case NameRole:
        return field.name;
    case LatitudeRole:
        return field.latitude;
    case LongitudeRole:
        return field.longitude;
    case HasBoundaryRole:
        return field.hasBoundary;
    case BoundaryAreaRole:
        return field.boundaryArea;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FieldsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[LatitudeRole] = "latitude";
    roles[LongitudeRole] = "longitude";
    roles[HasBoundaryRole] = "hasBoundary";
    roles[BoundaryAreaRole] = "boundaryArea";
    return roles;
}

void FieldsModel::setFields(const QVector<Field> &fields)
{
    beginResetModel();
    m_fields = fields;
    endResetModel();
}

void FieldsModel::addField(const Field &field)
{
    beginInsertRows(QModelIndex(), m_fields.count(), m_fields.count());
    m_fields.append(field);
    endInsertRows();
}

void FieldsModel::removeField(int index)
{
    if (index < 0 || index >= m_fields.count())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_fields.remove(index);
    endRemoveRows();
}

void FieldsModel::clear()
{
    beginResetModel();
    m_fields.clear();
    endResetModel();
}

FieldsModel::Field FieldsModel::fieldAt(int index) const
{
    if (index >= 0 && index < m_fields.count())
        return m_fields[index];
    return Field();
}

void FieldsModel::watchDirectory(const QString &path)
{
#ifdef Q_OS_ANDROID
    m_pollTimer.stop();
    m_boundaryFileTimestamps.clear();
#else
    // Remove any previously watched directory
    if (!m_watchedPath.isEmpty()) {
        m_watcher.removePath(m_watchedPath);
    }
    // Clear existing boundary watches
    QStringList watchedFiles = m_boundaryWatcher.files();
    if (!watchedFiles.isEmpty()) {
        m_boundaryWatcher.removePaths(watchedFiles);
    }
    // Clear existing subdirectory watches
    QStringList watchedDirs = m_subdirWatcher.directories();
    if (!watchedDirs.isEmpty()) {
        m_subdirWatcher.removePaths(watchedDirs);
    }
#endif

    m_watchedPath = path;
    m_dirsWithoutBoundary.clear();

    refreshDirectories();

#ifdef Q_OS_ANDROID
    m_pollTimer.start(POLL_INTERVAL_MS);
#else
    m_watcher.addPath(path);
#endif
}

void FieldsModel::refreshDirectories()
{
    if (m_watchedPath.isEmpty())
        return;

    QDir dir(m_watchedPath);
    m_knownDirectories = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // Set up boundary watches for all directories
    for (const QString &dirName : m_knownDirectories) {
        QString fullPath = dir.absoluteFilePath(dirName);
        watchBoundaryFile(fullPath);
    }
}

void FieldsModel::checkForChanges()
{
    if (m_watchedPath.isEmpty())
        return;

    QDir dir(m_watchedPath);
    QStringList currentDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // Find new directories
    for (const QString &dirName : currentDirs) {
        if (!m_knownDirectories.contains(dirName)) {
            QString fullPath = dir.absoluteFilePath(dirName);
            m_knownDirectories.append(dirName);
            watchBoundaryFile(fullPath);
            onDirectoryAdded(fullPath);
        }
    }

#ifdef Q_OS_ANDROID
    // Poll boundary files for changes
    for (auto it = m_boundaryFileTimestamps.begin(); it != m_boundaryFileTimestamps.end(); ++it) {
        QFileInfo fileInfo(it.key());
        if (fileInfo.exists() && fileInfo.lastModified() != it.value()) {
            it.value() = fileInfo.lastModified();
            onBoundaryFileChanged(it.key());
        }
    }

    // Check for Boundary.txt creation in directories that didn't have it
    QStringList dirsToCheck = m_dirsWithoutBoundary;
    for (const QString &dirPath : dirsToCheck) {
        QString boundaryPath = findBoundaryFile(dirPath);
        if (!boundaryPath.isEmpty()) {
            QFileInfo fileInfo(boundaryPath);
            m_dirsWithoutBoundary.removeAll(dirPath);
            m_boundaryFileTimestamps.insert(boundaryPath, fileInfo.lastModified());
            onBoundaryFileCreated(boundaryPath);
        }
    }
#endif
}

QString FieldsModel::findBoundaryFile(const QString &directoryPath) const
{
    QDir dir(directoryPath);
    const QStringList entries = dir.entryList(QDir::Files);

    for (const QString &entry : entries) {
        if (entry.compare(QStringLiteral("Boundary.txt"), Qt::CaseInsensitive) == 0) {
            return dir.absoluteFilePath(entry);
        }
    }
    return QString();
}

void FieldsModel::watchBoundaryFile(const QString &directoryPath)
{
    QString boundaryPath = findBoundaryFile(directoryPath);

    if (!boundaryPath.isEmpty()) {
        QFileInfo fileInfo(boundaryPath);
#ifdef Q_OS_ANDROID
        m_boundaryFileTimestamps.insert(boundaryPath, fileInfo.lastModified());
#else
        m_boundaryWatcher.addPath(boundaryPath);
#endif
        // Remove from dirs without boundary if it was there
        m_dirsWithoutBoundary.removeAll(directoryPath);
    } else {
        // Track this directory to watch for Boundary.txt creation
        if (!m_dirsWithoutBoundary.contains(directoryPath)) {
            m_dirsWithoutBoundary.append(directoryPath);
#ifndef Q_OS_ANDROID
            m_subdirWatcher.addPath(directoryPath);
#endif
        }
    }
}

void FieldsModel::onBoundaryFileChanged(const QString &path)
{
    double area = CBoundary::getSavedFieldArea(path);
}

#ifndef Q_OS_ANDROID
void FieldsModel::onSubdirectoryChanged(const QString &path)
{
    // Check if Boundary.txt was created in this directory
    QString boundaryPath = findBoundaryFile(path);

    if (!boundaryPath.isEmpty() && m_dirsWithoutBoundary.contains(path)) {
        m_dirsWithoutBoundary.removeAll(path);
        m_subdirWatcher.removePath(path);
        m_boundaryWatcher.addPath(boundaryPath);
        onBoundaryFileCreated(boundaryPath);
    }
}
#endif

void FieldsModel::onBoundaryFileCreated(const QString &path)
{
    // TODO: Flesh out this method
    Q_UNUSED(path);
}

void FieldsModel::onDirectoryAdded(const QString &path)
{
    // TODO: Flesh out this method
    Q_UNUSED(path);
}
