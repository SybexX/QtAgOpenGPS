#include "toolsproperties.h"

ToolsProperties::ToolsProperties(QObject *parent)
    : QObject{parent}
{}

QQmlListProperty<Tool> ToolsProperties::getTools()
{
    return QQmlListProperty<Tool>(this, nullptr,
                                            &ToolsProperties::appendTool,
                                            &ToolsProperties::toolCount,
                                            &ToolsProperties::toolAt,
                                            &ToolsProperties::clearTools);
}

void ToolsProperties::appendTool(QQmlListProperty<Tool> *list, Tool *tool)
{
    auto *self = static_cast<ToolsProperties*>(list->object);
    tool->setParent(self);  // Take ownership
    self->connect(tool, &Tool::toolChanged, self, &ToolsProperties::toolsChanged);
    self->m_tools.append(tool);
    emit self->toolsChanged();
}

qsizetype ToolsProperties::toolCount(QQmlListProperty<Tool> *list)
{
    auto *self = static_cast<ToolsProperties*>(list->object);
    return self->m_tools.count();
}

Tool *ToolsProperties::toolAt(QQmlListProperty<Tool> *list, qsizetype index)
{
    auto *self = static_cast<ToolsProperties*>(list->object);
    return self->m_tools.at(index);
}

void ToolsProperties::clearTools(QQmlListProperty<Tool> *list)
{
    auto *self = static_cast<ToolsProperties*>(list->object);
    self->m_tools.clear();
    emit self->toolsChanged();
}

// ============================================================================
// C++ wrapper methods for list manipulation
// ============================================================================

void ToolsProperties::addTool(Tool *tool)
{
    if (!tool)
        return;

    tool->setParent(this);
    connect(tool, &Tool::toolChanged, this, &ToolsProperties::toolsChanged);
    m_tools.append(tool);
    emit toolsChanged();
}

void ToolsProperties::removeTool(Tool *tool)
{
    if (!tool)
        return;

    int index = m_tools.indexOf(tool);
    if (index >= 0) {
        m_tools.removeAt(index);
        tool->disconnect(this);
        emit toolsChanged();
    }
}

void ToolsProperties::removeToolAt(int index)
{
    if (index < 0 || index >= m_tools.count())
        return;

    Tool *tool = m_tools.at(index);
    m_tools.removeAt(index);
    if (tool) {
        tool->disconnect(this);
    }
    emit toolsChanged();
}

void ToolsProperties::clearAllTools()
{
    for (Tool *tool : m_tools) {
        if (tool) {
            tool->disconnect(this);
        }
    }
    m_tools.clear();
    emit toolsChanged();
}

Tool* ToolsProperties::toolAt(int index) const
{
    if (index < 0 || index >= m_tools.count())
        return nullptr;
    return m_tools.at(index);
}

void ToolsProperties::setSectionButtonState(int toolIndex, int sectionButtonNo, SectionButtonsModel::State new_state)
{
    if (toolIndex < 0 || toolIndex >= m_tools.count())
        return;

    m_tools[toolIndex]->setSectionButtonState(sectionButtonNo, new_state);
}

void ToolsProperties::setAllSectionButtonsToState(int toolIndex, SectionButtonsModel::State new_state)
{
    if (toolIndex < 0 || toolIndex >= m_tools.count())
        return;

    m_tools[toolIndex]->setAllSectionButtonsToState(new_state);
}
