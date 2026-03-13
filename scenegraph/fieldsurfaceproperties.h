// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Field surface properties for FieldViewItem - grouped property for QML

#ifndef FIELDSURFACEPROPERTIES_H
#define FIELDSURFACEPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QColor>

#include "simpleproperty.h"

class FieldSurfaceProperties : public QObject
{
    Q_OBJECT
public:
    explicit FieldSurfaceProperties(QObject *parent = nullptr);

    SIMPLE_BINDABLE_PROPERTY(bool, visible)
    SIMPLE_BINDABLE_PROPERTY(bool, showTexture)
    SIMPLE_BINDABLE_PROPERTY(QColor, color)

signals:
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldSurfaceProperties, bool, m_visible, true, &FieldSurfaceProperties::visibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldSurfaceProperties, bool, m_showTexture, true, &FieldSurfaceProperties::showTextureChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FieldSurfaceProperties, QColor, m_color, QColor(0,0,0), &FieldSurfaceProperties::colorChanged)
};

#endif // FIELDSURFACEPROPERTIES_H
