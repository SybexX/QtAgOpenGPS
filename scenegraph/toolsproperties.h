// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Field surface properties for FieldViewItem - grouped property for QML

#ifndef TOOLSPROPERTIES_H
#define TOOLSPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QColor>
#include <QQmlListProperty>
#include <QList>

#include "simpleproperty.h"
#include "tool.h"
#include "sectionbuttonsmodel.h"

//Need an enum for type:
//arrow, tractor 2wd, tractor 4wd, combine, dot

class ToolsProperties : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<Tool> tools READ getTools NOTIFY toolsChanged)

public:
    explicit ToolsProperties(QObject *parent = nullptr);

    QQmlListProperty<Tool> getTools();

    // Direct access to the list from C++
    QList<Tool*>& toolsList() { return m_tools; }
    const QList<Tool*>& tools() const { return m_tools; }

    // C++ wrapper methods for list manipulation (emit signals properly)
    void addTool(Tool *tool);
    void removeTool(Tool *tool);
    void removeToolAt(int index);
    void clearAllTools();
    int toolCount() const { return m_tools.count(); }
    Tool* toolAt(int index) const;

    // Section button state methods (delegate to Tool instance)
    Q_INVOKABLE void setSectionButtonState(int toolIndex, int sectionButtonNo, SectionButtonsModel::State new_state);
    Q_INVOKABLE void setAllSectionButtonsToState(int toolIndex, SectionButtonsModel::State new_state);

    SIMPLE_BINDABLE_PROPERTY(bool, visible)


signals:
    void toolsChanged();

private:
    QList<Tool*> m_tools;
    // QQmlListProperty callbacks
    static void appendTool(QQmlListProperty<Tool> *list, Tool *tool);
    static qsizetype toolCount(QQmlListProperty<Tool> *list);
    static Tool *toolAt(QQmlListProperty<Tool> *list, qsizetype index);
    static void clearTools(QQmlListProperty<Tool> *list);

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ToolsProperties, bool, m_visible, true, &ToolsProperties::visibleChanged)

};

#endif // TOOLSPROPERTIES_H
