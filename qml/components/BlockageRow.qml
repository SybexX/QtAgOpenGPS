// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
//
import QtQuick


Rectangle {
    id: block
    width: 15 * theme.scaleWidth
    //height: 100 * theme.scaleHeight

    border.width: 1
    border.color: "black"
    property string buttonText: ""
    property color textColor: "white"

    color: "red"

    Text {
        id: label
        color: textColor
        text: block.buttonText
        anchors.bottom: parent.bottom
        font.pixelSize: 13 * theme.scaleWidth
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
