// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Config for sections. Actually displays a separate child window depending on if
// we are using sections or zones.
import QtQuick
import QtQuick.Controls.Fusion
//import Settings
import AOG


import ".."
import "../components"

//todo: noticed % coverage was in wrong spot
Rectangle{
    id: configImplementSection
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false

    // Qt 6.8 QProperty + BINDABLE: Simple properties to allow setProperty() updates from C++
    property bool toolIsSectionsNotZones: SettingsManager.tool_isSectionsNotZones
    property int vehicleMinCoverage: SettingsManager.vehicle_minCoverage
    property bool toolIsSectionOffWhenOut: SettingsManager.tool_isSectionOffWhenOut
    property double vehicleSlowSpeedCutoff: SettingsManager.vehicle_slowSpeedCutoff

    Row{
        id: bottomRow
        anchors.left: parent.left
        anchors.bottom: parent.bottom
		anchors.topMargin: 5 * theme.scaleHeight
		anchors.rightMargin: 5 * theme.scaleWidth
		anchors.leftMargin: 5 * theme.scaleWidth
		anchors.bottomMargin: 5 * theme.scaleHeight
        spacing: 90 * theme.scaleWidth
        Button{
            function toggleZones(){
                // Threading Phase 1: Toggle between sections and zones mode
                SettingsManager.tool_isSectionsNotZones = ! SettingsManager.tool_isSectionsNotZones
            }
            width: 180 * theme.scaleWidth
            height: 130 * theme.scaleHeight
            id: chooseZones
            objectName: "zonesOrSections"
            onClicked: {
                toggleZones()
            }
            background: Rectangle{
                color: aogInterface.backgroundColor
                border.color: aogInterface.blackDayWhiteNight
                border.width: 1
                Image{
                    id: image

                    // Threading Phase 1: Display section/zone mode image
                    source: toolIsSectionsNotZones ? prefix + "/images/Config/ConT_Asymmetric.png" : prefix + "/images/Config/ConT_Symmetric.png"
                    anchors.fill: parent
                }
            }
        }
        SpinBoxCustomized{
            id: percentCoverage
            from: 0
            to: 100
            // Threading Phase 1: Minimum coverage percentage
            boundValue: vehicleMinCoverage
            anchors.bottom: parent.bottom
            text: qsTr("% Coverage")
            onValueModified: vehicleMinCoverage = value
        }
        IconButton{
            icon.source: prefix + "/images/SectionOffBoundary.png"
            iconChecked: prefix + "/images/SectionOnBoundary.png"
            checkable: true
            anchors.bottom: parent.bottom
            implicitWidth: 100 * theme.scaleWidth
            implicitHeight: 100 * theme.scaleHeight
            border: 1
            radius: 0
            color3: "white"
            colorChecked1: "green"
            colorChecked2: "green"
            colorChecked3: "green"
            // Threading Phase 1: Section off when outside boundary
            isChecked: toolIsSectionOffWhenOut
            onCheckedChanged: SettingsManager.tool_isSectionOffWhenOut = checked
        }
        SpinBoxCustomized{
            //todo: this should be made english/metric
            decimals: 1
            id: slowSpeedCutoff
            from: Utils.speed_to_unit(0)
            to: Utils.speed_to_unit(30)
            // Threading Phase 1: Slow speed cutoff for sections
            boundValue: Utils.speed_to_unit(vehicleSlowSpeedCutoff)
            anchors.bottom: parent.bottom
            onValueModified: SettingsManager.vehicle_slowSpeedCutoff = Utils.speed_from_unit(value)
            text: Utils.speed_unit()

            Image{
                anchors.bottom: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                source: prefix + "/images/SectionOffBelow.png"
            }
        }
    }
    ConfigImplementSectionsSection{
        id: configImplementSectionsSection
        anchors.top: parent.top
        //anchors.topMargin: 80 * theme.scaleHeight
        anchors.right: parent.right
		anchors.rightMargin: 7 * theme.scaleWidth
        anchors.left: parent.left
		anchors.leftMargin: 7 * theme.scaleWidth
        anchors.bottom: bottomRow.top
        //anchors.bottomMargin: 30 * theme.scaleHeight
        // Threading Phase 1: Show sections configuration
        visible: toolIsSectionsNotZones
    }
    ConfigImplementSectionsZones{
        id: configImplementSectionsZones
        anchors.top: parent.top
        anchors.topMargin: 80 * theme.scaleHeight
        anchors.right: parent.right
		anchors.rightMargin: 7 * theme.scaleWidth
        anchors.left: parent.left
		anchors.leftMargin: 7 * theme.scaleWidth
        anchors.bottom: bottomRow.top
        anchors.bottomMargin: 30 * theme.scaleHeight
        // Threading Phase 1: Show zones configuration
        visible: !toolIsSectionsNotZones

    }
}
