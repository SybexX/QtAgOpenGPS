#ifndef TRACKPROPERTIES_H
#define TRACKPROPERTIES_H

#include <QObject>
#include <QQmlEngine>
#include <QBindable>
#include <QProperty>
#include "simpleproperty.h"

class TrackProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    TrackProperties();

    SIMPLE_BINDABLE_PROPERTY(bool, visible)
    SIMPLE_BINDABLE_PROPERTY(bool, dashed)
    SIMPLE_BINDABLE_PROPERTY(bool, loop)
    SIMPLE_BINDABLE_PROPERTY(QColor, color)
    SIMPLE_BINDABLE_PROPERTY(float, lineWidth)
    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, points)
signals:
    void trackPropertiesUniformsChanged();
    void trackPropertiesGeometryChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackProperties, bool, m_visible, false, &TrackProperties::visibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackProperties, bool, m_dashed, false, &TrackProperties::dashedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackProperties, bool, m_loop, false, &TrackProperties::loopChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackProperties, QColor, m_color, &TrackProperties::colorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackProperties, float, m_lineWidth, 0, &TrackProperties::lineWidthChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackProperties, QVector<QVector3D>, m_points, &TrackProperties::pointsChanged)

};

#endif // TRACKPROPERTIES_H
