#include <QCoreApplication>
#include "headacheinterface.h"

HeadacheInterface *HeadacheInterface::s_instance = nullptr;
QMutex HeadacheInterface::s_mutex;
bool HeadacheInterface::s_cpp_created = false;

HeadacheInterface::HeadacheInterface(QObject *parent)
    : QObject{parent}
{
    m_boundaryLineModel = new FenceLineModel(this);
    m_headacheLineModel = new FenceLineModel(this);
}

HeadacheInterface *HeadacheInterface::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new HeadacheInterface();
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                         delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

HeadacheInterface *HeadacheInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new HeadacheInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
