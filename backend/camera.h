// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Camera singleton - manages camera position and view transformation

#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QMatrix4x4>
#include <QQmlEngine>
#include <QMutex>
#include <QProperty>
#include <QBindable>
#include "simpleproperty.h"
#include "settingsmanager.h"

//camPitch = SettingsManager::instance()->display_camPitch();
//zoomValue = SettingsManager::instance()->display_camZoom();
//camSmoothFactor = (SettingsManager::instance()->display_camSmooth() * 0.004) + 0.2;



class Camera : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

public:
    // Singleton access
    static Camera *instance();
    static Camera *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Load settings from SettingsManager
    Q_INVOKABLE void loadSettings();

    static inline double camPitch() { return SettingsManager::instance()->display_camPitch(); }
    static inline double zoomValue() { return SettingsManager::instance()->display_camZoom(); }
    static inline double camSmoothFactor() { return (SettingsManager::instance()->display_camSmooth() * 0.004) + 0.2; }

    static inline void setCamPitch(double new_pitch) {
        SettingsManager::instance()->setDisplay_camPitch(new_pitch);
    }
    static inline void setZoomValue(double new_zoom) {
        SettingsManager::instance()->setDisplay_camZoom(new_zoom);
    }


    // Calculate grid zoom based on camera distance
    Q_INVOKABLE void SetZoom();
    // Core camera transformation method
    void SetWorldCam(QMatrix4x4 &modelview, double _fixPosX, double _fixPosY);


    // ===== Bindable Properties (macro creates getter, setter, bindable, signal) =====
    SIMPLE_BINDABLE_PROPERTY(double, camYaw)
    SIMPLE_BINDABLE_PROPERTY(double, camHeading)
    SIMPLE_BINDABLE_PROPERTY(double, previousZoom)
    SIMPLE_BINDABLE_PROPERTY(double, panX)
    SIMPLE_BINDABLE_PROPERTY(double, panY)
    SIMPLE_BINDABLE_PROPERTY(double, offset)
    SIMPLE_BINDABLE_PROPERTY(double, camSetDistance)
    SIMPLE_BINDABLE_PROPERTY(double, gridZoom)
    SIMPLE_BINDABLE_PROPERTY(bool, camFollowing)
    SIMPLE_BINDABLE_PROPERTY(int, camMode)
public slots:
    //respond to UI events here
    void zoomIn();
    void zoomOut();
    void tiltDown();
    void tiltUp();
    void view2D();
    void view3D();
    void normal2D();
    void normal3D();

signals:
    //tell the GUI to redraw the field
    void updateView();


private:
    explicit Camera(QObject *parent = nullptr);
    ~Camera() override = default;

    // Prevent copying
    Camera(const Camera &) = delete;
    Camera &operator=(const Camera &) = delete;

    static Camera *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    // ===== Q_OBJECT_BINDABLE_PROPERTY backing storage =====
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_camYaw, 0, &Camera::camYawChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_camHeading, 0.0, &Camera::camHeadingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_previousZoom, 25, &Camera::previousZoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_panX, 0, &Camera::panXChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_panY, 0, &Camera::panYChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_offset, 0, &Camera::offsetChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_camSetDistance, -75, &Camera::camSetDistanceChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, double, m_gridZoom, 0, &Camera::gridZoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, bool, m_camFollowing, true, &Camera::camFollowingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Camera, int, m_camMode, 0, &Camera::camModeChanged)
};

#endif // CAMERA_H
