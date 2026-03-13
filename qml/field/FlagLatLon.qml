// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Windows for creating new flag by Lat Lon
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import ".."
import "../components" as Comp

Comp.MoveablePopup {
    function show(){
        flagLatLon.visible = true
    }

    id: flagLatLon
    x: 40
    y: 40
    height: 350  * theme.scaleHeight
    width: 250  * theme.scaleWidth
    onVisibleChanged: {
        if (visible) {
            newLatPoint.text = (Number(Backend.fixFrame.latitude).toLocaleString(Qt.locale(), 'f', 9))
            newLonPoint.text = (Number(Backend.fixFrame.longitude).toLocaleString(Qt.locale(), 'f', 9))
        }
    }
    property double latt: 0
    property double longi: 0
    property int colorflag: 1

        Comp.TitleFrame{
        color: "#f2f2f2"
        border.color: "black"
        border.width: 2
        title: qsTr("Flag by Lat Lon")
        anchors.fill: parent

        Text {
            id: latTextTop
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            //color: "red"
            visible: true
            text: qsTr("Latitude")
            font.pixelSize: 15
        }
        Text {
            id: latTextBottom
            anchors.top: latTextTop.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            //color: "red"
            visible: true
            text: qsTr("( +90 to -90 )")
            // onTextChanged: update_model() // Removed - update_model not defined
            font.pixelSize: 15
        }

        Comp.NumberTextField {
            id: newLatPoint
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top:latTextBottom.bottom
            width:parent.width*0.75
            height: 50  * theme.scaleHeight
            anchors.margins: 5 * theme.scaleHeight
            bottomVal: -180
            topVal: 180
            decimals: 7
            text: "0"
        }

        Text {
            id: lonTextTop
            anchors.top: newLatPoint.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            //color: "red"
            visible: true
            text: qsTr("Longitude")
            font.pixelSize: 15
        }
        Text {
            id: lonTextBottom
            anchors.top: lonTextTop.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            //color: "red"
            visible: true
            text: qsTr("( +180 to -180 )")
            font.pixelSize: 15
        }

        Comp.NumberTextField {
            id: newLonPoint
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top:lonTextBottom.bottom
            width:parent.width*0.75
            height: 50  * theme.scaleHeight
            anchors.margins: 5 * theme.scaleHeight
            bottomVal: -90
            topVal: 90
            decimals: 7
            text: "0"
        }

        RowLayout{
            id: buttonsBottom
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10 * theme.scaleWidth

            Comp.IconButtonColor{
                id: red
                icon.source: prefix + "/images/FlagRed.png";
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {
                    latt = Number.fromLocaleString(locale, newLatPoint.text);
                    longi = Number.fromLocaleString(locale, newLonPoint.text);
                    aog.redFlagAt(latt, longi, 0) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            }
            Comp.IconButtonColor{
                id: grn
                icon.source: prefix + "/images/FlagGrn.png";
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {
                    latt = Number.fromLocaleString(locale, newLatPoint.text);
                    longi = Number.fromLocaleString(locale, newLonPoint.text);
                    aog.redFlagAt(latt, longi, 1) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            }
            Comp.IconButtonColor{
                id: yel
                icon.source: prefix + "/images/FlagYel.png";
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: {
                    latt = Number.fromLocaleString(locale, newLatPoint.text);
                    longi = Number.fromLocaleString(locale, newLonPoint.text);
                    aog.redFlagAt(latt, longi, 2) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            }
            Comp.IconButtonColor{
                id: cancel
                icon.source: prefix + "/images/Cancel64.png"
                implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                implicitHeight: theme.buttonSize
                color: "#ffffff"
                onClicked: flagLatLon.visible = false;
            }
        }
    }
}

