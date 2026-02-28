// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// The panel with display controls (+, -, 2d/3d etc.
import QtQuick
import QtQuick.Controls.Fusion
//import Settings
import AOG
import "components" as Comp

Comp.TimedRectangle{
    id: displayButtons
    color: aogInterface.backgroundColor

    Connections {
            target: mainWindow
            function onHotKeyPressed(index) {
                switch (index) {
                case 11:
                    btnZoomOut.clicked()
                    break
                case 12:
                    btnZoomIn.clicked()
                    break
                }
            }
        }

    Grid {
        id: tiltButtons
        anchors.leftMargin: 5
        anchors.topMargin: 5
        spacing: 6
        flow: Grid.TopToBottom
        //rows:6
		rows: 5
        columns:2
        onChildrenChanged: console.log("childrenChanged")
        Comp.IconButtonTransparent {
            id: btnTiltDown
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/TiltDown.png"
            onClicked: {
                Backend.tiltDown() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnCamera2d
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/Camera2D64.png"
            onClicked: {
                Backend.view2D() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnCameraNorth2d
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/CameraNorth2D.png"
            onClicked: {
                Backend.normal2D() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnZoomOut
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/ZoomOut48.png"
            onClicked: {
                Backend.zoomOut() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnWindowDayNight
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/WindowDayMode.png"
            iconChecked: prefix + "/images/WindowNightMode.png"
            checkable: true
            // Threading Phase 1: Display day/night mode
            isChecked: SettingsManager.display_isDayMode
            onCheckedChanged: SettingsManager.display_isDayMode = checked
            onClicked: {displayButtons.resetTimer()
                aog.settingsReload()} // Qt 6.8 MODERN: Direct Q_INVOKABLE call
        }
        Comp.IconButtonTransparent {
            id: btnBrightnessDown
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/BrightnessDn.png"
            onClicked: displayButtons.resetTimer()
            // Threading Phase 1: Brightness control visibility
            visible: SettingsManager.display_isBrightnessOn
        }
        Comp.IconButtonTransparent {
            id: btnTiltUp
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/TiltUp.png"
            onClicked: {
                Backend.tiltUp() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnCamera3d
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/Camera3D64.png"
            onClicked: {
                Backend.view3D() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnCameraNorth3d
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/CameraNorth64.png"
            onClicked: {
                Backend.normal3D() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Comp.IconButtonTransparent {
            id: btnZoomIn
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/ZoomIn48.png"
            onClicked: {
                Backend.zoomIn() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                displayButtons.resetTimer() 
            }
        }
        Rectangle {
            id: btnHZ
            width: 70
            height: 70
            radius: 10
        }
        Comp.IconButtonTransparent {
            id: btnBrightnessUp
            //visible: false //todo
            width: 70
            height: 70
            radius: 10
            icon.source: prefix + "/images/BrightnessUp.png"
            onClicked: displayButtons.resetTimer()
            // Threading Phase 1: Brightness control visibility
            visible: SettingsManager.display_isBrightnessOn
        }
    }
}
