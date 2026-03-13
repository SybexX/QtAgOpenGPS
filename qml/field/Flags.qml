// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Windows for creating new tracks
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import ".."
import "../components" as Comp

Comp.MoveablePopup {
    function show(){
        flags.visible = true
    }
    id: flags
    x: 40
    y: 40
    height: 320  * theme.scaleHeight
    width: 250  * theme.scaleWidth

    property double dist: 0

    function update_model() {
        //var distance = 0.0
        if (FlagsInterface.currentFlag > 0)
            dist = Utils.distanceLatLon(Backend.fixFrame.latitude, Backend.fixFrame.longitude,
                                        FlagsInterface.currentLatitude,
                                        FlagsInterface.currentLongitude);
        else dist = 0;
    }

    Connections {
        target: Backend

        function onFixFrameChanged() {
            update_model()
        }
    }

    Connections {
        target: FlagsInterface

        function onCurrentLatitudeChanged() {
            update_model()
        }

        function onCurrentLongitudeChanged() {
            update_model()
        }
    }

    Comp.TitleFrame{
        color: "#f2f2f2"
        border.color: "black"
        border.width: 2
        title: qsTr("Flags")
        anchors.fill: parent

        RowLayout{
            id: buttonsBottom
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10 * theme.scaleWidth

            Comp.IconButtonColor{
                id: up
                icon.source: prefix + "/images/UpArrow64.png"
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {
                    FlagsInterface.nextFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                    update_model()
                }
            }
            Comp.IconButtonColor{
                id: down
                icon.source: prefix + "/images/DnArrow64.png"
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {
                    FlagsInterface.prevFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                    update_model()
                }
            }
            Comp.IconButtonColor{
                id: deleteflag
                icon.source: prefix + "/images/FlagDelete.png"
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: FlagsInterface.deleteCurrentFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            Comp.IconButtonColor{
                id: cancel
                icon.source: prefix + "/images/Cancel64.png"
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {flags.visible = false;
                    FlagsInterface.cancelCurrentFlag()} // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
        }
        Rectangle{
            id: textEntry
            width:parent.width*0.75
            height: 50  * theme.scaleHeight
            anchors.top:parent.top
            anchors.topMargin: 20 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            color: aogInterface.backgroundColor
            border.color: "darkgray"
            border.width: 1
            TextField{
                id: newField
                anchors.fill: parent
                selectByMouse: true
                text: FlagsInterface.currentNotes
                onTextEdited: {
                    FlagsInterface.currentNotes = text
                }
                font.pixelSize: 15
            }
            Text {
                id: idText
                anchors.top: textEntry.bottom
                anchors.left: parent.left
                //color: "red"
                visible: true
                text: qsTr("ID: ") + (FlagsInterface.currentFlag > 0 ? FlagsInterface.currentFlag : 0)
                font.pixelSize: 15
            }
            Text {
                id: latText
                anchors.top: idText.bottom
                anchors.left: parent.left
                //color: "red"
                visible: true
                text: qsTr("Lat: ") + (FlagsInterface.currentFlag > 0
                                       ? Number(FlagsInterface.currentLatitude).toLocaleString(Qt.locale(), 'f', 9)
                                       : "")
                font.pixelSize: 15
            }
            Text {
                id: lonText
                anchors.top: latText.bottom
                anchors.left: parent.left
                //color: "red"
                visible: true
                text: qsTr("Lon: ") + (FlagsInterface.currentFlag > 0
                                       ? Number(FlagsInterface.currentLongitude).toLocaleString(Qt.locale(), 'f', 9)
                                       : "")
                font.pixelSize: 15
            }
            Text {
                anchors.top: textEntry.bottom
                anchors.left: parent.left
                //color: "red"
                visible: false
                text: Number(Backend.fixFrame.longitude).toLocaleString(Qt.locale(), 'f', 9)
            }
            Text {
                anchors.top: textEntry.bottom
                anchors.right: parent.right
                //color: "red"
                visible: false
                text: Number(Backend.fixFrame.latitude).toLocaleString(Qt.locale(), 'f', 9)
            }
            Text {
                id: distText
                anchors.top: lonText.bottom
                anchors.left: parent.left
                //color: "red"
                visible: true
                text: qsTr("Dist: ") + Math.round(dist*100)/100 +" " + Utils.m_unit_abbrev()
            }
            Text {
                id: errorMessage
                anchors.top: newField.bottom
                anchors.left: newField.left
                color: "red"
                visible: false
                text: qsTr("This flag exists already; please choose another name.")
            }
        }
    }
}
