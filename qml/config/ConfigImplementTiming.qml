// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Where we set the timing for section on/off
import QtQuick
import QtQuick.Controls.Fusion
//import Settings

import ".."
import "../components"

Rectangle{
    id: configImpTimWin
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false
    Text{
        font.pixelSize: 25
        font.bold: true
        text: qsTr("Look Ahead Time Settings")
        anchors.bottom: offPic.top
        anchors.bottomMargin: 70 * theme.scaleHeight
        anchors.horizontalCenter: offPic.horizontalCenter
    }
    AnimatedImage{
        id: onPic
        width: parent.width /3 - (10 * theme.scaleWidth)
        anchors.topMargin: 3 * theme.scaleHeight
        anchors.bottomMargin: 3 * theme.scaleHeight
        anchors.leftMargin: 3 * theme.scaleWidth
        anchors.rightMargin: 3 * theme.scaleWidth
        height: parent.height/2 + (50 * theme.scaleHeight)
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        cache: true
        source: prefix + "/images/Config/SectionOnLookAhead.gif"
    }
    AnimatedImage{
        id: offPic
        width: parent.width /3 - (10 * theme.scaleWidth)
        anchors.topMargin: 3 * theme.scaleHeight
        anchors.bottomMargin: 3 * theme.scaleHeight
        anchors.leftMargin: 3 * theme.scaleWidth
        anchors.rightMargin: 3 * theme.scaleWidth
        height: parent.height/2 + (50 * theme.scaleHeight)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        cache: true
        source: prefix + "/images/Config/SectionLookAheadOff.gif"
    }
    AnimatedImage{
        id: offDelayPic
        width: parent.width /3 - (10 * theme.scaleWidth)
        anchors.topMargin: 3 * theme.scaleHeight
        anchors.bottomMargin: 3 * theme.scaleHeight
        anchors.leftMargin: 3 * theme.scaleWidth
        anchors.rightMargin: 3 * theme.scaleWidth
        height: parent.height/2 + (50 * theme.scaleHeight)
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        cache: true
        source: prefix + "/images/Config/SectionLookAheadDelay.gif"
    }

    SpinBoxCustomized{
        decimals: 1
        anchors.horizontalCenter: onPic.horizontalCenter
        anchors.top: onPic.bottom
        anchors.topMargin: 10
        from: 0.2
        // Threading Phase 1: Tool look ahead on timing
        value: SettingsManager.vehicle_toolLookAheadOn
        onValueChanged: SettingsManager.vehicle_toolLookAheadOn = value
        to: 22
        text: qsTr("On (secs)")
    }
    SpinBoxCustomized{
        decimals: 1
        anchors.horizontalCenter: offPic.horizontalCenter
        anchors.top: offPic.bottom
        anchors.topMargin: 10
        from: 0
        // Threading Phase 1: Tool look ahead off timing
        value: SettingsManager.vehicle_toolLookAheadOff
        onValueChanged: SettingsManager.vehicle_toolLookAheadOff = value
        to: 20
        editable: true
        text: qsTr("Off (secs)")
    }
    SpinBoxCustomized{
        decimals: 1
        anchors.horizontalCenter: offDelayPic.horizontalCenter
        anchors.top: offDelayPic.bottom
        anchors.topMargin: 10
        from: 0
        // Threading Phase 1: Tool off delay timing
        value: SettingsManager.vehicle_toolOffDelay
        onValueChanged: SettingsManager.vehicle_toolOffDelay = value
        to: 10
        editable: true
        text: qsTr("Delay (secs)")
    }
}
