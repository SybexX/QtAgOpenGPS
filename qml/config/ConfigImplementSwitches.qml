// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window where we set if work/steer switch start section control or anything
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
//import Settings

import ".."
import "../components"

/*todo
  this page definitely isn't done yet. Need to add logic for
  changing one button state when it's opposite is clicked
  */
Rectangle{
	id: configImplementSwitch
	anchors.fill: parent
    color: aogInterface.backgroundColor
	visible: false
	Rectangle{
		anchors.left: parent.left
		anchors.bottom: parent.bottom
		anchors.top: parent.top
		width: workswitchcolumn.width + (40 * theme.scaleWidth)
		anchors.topMargin: 20 * theme.scaleHeight
		anchors.bottomMargin: 20 * theme.scaleHeight
		anchors.leftMargin: 20 * theme.scaleWidth
		anchors.rightMargin: 20 * theme.scaleWidth
        border.color: aogInterface.blackDayWhiteNight
        color: aogInterface.backgroundColor
		Text{
			id: workTitle
			anchors.top: parent.top
			anchors.left: parent.left
			text: qsTr("Work Switch")
		}
		ColumnLayout{
			id: workswitchcolumn
			anchors.top: workTitle.bottom
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			width: childrenRect.width
			anchors.topMargin: 7 * theme.scaleHeight
			anchors.bottomMargin: 7 * theme.scaleHeight
			anchors.leftMargin: 7 * theme.scaleWidth
			anchors.rightMargin: 7 * theme.scaleWidth
			IconButtonTextBeside{
				icon.source: prefix + "/images/HydraulicLiftOff.png"
                checkable: true
                // Threading Phase 1: Remote work system enable
                isChecked: SettingsManager.f_isRemoteWorkSystemOn
                onCheckedChanged: SettingsManager.f_isRemoteWorkSystemOn = checked
				text: qsTr("Work Switch")
			}

            ButtonGroup {
                buttons: [workSwitchMan, workSwitchAuto ]
            }

            IconButtonTextBeside{
                id: workSwitchMan
				icon.source: prefix + "/images/ManualOff.png"
                checkable: true
                enabled: SettingsManager.f_isRemoteWorkSystemOn
                isChecked: SettingsManager.f_isWorkSwitchManualSections
                onCheckedChanged: SettingsManager.f_isWorkSwitchManualSections = checked
				text: qsTr("Manual Sections")
			}

            IconButtonTextBeside{
                id: workSwitchAuto
				icon.source: prefix + "/images/SectionMasterOff.png"
                checkable: true
                enabled: SettingsManager.f_isRemoteWorkSystemOn
                isChecked: !SettingsManager.f_isWorkSwitchManualSections
                onCheckedChanged: SettingsManager.f_isWorkSwitchManualSections = !checked
				text: qsTr("Auto Sections")
			}

			IconButton{
				implicitWidth: 270 * theme.scaleWidth
				implicitHeight: 80 * theme.scaleHeight
				border: 1
				id: switchStateButton
				icon.source: prefix + "/images/SwitchActiveClosed.png"
				color3: "white"
				colorChecked1: "green"
				colorChecked2: "green"
				colorChecked3: "green"
                checkable: true
                // Threading Phase 1: Work switch active low logic
                isChecked: SettingsManager.f_isWorkSwitchActiveLow
                onCheckedChanged: SettingsManager.f_isWorkSwitchActiveLow = checked
			}
		}
	}

	Rectangle{
		anchors.right:  parent.right
		anchors.bottom: parent.bottom
		anchors.top: parent.top
		width: steerswitchcolumn.width + (40 * theme.scaleWidth)
		anchors.topMargin: 20 * theme.scaleHeight
		anchors.bottomMargin: 20 * theme.scaleHeight
		anchors.rightMargin: 20 * theme.scaleWidth
		anchors.leftMargin: 20 * theme.scaleWidth
        border.color: aogInterface.blackDayWhiteNight
        color: aogInterface.backgroundColor
		Text{
			id: steerTitle
			anchors.top: parent.top
			anchors.left: parent.left
			text: qsTr("Steer Switch")
		}
		ColumnLayout{
			id: steerswitchcolumn
			anchors.top: steerTitle.bottom
			anchors.bottom: parent.bottom
			width: childrenRect.width
			anchors.right: parent.right
			anchors.topMargin: 7 * theme.scaleHeight
			anchors.bottomMargin: 7 * theme.scaleHeight
			anchors.rightMargin: 7 * theme.scaleWidth
			anchors.leftMargin: 7 * theme.scaleWidth
			IconButtonTextBeside{
                // Threading Phase 1: Steer work switch enable
                isChecked: SettingsManager.f_isSteerWorkSwitchEnabled
                onCheckedChanged: SettingsManager.f_isSteerWorkSwitchEnabled = checked
                checkable: true
				text: qsTr("Steer Switch")
				icon.source: prefix + "/images/AutoSteerOff.png"
			}

            ButtonGroup {
                buttons: [steerSwitchMan, steerSwitchAuto ]
            }

            IconButtonTextBeside{
                id: steerSwitchMan
                // Threading Phase 1: Steer switch manual sections
                isChecked: SettingsManager.f_isSteerWorkSwitchManualSections
                onCheckedChanged: SettingsManager.f_isSteerWorkSwitchManualSections = checked
                enabled: SettingsManager.f_isSteerWorkSwitchEnabled
                checkable: true
				text: qsTr("Manual Sections")
				icon.source: prefix + "/images/ManualOff.png"
			}
			IconButtonTextBeside{
                id: steerSwitchAuto
                // Threading Phase 1: Steer switch auto sections
                isChecked: !SettingsManager.f_isSteerWorkSwitchManualSections
                onCheckedChanged: SettingsManager.f_isSteerWorkSwitchManualSections = !checked
                enabled: SettingsManager.f_isSteerWorkSwitchEnabled
                text: qsTr("Auto Sections")
				icon.source: prefix + "/images/SectionMasterOff.png"
			}
		}
	}
}
