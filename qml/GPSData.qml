// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Displays the GPS Data on main screen.
import QtQuick
import QtQuick.Controls.Fusion
import AOG
import "components" as Comp

Rectangle{
    id: gpsData
    width: 200* theme.scaleWidth
    height: childrenRect.height + 30 * theme.scaleHeight
    color: "#4d4d4d"

    Column{
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 15 * theme.scaleHeight
        Comp.TextLine{ color: "white"; text: qsTr("Serial ")+ aog.satellitesTracked}
        Comp.TextLine{ color: "white"; text: qsTr("Lat ", "abbreviation for latitude")+(Number(Backend.fixFrame.latitude).toLocaleString(Qt.locale(), 'f', 9))}
        Comp.TextLine{ color: "white"; text: qsTr("Lon ", "abbreviation for longitude")+(Number(Backend.fixFrame.longitude).toLocaleString(Qt.locale(), 'f', 9))}
        Comp.TextLine{ color: "white"; text: qsTr("N ", "abbreviation for northing") + (Number(Backend.fixFrame.northing).toLocaleString(Qt.locale(), 'f', 3))}
        Comp.TextLine{ color: "white"; text: qsTr("E ", "abbreviation for easting")+ (Number(Backend.fixFrame.easting).toLocaleString(Qt.locale(), 'f', 3))}
        Comp.TextLine{ color: "white"; text: qsTr("Elev ", "abbreviation for elevation")+ (Number(Backend.fixFrame.altitude).toLocaleString(Qt.locale(), 'f', 2))}
        Comp.TextLine{ color: "white"; text: qsTr("# Sats ", "abbreviation for # Satellites")+ Backend.fixFrame.satellitesTracked}
        Comp.TextLine{ color: "white"; text: qsTr("HDOP ", "acronym for Horizontal Degree Of Position") + (Number(Backend.fixFrame.hdop).toLocaleString(Qt.locale(), 'f', 3))}
        Comp.TextLine{ color: "white"; text: qsTr("Frame ")+ (Number(Backend.fixFrame.frameTime).toLocaleString(Qt.locale(), 'f', 1))}
        Comp.TextLine{ color: "white"; text: qsTr("Raw Hz ", "abbreviation for Raw Hertz")+ (Number(Backend.fixFrame.rawHz).toLocaleString(Qt.locale(), 'f', 1))}
        Comp.TextLine{ color: "white"; text: qsTr("Hz ", "abbreviation for Hertz")+ (Number(Backend.fixFrame.hz).toLocaleString(Qt.locale(), 'f', 1))}
        Comp.TextLine{ color: "white"; text: qsTr("Dropped ")+ Backend.fixFrame.droppedSentences}
        Comp.TextLine{ color: "white"; text: qsTr("Fix2Fix ")+ (Number(Backend.fixFrame.gpsHeading * 180 / Math.PI).toLocaleString(Qt.locale(), 'f', 1))}//convert from radians
        Comp.TextLine{ color: "white"; text: qsTr("IMU ")+ (Backend.fixFrame.imuHeading > 360 ? "#INV" : Number(Backend.fixFrame.imuHeading).toLocaleString(Qt.locale(), 'f', 1))}
        Comp.TextLine{ color: "white"; text: qsTr("Heading ")+ (Number(VehicleInterface.fixHeading * 180 / Math.PI).toLocaleString(Qt.locale(), 'f', 1))}
        Comp.TextLine{ color: "white"; text: "<br>"}
        Comp.TextLine{ 
            visible: Backend.fixFrame.fixQuality === 4 || Backend.fixFrame.fixQuality === 8  //should reference a setting if rtk is turned on
            color: "white"; 
            text: qsTr("RTK Age: ", "abbreviation for Real Time Kinematic Age") + Number(Backend.fixFrame.age).toLocaleString(Qt.locale(), 'f', 1)
        }
        Comp.TextLine{
            visible: Backend.fixFrame.fixQuality === 4 || Backend.fixFrame.fixQuality === 8  //should reference a setting if rtk is turned on
            color: "white"; 
            text: qsTr("Fix: ") + Backend.fixFrame.fixQuality
        }
    }
}
