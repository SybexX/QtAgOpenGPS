// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// The panel of buttons where track editing(new, go to select which track, line and ref nudge, etc)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
import AOG
// Interface import removed - now QML_SINGLETON
import "../components" as Comp

import ".."

Comp.TimedRectangle {
    id: trackButtons
    color: "white"
    z: 1
    width: (TracksInterface.idx > -1)?marker.width*7+40:marker.width*2+15
    height: marker.height+10
    RowLayout{
        anchors.fill: parent
        anchors.margins: 5
        Comp.IconButtonTransparent{
            id: marker
            icon.source: prefix + "/images/ABSnapNudgeMenuRef.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: refNudge.show()
            visible: TracksInterface.idx > -1
        }
        Comp.IconButtonColor{
            icon.source: prefix + "/images/AutoSteerSnapToPivot.png"
            implicitWidth: marker.width
            implicitHeight: marker.height
            Layout.alignment: Qt.AlignCenter
            onClicked: TracksInterface.nudge_center()
            visible: TracksInterface.idx > -1
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/SwitchOff.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                TracksInterface.select(-1);
            }
            visible: TracksInterface.idx > -1
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/ABTracks.png"
            Layout.alignment: Qt.AlignCenter
			onClicked: trackList.show()
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/AddNew.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                trackNewButtons.show()
            }
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/ABDraw.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: lineDrawer.show()
            visible: TracksInterface.idx > -1
        }
        Comp.IconButtonTransparent{
            icon.source: prefix + "/images/ABSnapNudgeMenu.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: lineNudge.show()
            visible: TracksInterface.idx > -1
        }
    }
}
