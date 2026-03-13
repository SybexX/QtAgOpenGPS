// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Camera settings for FieldViewItem - grouped property for QML

#ifndef CAMERAPROPERTIES_H
#define CAMERAPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>

class CameraProperties : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged BINDABLE bindableZoom)
    Q_PROPERTY(double x READ x WRITE setX NOTIFY xChanged BINDABLE bindableX)
    Q_PROPERTY(double y READ y WRITE setY NOTIFY yChanged BINDABLE bindableY)
    Q_PROPERTY(double rotation READ rotation WRITE setRotation NOTIFY rotationChanged BINDABLE bindableRotation)
    Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY pitchChanged BINDABLE bindablePitch)
    Q_PROPERTY(double fov READ fov WRITE setFov NOTIFY fovChanged BINDABLE bindableFov)

public:
    explicit CameraProperties(QObject *parent = nullptr);

    double zoom() const;
    void setZoom(double value);
    QBindable<double> bindableZoom();

    double x() const;
    void setX(double value);
    QBindable<double> bindableX();

    double y() const;
    void setY(double value);
    QBindable<double> bindableY();

    double rotation() const;
    void setRotation(double value);
    QBindable<double> bindableRotation();

    double pitch() const;
    void setPitch(double value);
    QBindable<double> bindablePitch();

    double fov() const;
    void setFov(double value);
    QBindable<double> bindableFov();

signals:
    void zoomChanged();
    void xChanged();
    void yChanged();
    void rotationChanged();
    void pitchChanged();
    void fovChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_zoom, 15.0, &CameraProperties::zoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_x, 0.0, &CameraProperties::xChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_y, 0.0, &CameraProperties::yChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_rotation, 0.0, &CameraProperties::rotationChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_pitch, -20.0, &CameraProperties::pitchChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CameraProperties, double, m_fov, 40.1, &CameraProperties::fovChanged)
};

#endif // CAMERAPROPERTIES_H
