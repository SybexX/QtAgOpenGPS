#ifndef QMLBLOCKAGE_H
#define QMLBLOCKAGE_H

#include <QObject>

class qmlblockage : public QObject
{
    Q_OBJECT
public:
    explicit qmlblockage(QObject *parent = nullptr);

signals:
};

#endif // QMLBLOCKAGE_H
