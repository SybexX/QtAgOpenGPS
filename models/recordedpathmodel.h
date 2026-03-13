#ifndef RECORDEDPATHMODEL_H
#define RECORDEDPATHMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QList>
#include <QtQml/qqml.h>

class RecordedPathModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum PathRoles {
        IndexRole = Qt::UserRole + 1,
        NameRole
    };

    explicit RecordedPathModel(QObject *parent = nullptr);

    // Model interface
    Q_INVOKABLE void addPath(int index, const QString &name);
    Q_INVOKABLE void removePath(int row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_paths.count(); }

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct PathRecord {
        int index;
        QString name;
    };

    QList<PathRecord> m_paths;
};

#endif // RECORDEDPATHMODEL_H
