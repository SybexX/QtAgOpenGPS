#include <QCoreApplication>
#include "fieldinterface.h"

FieldInterface *FieldInterface::s_instance = nullptr;
QMutex FieldInterface::s_mutex;
bool FieldInterface::s_cpp_created = false;

FieldInterface::FieldInterface(QObject *parent)
    : QObject{parent}
{}

FieldInterface *FieldInterface::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new FieldInterface();
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                         delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

FieldInterface *FieldInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new FieldInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
