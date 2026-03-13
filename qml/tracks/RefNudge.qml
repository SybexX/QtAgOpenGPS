// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Where we nudge the ref.
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
//import Settings
import AOG
// Interface import removed - now QML_SINGLETON

import ".."
import "../components" as Comp

Comp.MoveablePopup {
    id: refNudge
    x: 40
    y: 40
    height: 400
    width: 230
    function show(){
        refNudge.visible = true
    }

    Comp.TitleFrame{
        id: rootRect
        color: "#ff6666"
        anchors.fill: parent
        title: qsTr("Ref Adjust")
        Rectangle{
            anchors.top: parent.top
            anchors.bottom: nudgeText.top
            anchors.bottomMargin: 15
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 16
            ColumnLayout{
                anchors.fill: parent
                RowLayout{
                    Layout.alignment: Qt.AlignCenter
                    implicitWidth: parent.width
                    Comp.IconButtonTransparent{
                        icon.source: prefix + "/images/SnapLeftHalf.png"
                        Layout.alignment: Qt.AlignLeft
                        onClicked: TracksInterface.ref_nudge((SettingsManager.vehicle_toolWidth - SettingsManager.vehicle_toolOverlap)/-2)
                    }
                    Comp.IconButtonTransparent{
                        icon.source: prefix + "/images/SnapRightHalf.png"
                        Layout.alignment: Qt.AlignRight
                        onClicked: TracksInterface.ref_nudge((SettingsManager.vehicle_toolWidth - SettingsManager.vehicle_toolOverlap)/2)
                    }
                }
                RowLayout{
                    Layout.alignment: Qt.AlignCenter
                    implicitWidth: parent.width
                    Comp.IconButtonTransparent{
                        icon.source: prefix + "/images/SnapLeft.png"
                        Layout.alignment: Qt.AlignLeft
                        onClicked: TracksInterface.ref_nudge(SettingsManager.as_snapDistanceRef/-100) // Threading Phase 1: Ref snap distance
                    }
                    Comp.IconButtonTransparent{
                        icon.source: prefix + "/images/SnapRight.png"
                        Layout.alignment: Qt.AlignRight
                        onClicked: TracksInterface.ref_nudge(SettingsManager.as_snapDistanceRef/100) // Threading Phase 1: Ref snap distance
                    }
                }
                Comp.SpinBoxCM{
                    id: offset
                    Layout.alignment: Qt.AlignCenter
                    from: 1
                    to: 10000
                    boundValue: SettingsManager.as_snapDistanceRef
                    onValueModified: SettingsManager.as_snapDistanceRef = value // Threading Phase 1: Reference nudge distance
                }
            }
        }
        Comp.TextLine{
            id: nudgeText
            font.pixelSize: 25
            text: qsTr("0 ")+ Utils.cm_unit_abbrev()
            anchors.bottom: bottomButtons.top
            anchors.bottomMargin: 15
        }
        RowLayout{
            id: bottomButtons
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 15
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: refNudge.visible = false
                Layout.alignment: Qt.AlignCenter
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/OK64.png"
                onClicked: refNudge.visible = false
                Layout.alignment: Qt.AlignCenter
            }
        }
    }
}
