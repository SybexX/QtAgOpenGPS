// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Boundary properties for FieldViewItem - represents a single boundary

#ifndef BOUNDARYPROPERTIES_H
#define BOUNDARYPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QVector3D>
#include <QList>
#include <QtQml/qqmlregistration.h>

#include "simpleproperty.h"

class BoundaryProperties : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BoundaryLine)

public:
    explicit BoundaryProperties(QObject *parent = nullptr);

    SIMPLE_BINDABLE_PROPERTY(bool, visible)
    SIMPLE_BINDABLE_PROPERTY(QList<QVector3D>, points)

signals:
    void boundaryChanged();
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryProperties, bool, m_visible, false, &BoundaryProperties::visibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BoundaryProperties, QList<QVector3D>, m_points, &BoundaryProperties::pointsChanged)
};

#endif // BOUNDARYPROPERTIES_H
