#include "qmlblockage.h"

qmlblockage::qmlblockage(QObject *parent)
    : QObject{parent}
{}
void qmlblockage::onRowsUpdated(void) {
    //if QML updated things we need to re-read the items
    needRead = true;
    qDebug() << "Connected to blockage";
}
