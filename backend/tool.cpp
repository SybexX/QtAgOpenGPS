// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "tool.h"
#include "sectionproperties.h"

Tool::Tool(QObject *parent)
    : QObject(parent)
    , m_sectionButtons(new SectionButtonsModel(this))
{
    // Report any geometry changes via toolChanged signal
    connect(this, &Tool::trailingChanged, this, &Tool::toolChanged);
    connect(this, &Tool::isTBTTankChanged, this, &Tool::toolChanged);
    connect(this, &Tool::hitchLengthChanged, this, &Tool::toolChanged);
    connect(this, &Tool::pivotToToolLengthChanged, this, &Tool::toolChanged);
    connect(this, &Tool::offsetChanged, this, &Tool::toolChanged);
    connect(this, &Tool::sectionsChanged, this, &Tool::toolChanged);
}

void Tool::setSectionButtonState(int sectionButtonNo, SectionButtonsModel::State new_state)
{
    m_sectionButtons->setState(sectionButtonNo, new_state);
    emit sectionButtonStateChanged(sectionButtonNo, new_state);
}

void Tool::setAllSectionButtonsToState(SectionButtonsModel::State new_state)
{
    m_sectionButtons->setAllState(new_state);
    for (int i = 0; i < m_sectionButtons->count(); i++) {
        emit sectionButtonStateChanged(i, new_state);
    }
}

// ============================================================================
// QQmlListProperty implementation for sections
// ============================================================================

QQmlListProperty<SectionProperties> Tool::getSections()
{
    return QQmlListProperty<SectionProperties>(this, nullptr,
                                               &Tool::appendSection,
                                               &Tool::sectionCount,
                                               &Tool::sectionAt,
                                               &Tool::clearSections);
}

void Tool::appendSection(QQmlListProperty<SectionProperties> *list, SectionProperties *section)
{
    auto *self = static_cast<Tool*>(list->object);
    section->setParent(self);  // Take ownership
    self->watchSectionProperties(section);

    self->m_sections.append(section);
    emit self->sectionsChanged();
    emit self->toolChanged();
}

qsizetype Tool::sectionCount(QQmlListProperty<SectionProperties> *list)
{
    auto *self = static_cast<Tool*>(list->object);
    return self->m_sections.count();
}

SectionProperties *Tool::sectionAt(QQmlListProperty<SectionProperties> *list, qsizetype index)
{
    auto *self = static_cast<Tool*>(list->object);
    return self->m_sections.at(index);
}

void Tool::clearSections(QQmlListProperty<SectionProperties> *list)
{
    auto *self = static_cast<Tool*>(list->object);
    self->m_sections.clear();
    emit self->sectionsChanged();
    emit self->toolChanged();
}

// ============================================================================
// C++ wrapper methods for sections list manipulation
// ============================================================================

void Tool::addSection(SectionProperties *section)
{
    if (!section)
        return;

    section->setParent(this);
    watchSectionProperties(section);

    m_sections.append(section);
    emit sectionsChanged();
    emit toolChanged();
}

void Tool::removeSection(SectionProperties *section)
{
    if (!section)
        return;

    int index = m_sections.indexOf(section);
    if (index >= 0) {
        m_sections.removeAt(index);
        section->disconnect(this);
        emit sectionsChanged();
        emit toolChanged();
    }
}

void Tool::removeSectionAt(int index)
{
    if (index < 0 || index >= m_sections.count())
        return;

    SectionProperties *section = m_sections.at(index);
    m_sections.removeAt(index);
    if (section) {
        section->disconnect(this);
    }
    emit sectionsChanged();
    emit toolChanged();
}

void Tool::clearAllSections()
{
    for (SectionProperties *section : m_sections) {
        if (section) {
            section->disconnect(this);
        }
    }
    m_sections.clear();
    emit sectionsChanged();
    emit toolChanged();
}

SectionProperties* Tool::getSection(int index) const
{
    if (index < 0 || index >= m_sections.count())
        return nullptr;
    return m_sections.at(index);
}

SectionProperties* Tool::createSection()
{
    auto *section = new SectionProperties(this);
    watchSectionProperties(section);

    m_sections.append(section);
    emit sectionsChanged();
    emit toolChanged();
    return section;
}

void Tool::watchSectionProperties(SectionProperties *section) {
    connect(section, &SectionProperties::leftPositionChanged, this, &Tool::toolChanged);
    connect(section, &SectionProperties::rightPositionChanged, this, &Tool::toolChanged);
    connect(section, &SectionProperties::stateChanged, this, &Tool::toolChanged);
    connect(section, &SectionProperties::mappingChanged, this, &Tool::toolChanged);
    connect(section, &SectionProperties::onChanged, this, &Tool::toolChanged);
}
