// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window where we set dimensions for the TBT implement, or a drawn implement behind a drawn implement
import QtQuick
import QtQuick.Controls.Fusion
//import Settings
import AOG


import ".."
import "../components"

Rectangle{
    id: configImpDimWin
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false
    Image{
        id: image1
        source: prefix + "/images/ToolHitchPageTBT.png"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 7 * theme.scaleHeight
        anchors.bottomMargin: 7 * theme.scaleHeight
        anchors.leftMargin: 7 * theme.scaleWidth
        anchors.rightMargin: 7 * theme.scaleWidth
        height: parent.height*.75
        SpinBoxCM{
            id: toolTrailingHitchLength
            anchors.top: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.rightMargin: 80 * theme.scaleWidth
            from: 10
            to:3000
            // Threading Phase 1: Tool trailing hitch length
            boundValue: -SettingsManager.tool_toolTrailingHitchLength
            onValueModified: SettingsManager.tool_toolTrailingHitchLength = -value
        }
        SpinBoxCM{
            id: toolTBTHitchLength
            anchors.top: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: 130 * theme.scaleWidth
            from: 10
            to:3000
            // Threading Phase 1: Vehicle tank trailing hitch length
            boundValue: -SettingsManager.vehicle_tankTrailingHitchLength
            onValueModified: SettingsManager.vehicle_tankTrailingHitchLength = -value
        }
    }
    TextLine{
        text: qsTr("Units: ")+ Utils.cm_unit_abbrev()
        font.bold: true
        anchors.top: image1.bottom
        anchors.topMargin: toolTrailingHitchLength.height + (15 * theme.scaleHeight)
    }
}
