// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Section properties for ToolProperties - individual section definition

#ifndef SECTIONPROPERTIES_H
#define SECTIONPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QColor>
#include <QtQml/qqmlregistration.h>

#include "simpleproperty.h"
#include "sectionstate.h"

class SectionProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit SectionProperties(QObject *parent = nullptr);

    SIMPLE_BINDABLE_PROPERTY(float, leftPosition)
    SIMPLE_BINDABLE_PROPERTY(float, rightPosition)
    SIMPLE_BINDABLE_PROPERTY(QColor, color)
    SIMPLE_BINDABLE_PROPERTY(SectionState::State, state)
    SIMPLE_BINDABLE_PROPERTY(bool, mapping)
    SIMPLE_BINDABLE_PROPERTY(bool, on)


signals:
    void sectionChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, float, m_leftPosition, 0.0, &SectionProperties::leftPositionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, float, m_rightPosition, 0.0, &SectionProperties::rightPositionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, QColor, m_color, QColor::fromRgbF(0,0,0), &SectionProperties::colorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, SectionState::State, m_state, SectionState::Off, &SectionProperties::stateChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, bool, m_mapping, SectionState::Off, &SectionProperties::mappingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SectionProperties, bool, m_on, SectionState::Off, &SectionProperties::onChanged)
};

#endif // SECTIONPROPERTIES_H
