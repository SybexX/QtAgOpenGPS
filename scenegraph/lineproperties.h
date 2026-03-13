// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Line properties for FieldViewItem - grouped property for QML

#ifndef LINEPROPERTIES_H
#define LINEPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QColor>
#include <QVector3D>
#include <QVector>
#include <QQmlEngine>

#include "simpleproperty.h"

class LineProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Line)
public:
    explicit LineProperties(QObject *parent = nullptr);

    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, points)
    SIMPLE_BINDABLE_PROPERTY(float, width)
    SIMPLE_BINDABLE_PROPERTY(bool, dashed)
    SIMPLE_BINDABLE_PROPERTY(bool, loop)
    SIMPLE_BINDABLE_PROPERTY(QColor, color)

signals:
    void lineChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LineProperties, QVector<QVector3D>, m_points, QVector<QVector3D>(), &LineProperties::pointsChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LineProperties, float, m_width, 1.0f, &LineProperties::widthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LineProperties, bool, m_dashed, false, &LineProperties::dashedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LineProperties, bool, m_loop, false, &LineProperties::loopChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LineProperties, QColor, m_color, QColor(0, 0, 0), &LineProperties::colorChanged)
};

#endif // LINEPROPERTIES_H
