// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Displays the GPS Data on main screen.
import QtQuick
import QtQuick.Controls.Fusion
import AOG
import "components" as Comp

Rectangle{
    id: blockageData
    width: 200* theme.scaleWidth
    height: 200 * theme.scaleHeight
    color: "#4d4d4d"
    Column{
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 15 * theme.scaleHeight
        Text{ color: "white"; font.pointSize: 20; text: qsTr("Max: ")+ Utils.area_to_unit_string(Blockage.max*10000, 0)}
        Text{ color: "white"; font.pointSize: 20; text: qsTr("Row N max  ")+Blockage.max_i}
        Text{ color: "white"; font.pointSize: 20; text: qsTr("Avg ")+ Utils.area_to_unit_string(Blockage.avg*10000, 0)}
        Text{ color: "white"; font.pointSize: 20; text: qsTr("Min ")+Utils.area_to_unit_string(Blockage.min1*10000, 0) +(" ")+ Utils.area_to_unit_string(Blockage.min2*10000, 0)}
        Text{ color: "red"; font.pointSize: 20; text: qsTr("Rows ")+Blockage.min1_i+(" ")+Blockage.min2_i}
        Text{ color: "red"; font.pointSize: 20; text: qsTr("Blocked ")+Blockage.blocked}
        }
}
