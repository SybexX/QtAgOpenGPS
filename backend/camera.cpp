// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later

#include "camera.h"
#include "settingsmanager.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include "backend.h"

Q_LOGGING_CATEGORY(camera_log, "camera.qtagopengps")

Camera *Camera::s_instance = nullptr;
QMutex Camera::s_mutex;
bool Camera::s_cpp_created = false;

Camera::Camera(QObject *parent)
    : QObject(parent)
{
    m_camFollowing = true;

    connect(Backend::instance(), &Backend::zoomIn, this, &Camera::zoomIn);
    connect(Backend::instance(), &Backend::zoomOut, this, &Camera::zoomOut);
    connect(Backend::instance(), &Backend::tiltDown, this, &Camera::tiltDown);
    connect(Backend::instance(), &Backend::tiltUp, this, &Camera::tiltUp);
    connect(Backend::instance(), &Backend::view2D, this, &Camera::view2D);
    connect(Backend::instance(), &Backend::view3D, this, &Camera::view3D);
    connect(Backend::instance(), &Backend::normal2D, this, &Camera::normal2D);
    connect(Backend::instance(), &Backend::normal3D, this, &Camera::normal3D);
}

Camera *Camera::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Camera();
        qDebug(camera_log) << "Camera singleton created by C++ code.";
        s_cpp_created = true;

        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

Camera *Camera::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new Camera();
        qDebug(camera_log) << "Camera singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void Camera::loadSettings()
{
}

void Camera::SetWorldCam(QMatrix4x4 &modelview,
                         double _fixPosX, double _fixPosY)
{
    // Back the camera up
    modelview.translate(0, 0, m_camSetDistance * 0.5);

    // Rotate the camera down to look at fix
    modelview.rotate(static_cast<float>(Camera::camPitch()), 1.0f, 0.0f, 0.0f);

    // Pan if set
    modelview.translate(static_cast<float>(m_panX), static_cast<float>(m_panY), 0.0f);

    // Following game style or N fixed cam
    if (m_camFollowing) {
        modelview.rotate(static_cast<float>(m_camHeading), 0.0f, 0.0f, 1.0f);
        modelview.translate(static_cast<float>(-_fixPosX),
                           static_cast<float>(-_fixPosY),
                           static_cast<float>(-0));
    } else {
        modelview.translate(static_cast<float>(-_fixPosX),
                           static_cast<float>(-_fixPosY),
                           static_cast<float>(-0));
    }
}

void Camera::SetZoom()
{
    // Match grid to cam distance and redo perspective
    if (m_camSetDistance <= -20000) m_gridZoom = 2000;
    else if (m_camSetDistance >= -20000 && m_camSetDistance < -10000) m_gridZoom = 2012;
    else if (m_camSetDistance >= -10000 && m_camSetDistance < -5000) m_gridZoom = 1006;
    else if (m_camSetDistance >= -5000 && m_camSetDistance < -2000) m_gridZoom = 503;
    else if (m_camSetDistance >= -2000 && m_camSetDistance < -1000) m_gridZoom = 201.2;
    else if (m_camSetDistance >= -1000 && m_camSetDistance < -500) m_gridZoom = 100.6;
    else if (m_camSetDistance >= -500 && m_camSetDistance < -250) m_gridZoom = 50.3;
    else if (m_camSetDistance >= -250 && m_camSetDistance < -150) m_gridZoom = 25.15;
    else if (m_camSetDistance >= -150 && m_camSetDistance < -50) m_gridZoom = 10.06;
    else if (m_camSetDistance >= -50 && m_camSetDistance < -1) m_gridZoom = 5.03;

    emit gridZoomChanged();
}

void Camera::tiltDown() {
    double camPitch = Camera::camPitch(); //shortcut to SettingsManager

    if (camPitch > -59) camPitch = -60;
    camPitch += ((camPitch * 0.012) - 1);
    if (camPitch < -76) camPitch = -76;

    Camera::setCamPitch(camPitch);

    emit updateView();
    //if (openGLControl) {
    //    QMetaObject::invokeMethod(openGLControl, "update", Qt::QueuedConnection);
    //}
}

void Camera::tiltUp() {
    double camPitch = Camera::camPitch();

    camPitch -= ((camPitch * 0.012) - 1);
    if (camPitch > -58) camPitch = 0;

    Camera::setCamPitch(camPitch);
    emit updateView();
}

void Camera::view2D() {
    set_camFollowing (true);
    Camera::setCamPitch(0);
}

void Camera::view3D() {
    set_camFollowing (true);
    Camera::setCamPitch(-65);
}

void Camera::normal2D() {
    set_camFollowing (false);
    Camera::setCamPitch(0);
}

void Camera::normal3D() {
    Camera::setCamPitch(-65);
    set_camFollowing (false);
}

void Camera::zoomIn() {
    double zoomValue = Camera::zoomValue();
    if (zoomValue <= 20) {
        if ((zoomValue -= zoomValue * 0.1) < 3.0)
            zoomValue = 3.0;
    } else {
        if ((zoomValue -= zoomValue * 0.05) < 3.0)
            zoomValue = 3.0;
    }
    set_camSetDistance(zoomValue * zoomValue * -1);

    Camera::setZoomValue(zoomValue);

    SetZoom();
    emit updateView();
}

void Camera::zoomOut() {
    double zoomValue = Camera::zoomValue();

    if (zoomValue <= 20) zoomValue += zoomValue * 0.1;
    else zoomValue += zoomValue * 0.05;
    if (zoomValue > 220) zoomValue = 220;
    set_camSetDistance(zoomValue * zoomValue * -1);

    Camera::setZoomValue(zoomValue);

    SetZoom();
    emit updateView();
}

