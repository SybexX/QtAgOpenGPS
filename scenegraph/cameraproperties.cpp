// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Camera settings implementation

#include "cameraproperties.h"

CameraProperties::CameraProperties(QObject *parent)
    : QObject(parent)
{
}

double CameraProperties::zoom() const { return m_zoom; }
void CameraProperties::setZoom(double value) { m_zoom = value; }
QBindable<double> CameraProperties::bindableZoom() { return &m_zoom; }

double CameraProperties::x() const { return m_x; }
void CameraProperties::setX(double value) { m_x = value; }
QBindable<double> CameraProperties::bindableX() { return &m_x; }

double CameraProperties::y() const { return m_y; }
void CameraProperties::setY(double value) { m_y = value; }
QBindable<double> CameraProperties::bindableY() { return &m_y; }

double CameraProperties::rotation() const { return m_rotation; }
void CameraProperties::setRotation(double value) { m_rotation = value; }
QBindable<double> CameraProperties::bindableRotation() { return &m_rotation; }

double CameraProperties::pitch() const { return m_pitch; }
void CameraProperties::setPitch(double value) { m_pitch = value; }
QBindable<double> CameraProperties::bindablePitch() { return &m_pitch; }

double CameraProperties::fov() const { return m_fov; }
void CameraProperties::setFov(double value) { m_fov = value; }
QBindable<double> CameraProperties::bindableFov() { return &m_fov; }
