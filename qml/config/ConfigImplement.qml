// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Implement type(drawn, front 3pt, etc)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
//import Settings
//import QtQuick.Controls.Styles 1.4

import ".."
import "../components"

Rectangle{
	anchors.fill: parent
	visible: true
    color: aogInterface.backgroundColor

    // Qt 6.8 QProperty + BINDABLE: Use SettingsManager properties directly
    // Removed local properties - now using SettingsManager.tool_isToolXXX directly
    TextLine{
		id: text
		anchors.top: parent.top
		anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("<h1>Attachment Style</h1>")
	}
	GridLayout{
		anchors.top: text.bottom
		anchors.horizontalCenter: parent.horizontalCenter
        width: 650 * theme.scaleWidth
        height: 450 * theme.scaleHeight
		rows:2
		columns: 2
        flow:Grid.TopToBottom

        ButtonGroup {
            buttons: [ i3pt, i3ptfront, itrailed, iTBT]
        }

		IconButtonColor{
            implicitWidth:300 * theme.scaleWidth
            implicitHeight:200 * theme.scaleHeight
			id: i3pt
            icon.source: prefix + "/images/ToolChkRear.png"
            checkable: true
            // Threading Phase 1: Tool type - Rear Fixed 3 Point
            isChecked: SettingsManager.tool_isToolRearFixed
            onClicked: {
                SettingsManager.tool_isToolRearFixed = true
                SettingsManager.tool_isToolFront = false
                SettingsManager.tool_isToolTrailing = false
                SettingsManager.tool_isTBT = false
            }
		}

		IconButtonColor{
            implicitWidth:300 * theme.scaleWidth
            implicitHeight:200 * theme.scaleHeight
			id: i3ptfront
            icon.source: prefix + "/images/ToolChkFront.png"
            checkable: true
            isChecked: SettingsManager.tool_isToolFront
            onClicked: {
                SettingsManager.tool_isToolRearFixed = false
                SettingsManager.tool_isToolFront = true
                SettingsManager.tool_isToolTrailing = false
                SettingsManager.tool_isTBT = false
            }
        }

		IconButtonColor{
            implicitWidth:300 * theme.scaleWidth
            implicitHeight:200 * theme.scaleHeight
			id: itrailed
            icon.source: prefix + "/images/ToolChkTrailing.png"
            checkable: true
            // Threading Phase 1: Tool type - Trailing
            isChecked: SettingsManager.tool_isToolTrailing
            onClicked: {
                SettingsManager.tool_isToolRearFixed = false
                SettingsManager.tool_isToolFront = false
                SettingsManager.tool_isToolTrailing = true
                SettingsManager.tool_isTBT = false
            }
        }
		IconButtonColor{
            implicitWidth:300 * theme.scaleWidth
            implicitHeight:200 * theme.scaleHeight
			id: iTBT
            icon.source: prefix + "/images/ToolChkTBT.png"
            checkable: true
            // Threading Phase 1: Tool type - TBT (Tow Between Tractor)
            isChecked: SettingsManager.tool_isTBT
            onClicked: {
                SettingsManager.tool_isToolRearFixed = false
                SettingsManager.tool_isToolFront = false
                SettingsManager.tool_isToolTrailing = false
                SettingsManager.tool_isTBT = true
            }
        }
	}
}
