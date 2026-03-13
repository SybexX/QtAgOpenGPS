#include <QCoreApplication>
#include "headlandinterface.h"

HeadlandInterface *HeadlandInterface::s_instance = nullptr;
QMutex HeadlandInterface::s_mutex;
bool HeadlandInterface::s_cpp_created = false;

HeadlandInterface::HeadlandInterface(QObject *parent)
    : QObject{parent}
{
    m_boundaryLineModel = new FenceLineModel(this);
}

HeadlandInterface *HeadlandInterface::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new HeadlandInterface();
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                         delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

HeadlandInterface *HeadlandInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new HeadlandInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
