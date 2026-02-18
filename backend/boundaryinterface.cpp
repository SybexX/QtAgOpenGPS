#include <QCoreApplication>
#include "boundaryinterface.h"
#include "boundariesproperties.h"
#include "mainwindowstate.h"

BoundaryInterface *BoundaryInterface::s_instance = nullptr;
QMutex BoundaryInterface::s_mutex;
bool BoundaryInterface::s_cpp_created = false;

BoundaryInterface::BoundaryInterface(QObject *parent)
    : QObject{parent}
    , m_properties(new BoundariesProperties(this))
{
    auto updateBoundaryDrawing = [this]() {
        if (this->m_isDrawRightSide) {
            //positive distance
            this->properties()->set_markBoundary(this->m_createBndOffset);
        } else {
            //negative distance
            this->properties()->set_markBoundary(-this->m_createBndOffset);
        }
    };
    connect(this, &BoundaryInterface::isDrawRightSideChanged, this, updateBoundaryDrawing);
    connect(this, &BoundaryInterface::createBndOffsetChanged, this, updateBoundaryDrawing);
    //headland should only show if button is pressed in the main window
    connect(MainWindowState::instance(), &MainWindowState::isHeadlandOnChanged, this, [this]() {
        if (properties()->hdLine()) {
            properties()->hdLine()->set_visible(MainWindowState::instance()->isHeadlandOn());
            emit properties()->hdLineChanged();
        }
    });
}

BoundaryInterface *BoundaryInterface::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new BoundaryInterface();
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                         delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

BoundaryInterface *BoundaryInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new BoundaryInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
