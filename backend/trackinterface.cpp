#include <QCoreApplication>
#include "trackinterface.h"

TrackInterface *TrackInterface::s_instance = nullptr;
QMutex TrackInterface::s_mutex;
bool TrackInterface::s_cpp_created = false;

TrackInterface::TrackInterface(QObject *parent)
    : QObject{parent}
{
    m_boundaryLineModel = new FenceLineModel(this);
}

TrackInterface *TrackInterface::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new TrackInterface();
        s_cpp_created = true;
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                         delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

TrackInterface *TrackInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new TrackInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
