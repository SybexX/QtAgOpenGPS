// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "tools.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include "settingsmanager.h"
#include "toolsproperties.h"
#include "sectionproperties.h"

Q_LOGGING_CATEGORY(toolsLog, "tools.qtagopengps")

Tools *Tools::s_instance = nullptr;
QMutex Tools::s_mutex;
bool Tools::s_cpp_created = false;

Tools::Tools(QObject *parent)
    : QObject{parent}
    , m_toolsWithSectionsModel(new ToolsWithSectionsModel(this))
    , m_toolsProperties(new ToolsProperties(this))
{
    //put create tools from settings.  CTool and FormGPS_position will fill in
    //specific values for leftPosition, rightPosition, easting, northing, etc.
    //eventually this will be done differently.

    generateToolFromSettings();
    //connect to any setting that would change the configuration of the tool. Probably
    //missed some.
    connect(SettingsManager::instance(), &SettingsManager::tool_isSectionsNotZonesChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::vehicle_numSectionsChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_zonesChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_numSectionsMultiChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_toolTrailingHitchLengthChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_isToolFrontChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_isToolRearFixedChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_isToolTrailingChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_trailingToolToPivotLengthChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::tool_isTBTChanged,
            this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::vehicle_toolOffsetChanged, this, &Tools::generateToolFromSettings);

    connect(SettingsManager::instance(), &SettingsManager::tool_sectionWidthMultiChanged, this, &Tools::generateToolFromSettings);

    connect(SettingsManager::instance(), &SettingsManager::section_position1Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position2Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position3Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position4Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position5Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position6Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position7Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position8Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position9Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position10Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position11Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position12Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position13Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position14Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position15Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position16Changed, this, &Tools::generateToolFromSettings);
    connect(SettingsManager::instance(), &SettingsManager::section_position17Changed, this, &Tools::generateToolFromSettings);
}

Tools *Tools::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Tools();
        qDebug(toolsLog) << "Tools singleton created by C++ code.";
        s_cpp_created = true;
        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

Tools *Tools::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new Tools();
        qDebug(toolsLog) << "Tools singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void Tools::addTool(Tool *tool)
{
    if (!tool) {
        qWarning(toolsLog) << "Cannot add null tool";
        return;
    }

    m_toolsProperties->addTool(tool);

    //pass on the signals from this tool
    int whichTool = m_toolsProperties->toolCount() - 1;

    connect(tool, &Tool::sectionButtonStateChanged,
            [this, whichTool](int sectionButtonNo, SectionButtonsModel::State new_state) {
                emit sectionButtonStateChanged(whichTool, sectionButtonNo, new_state);
    });

    if (tool->sectionButtonsModel()->count()) {
        m_toolsWithSectionsModel->addToolIndex(whichTool);
    }
    //notify QML users
    emit m_toolsProperties->toolsChanged();

    qDebug(toolsLog) << "Tool added. Total tools:" << m_toolsProperties->toolCount();
}

void Tools::removeTool(int index)
{
    if (index < 0 || index >= m_toolsProperties->toolCount()) {
        qWarning(toolsLog) << "Invalid tool index:" << index;
        return;
    }

    m_toolsProperties->removeToolAt(index);
    m_toolsWithSectionsModel->removeToolIndex(index);

    //notify QML users
    emit m_toolsProperties->toolsChanged();

    qDebug(toolsLog) << "Tool removed at index" << index << ". Remaining tools:" << m_toolsProperties->toolCount();
}

void Tools::clearTools()
{
    m_toolsProperties->clearAllTools();
    m_toolsWithSectionsModel->clear();

    emit m_toolsProperties->toolsChanged();

    qDebug(toolsLog) << "All tools cleared";
}

Tool* Tools::toolAt(int index) const
{
    if (index < 0 || index >= m_toolsProperties->toolCount()) {
        qWarning(toolsLog) << "Invalid tool index:" << index;
        return nullptr;
    }

    return m_toolsProperties->toolAt(index);
}

void Tools::generateToolFromSettings() {
    int numButtons;
    int numSections;

    m_toolsProperties->clearAllTools();
    m_toolsWithSectionsModel->clear();

    if (SettingsManager::instance()->tool_isTBT()) {
        //create a tool to represent the cart, but will
        //have no sections associated with it.
        auto *newTool = new Tool(this);

        newTool->set_isTBTTank(true);
        newTool->set_trailing(true);
        newTool->set_pivotToToolLength(0);

        newTool->set_hitchLength(SettingsManager::instance()->vehicle_tankTrailingHitchLength());

        //nothing added to toolsWithSectionsModel

        addTool(newTool);

    }

    auto *newTool = new Tool(this);

    if (SettingsManager::instance()->tool_isSectionsNotZones()) {
        numButtons = SettingsManager::instance()->vehicle_numSections();
        numSections = numButtons;


    } else {
        QVector<int> zoneRanges;
        zoneRanges = SettingsManager::instance()->tool_zones();
        if (zoneRanges.size() > 1) {
            numButtons = zoneRanges[0];
            newTool->set_zones(zoneRanges.mid(1));
        } else {
            qWarning() << "Zones used, not sections, but the number of zones is zero!";
            numButtons = 0;
        }

        //many sections, fewer buttons
        numSections = SettingsManager::instance()->tool_numSectionsMulti();
    }

    if (SettingsManager::instance()->tool_isToolTrailing()) {
        newTool->set_trailing(true);
        newTool->set_hitchLength(SettingsManager::instance()->tool_toolTrailingHitchLength());
    } else {
        newTool->set_trailing(false);
        newTool->set_hitchLength(0);
    }
    newTool->set_pivotToToolLength(SettingsManager::instance()->tool_trailingToolToPivotLength());

    newTool->set_offset(SettingsManager::instance()->vehicle_toolOffset());

    //Set up the QML buttons
    int i;

    for (i = 0; i  < numButtons; i++) {
        newTool->sectionButtonsModel()->addSectionState( {i, SectionState::Off} );
    }

    double sectionPos[17] = {
        SettingsManager::instance()->section_position1(),
        SettingsManager::instance()->section_position2(),
        SettingsManager::instance()->section_position3(),
        SettingsManager::instance()->section_position4(),
        SettingsManager::instance()->section_position5(),
        SettingsManager::instance()->section_position6(),
        SettingsManager::instance()->section_position7(),
        SettingsManager::instance()->section_position8(),
        SettingsManager::instance()->section_position9(),
        SettingsManager::instance()->section_position10(),
        SettingsManager::instance()->section_position11(),
        SettingsManager::instance()->section_position12(),
        SettingsManager::instance()->section_position13(),
        SettingsManager::instance()->section_position14(),
        SettingsManager::instance()->section_position15(),
        SettingsManager::instance()->section_position16(),
        SettingsManager::instance()->section_position17() };

    //Set up Scenegraph section structures
    float rowUnitWidth = SettingsManager::instance()->tool_sectionWidthMulti();
    float rowLeft = -rowUnitWidth * numSections / 2;

    for (i = 0 ; i < numSections; i++) {
        newTool->createSection();
        if (SettingsManager::instance()->tool_isSectionsNotZones()) {
            newTool->sections()[i]->set_leftPosition(sectionPos[i]);
            newTool->sections()[i]->set_rightPosition(sectionPos[i+1]);
        } else {
            newTool->sections()[i]->set_leftPosition(rowLeft + i*rowUnitWidth);
            newTool->sections()[i]->set_rightPosition(rowLeft + (i+1)*rowUnitWidth);
        }
    }

    addTool(newTool);
}

void Tools::setAllSectionButtonsToState(int toolIndex, SectionButtonsModel::State new_state)
{
    toolsProperties()->setAllSectionButtonsToState(toolIndex, new_state);

}

void Tools::setSectionButtonState(int toolIndex, int sectionButtonNo, SectionButtonsModel::State new_state)
{
    toolsProperties()->setSectionButtonState(toolIndex, sectionButtonNo, new_state);
}
