#ifndef QMLUTIL_H
#define QMLUTIL_H

#include <QQuickItem>

static inline QObject *qmlItem(QObject *root, QString name)
{
    // ⚡ PHASE 6.3.0 SAFE ACCESS: Protect against NULL root or invalid objects
    if (!root) {
        qWarning() << "qmlItem: root is NULL for" << name;
        return nullptr;
    }

    QObject *result = root->findChild<QObject *>(name);
    if (!result) {
        qDebug() << "qmlItem: Object not found:" << name;
    }

    return result;
}

#endif // QMLUTIL_H
