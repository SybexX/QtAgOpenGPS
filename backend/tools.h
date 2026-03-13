// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef TOOLS_H
#define TOOLS_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QVariantList>
#include "tool.h"
#include "toolswithsectionsmodel.h"
#include "toolsproperties.h"

Q_MOC_INCLUDE("toolsproperties.h")

class Tools : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(ToolsWithSectionsModel* toolsWithSectionsModel READ toolsWithSectionsModel CONSTANT)
    // Scene graph properties for FieldViewItem
    Q_PROPERTY(ToolsProperties* toolsProperties READ toolsProperties CONSTANT)

private:
    explicit Tools(QObject *parent = nullptr);
    ~Tools() override = default;

    // Prevent copying
    Tools(const Tools &) = delete;
    Tools &operator=(const Tools &) = delete;

    static Tools *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static Tools *instance();
    static Tools *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Getters
    ToolsWithSectionsModel* toolsWithSectionsModel() const { return m_toolsWithSectionsModel; }
    ToolsProperties* toolsProperties() const { return m_toolsProperties; }

    // Tool management
    Q_INVOKABLE void addTool(Tool *tool);
    Q_INVOKABLE void removeTool(int index);
    Q_INVOKABLE void clearTools();
    Q_INVOKABLE Tool* toolAt(int index) const;

    // Section button state methods (delegate to ToolsProperties
    Q_INVOKABLE void setSectionButtonState(int toolIndex, int sectionButtonNo, SectionButtonsModel::State new_state);
    Q_INVOKABLE void setAllSectionButtonsToState(int toolIndex, SectionButtonsModel::State new_state);



public slots:
    //generate a single tool from settings, which is all QtAOG supports
    //for now.  tool index will always be 1 until multiple tools are supported
    void generateToolFromSettings();

signals:
    void sectionButtonStateChanged(int toolIndex, int sectionButtonNo, SectionButtonsModel::State new_state);

private:
    ToolsWithSectionsModel *m_toolsWithSectionsModel;
    ToolsProperties *m_toolsProperties = nullptr;
};

#endif // TOOLS_H
