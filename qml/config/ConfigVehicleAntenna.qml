// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Antenna dimensions
import QtQuick
import QtQuick.Controls.Fusion
//import Settings

import ".."
import "../components"

Rectangle{
    id: configTractorAntenna
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false
    Image {
        id: antImage
        //3 vehicle types  tractor=0 harvestor=1 4wd=2
        // Threading Phase 1: Vehicle type dependent antenna image
        source: SettingsManager.vehicle_vehicleType === 0 ? prefix + "/images/qtSpecific/AntennaTractor.png" :
                SettingsManager.vehicle_vehicleType === 1 ? prefix + "/images/qtSpecific/AntennaHarvester.png" :
                SettingsManager.vehicle_vehicleType === 2 ? prefix + "/images/qtSpecific/Antenna4WD.png" :
                prefix + "/images/Config/ConSt_Mandatory.png"
        width: 350 * theme.scaleWidth
        height: 175 * theme.scaleHeight
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: parent.top
		anchors.topMargin: (30+antennaPivot.height) * theme.scaleHeight
    }
    SpinBoxCM{
        id: antennaPivot
        anchors.bottom: antImage.top
        anchors.left: antImage.left
        anchors.leftMargin: 30
        from: -999
        to: 999
        editable: true
        // Threading Phase 1: Antenna pivot dimension
        boundValue: SettingsManager.vehicle_antennaPivot
        onValueModified: SettingsManager.vehicle_antennaPivot = value
    }
    SpinBoxCM{
        id: antennaHeight
        anchors.top: antImage.top
        anchors.topMargin: 100
        anchors.left: antImage.right
        anchors.leftMargin: -50
        from: 0
        to: 1000
        editable: true
        // Threading Phase 1: Antenna height dimension
        boundValue: SettingsManager.vehicle_antennaHeight
        onValueModified: SettingsManager.vehicle_antennaHeight = value
    }
	TitleFrame{
		anchors.top: antImage.bottom
		anchors.topMargin: 20 * theme.scaleHeight
		title: qsTr("Antenna Offset")
		anchors.horizontalCenter: parent.horizontalCenter
		width: parent.width * 0.65
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 20 * theme.scaleHeight
		border.width: 2
        Item{
            ButtonGroup {
                buttons: [ offsetLeft, offsetRight ]
            }
        }

        IconButtonColor{
            id: offsetLeft
            implicitWidth: 170 * theme.scaleWidth
            implicitHeight: 250 * theme.scaleHeight
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
			anchors.leftMargin: 7 * theme.scaleWidth
			iconHeightScaleText: 1
            // Threading Phase 1: Left antenna offset configuration
            checkable: SettingsManager.gps_headingFromWhichSource === "Single"
            isChecked: (SettingsManager.vehicle_antennaOffset < 0)
			icon.source: SettingsManager.vehicle_vehicleType === 0 ? prefix + "/images/qtSpecific/LeftAntenna.png"
			: SettingsManager.vehicle_vehicleType === 1 ? prefix + "/images/qtSpecific/LeftAntennaHarvester.png"
			: SettingsManager.vehicle_vehicleType === 2 ? prefix + "/images/qtSpecific/LeftAntenna4WD.png"
			: prefix + "/images/Config/ConSt_Mandatory.png"
			onClicked: {
				if (SettingsManager.gps_headingFromWhichSource === "Dual") 
				    timedMessage.addMessage(5000, qsTr("Not Allowed"), qsTr("Dual heading MUST be right only"))
				else
					SettingsManager.vehicle_antennaOffset = -Math.abs(SettingsManager.vehicle_antennaOffset)
			}

        }

        IconButtonColor{
            id: offsetRight
            implicitWidth: 170 * theme.scaleWidth
            implicitHeight: 250 * theme.scaleHeight
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
			anchors.rightMargin: 7 * theme.scaleWidth
            // Threading Phase 1: Right antenna offset configuration
            isChecked: (SettingsManager.vehicle_antennaOffset >= 0)
            checkable: true
            onClicked: SettingsManager.vehicle_antennaOffset = Math.abs(SettingsManager.vehicle_antennaOffset)
            icon.source: SettingsManager.vehicle_vehicleType === 0 ? prefix + "/images/qtSpecific/RightAntenna.png"
					: SettingsManager.vehicle_vehicleType === 1 ? prefix + "/images/qtSpecific/RightAntennaHarvester.png"
					: SettingsManager.vehicle_vehicleType === 2 ? prefix + "/images/qtSpecific/RightAntenna4WD.png"
					: prefix + "/images/Config/ConSt_Mandatory.png"
        }
		SpinBoxCM{
			id: antennaOffset
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.bottom: parent.verticalCenter
			anchors.bottomMargin: 10 * theme.scaleHeight
			from: 0
			to: 500
			editable: true
			// Threading Phase 1: Antenna offset absolute value
			boundValue: Math.abs(SettingsManager.vehicle_antennaOffset)
			onValueChanged:{
				if (offsetLeft.checked){
					SettingsManager.vehicle_antennaOffset = -value
				} else {
				SettingsManager.vehicle_antennaOffset = value
				}
				console.log(SettingsManager.vehicle_antennaOffset)
			}
		}
	}
}
