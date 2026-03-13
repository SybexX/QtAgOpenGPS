// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QtQml/qqml.h>
#include <QObjectBindableProperty>
#include <QQmlListProperty>
#include <QColor>
#include <QList>

#include "sectionbuttonsmodel.h"
#include "simpleproperty.h"

class SectionProperties;

Q_MOC_INCLUDE("sectionproperties.h")

class Tool : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SectionButtonsModel* sectionButtonsModel READ sectionButtonsModel CONSTANT)
    Q_PROPERTY(QQmlListProperty<SectionProperties> sections READ getSections NOTIFY sectionsChanged)

public:
    explicit Tool(QObject *parent = nullptr);

    SectionButtonsModel* sectionButtonsModel() const { return m_sectionButtons; }

    // Sections list for scene graph rendering
    QQmlListProperty<SectionProperties> getSections();
    QList<SectionProperties*>& sections() { return m_sections; }
    const QList<SectionProperties*>& sections() const { return m_sections; }

    // C++ wrapper methods for sections list manipulation
    void addSection(SectionProperties *section);
    void removeSection(SectionProperties *section);
    void removeSectionAt(int index);
    void clearAllSections();
    int numSections() const { return m_sections.count(); }
    SectionProperties* getSection(int index) const;
    SectionProperties* createSection();  // Creates new SectionProperties and adds it

    // Position and orientation (updated by core logic)
    SIMPLE_BINDABLE_PROPERTY(double, easting)
    SIMPLE_BINDABLE_PROPERTY(double, northing)
    SIMPLE_BINDABLE_PROPERTY(double, latitude)
    SIMPLE_BINDABLE_PROPERTY(double, longitude)
    SIMPLE_BINDABLE_PROPERTY(double, heading)

    SIMPLE_BINDABLE_PROPERTY(QList<int>, zones)

    // Tool geometry (from settings/configuration)
    SIMPLE_BINDABLE_PROPERTY(bool, trailing)
    SIMPLE_BINDABLE_PROPERTY(bool, isTBTTank)
    SIMPLE_BINDABLE_PROPERTY(float, hitchLength)
    SIMPLE_BINDABLE_PROPERTY(float, pivotToToolLength)
    SIMPLE_BINDABLE_PROPERTY(float, offset)
    SIMPLE_BINDABLE_PROPERTY(QColor, color)

    Q_INVOKABLE void setSectionButtonState(int sectionButtonNo, SectionButtonsModel::State new_state);
    Q_INVOKABLE void setAllSectionButtonsToState(SectionButtonsModel::State new_state);

signals:
    void sectionButtonStateChanged(int sectionButtonNo, SectionButtonsModel::State new_state);
    void toolChanged();
    void sectionsChanged();

private:
    // QQmlListProperty callbacks for sections
    static void appendSection(QQmlListProperty<SectionProperties> *list, SectionProperties *section);
    static qsizetype sectionCount(QQmlListProperty<SectionProperties> *list);
    static SectionProperties *sectionAt(QQmlListProperty<SectionProperties> *list, qsizetype index);
    static void clearSections(QQmlListProperty<SectionProperties> *list);

    void watchSectionProperties(SectionProperties *section);

    SectionButtonsModel *m_sectionButtons;
    QList<SectionProperties*> m_sections;

    // Position properties
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, double, m_easting, 0, &Tool::eastingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, double, m_northing, 0, &Tool::northingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, double, m_latitude, 0, &Tool::latitudeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, double, m_longitude, 0, &Tool::longitudeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, double, m_heading, 0, &Tool::headingChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Tool, QList<int>, m_zones, &Tool::zonesChanged)

    // Geometry properties
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, bool, m_trailing, true, &Tool::trailingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, bool, m_isTBTTank, false, &Tool::isTBTTankChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, float, m_hitchLength, 0.0, &Tool::hitchLengthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, float, m_pivotToToolLength, 0.0, &Tool::pivotToToolLengthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, float, m_offset, 0.0, &Tool::offsetChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Tool, QColor, m_color, QColor(255, 0, 0), &Tool::colorChanged)
};

#endif // TOOL_H
