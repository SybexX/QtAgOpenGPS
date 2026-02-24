// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Recorded path properties for FieldViewItem scene graph rendering

#ifndef RECORDEDPATHPROPERTIES_H
#define RECORDEDPATHPROPERTIES_H

#include <QObject>
#include <QQmlEngine>
#include <QBindable>
#include <QProperty>
#include <QVector>
#include <QVector3D>

#include "simpleproperty.h"

class RecordedPathProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit RecordedPathProperties(QObject *parent = nullptr);

    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, recordedLine)
    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, dubinsPath)
    SIMPLE_BINDABLE_PROPERTY(QVector3D, lookaheadPoint)
    SIMPLE_BINDABLE_PROPERTY(bool, showLookahead)

signals:
    void recordedPathPropertiesChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(RecordedPathProperties, QVector<QVector3D>, m_recordedLine, &RecordedPathProperties::recordedLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RecordedPathProperties, QVector<QVector3D>, m_dubinsPath, &RecordedPathProperties::dubinsPathChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RecordedPathProperties, QVector3D, m_lookaheadPoint, &RecordedPathProperties::lookaheadPointChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(RecordedPathProperties, bool, m_showLookahead, false, &RecordedPathProperties::showLookaheadChanged)
};

#endif // RECORDEDPATHPROPERTIES_H
