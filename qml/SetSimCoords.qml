// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window to set the coordinates of the simulator
import QtQuick
import QtQuick.Layouts
import "components" as Comp

Rectangle{
    id: setSimCoordsRoot
    anchors.fill: mainWindow
    color: aogInterface.backgroundColor
    visible: false
    function show(){
        setSimCoordsRoot.visible = true
        console.log("showing in setsim")
    }
    Comp.TextLine{
        id: textSimCoords
        anchors.top: parent.top
        anchors.margins: 10
        font.pixelSize: 30
        text: qsTr("Set Sim Coords")
    }

    Image { // map image
        id: setSimCoordsImage
        source: prefix + "/images/LatLon.png"
        anchors.top: textSimCoords.bottom
        anchors.left: parent.left
        width: parent.width *.9
        fillMode: Image.PreserveAspectFit
        Rectangle{ //lat text
            color: setSimCoordsRoot.color
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height *.6
            width: children.width
            Comp.TextLine{
                anchors.centerIn: parent
                text: qsTr("Latitude")
                font.pixelSize: 30
                font.bold: true
                rotation: -90
            }
        }
        Rectangle{// longitude text
            color: setSimCoordsRoot.color
            anchors.top: parent.bottom
            anchors.topMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width *.45
            height: children.height
            Comp.TextLine{
                text: qsTr("Longitude")
                anchors.centerIn: parent
                font.pixelSize: 30
                font.bold: true
            }
        }
    }
    Item{
        id: textInputField
        anchors.left: parent.left
        anchors.right: buttons.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        Comp.TextLine{
            text: qsTr("Latitude")
            anchors.bottom: latFromTo.top
            anchors.horizontalCenter: latInput.horizontalCenter
        }
        Comp.TextLine{
            id: latFromTo
            text: qsTr("From -90 to +90")
            anchors.bottom: latInput.top
            anchors.horizontalCenter: latInput.horizontalCenter
        }

        Comp.LatLonTextField{
            id: latInput
            width: textInputField.width * 0.45
            height: 50* theme.scaleHeight
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            placeholderText: SettingsManager.gps_simLatitude
            inputMethodHints: Qt.ImhDigitsOnly
        }
        Comp.TextLine{
            text: qsTr("Longitude")
            anchors.bottom: lonFromTo.top
            anchors.horizontalCenter: lonInput.horizontalCenter
        }
        Comp.TextLine{
            id: lonFromTo
            text: qsTr("From -180 to +180")
            anchors.bottom: lonInput.top
            anchors.horizontalCenter: lonInput.horizontalCenter
        }
        Comp.LatLonTextField{
            id: lonInput
            width: textInputField.width * 0.45
            height: 50* theme.scaleHeight
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            placeholderText: SettingsManager.gps_simLongitude
            inputMethodHints: Qt.ImhDigitsOnly
        }
    }
    //width: 200* theme.scaleWidth
    //height: childrenRect.height + 30 * theme.scaleHeight
    RowLayout{
        id: buttons
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        height: children.height
        width: parent.width *.3
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/Cancel64.png"
            onClicked: setSimCoordsRoot.visible = false
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/OK64.png"
            onClicked: {
                SettingsManager.gps_simLatitude = latInput.text
                SettingsManager.gps_simLongitude = lonInput.text
                setSimCoordsRoot.visible = false
                aog.settings_reload()
            }
        }
    }
}
