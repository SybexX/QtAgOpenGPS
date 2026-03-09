// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window to set the coordinates of the simulator
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
//import Settings
import AOG


import ".."
import "../components"

Rectangle{
    id: setColors
    anchors.fill: mainWindow
    color: aogInterface.backgroundColor
    visible: false
    function show(){
        setColors.visible = true
        console.log("showing in setcolors")
    }
    Image { // map image
        id: setColorsImage
        source: prefix + "/images/ColorBackGnd.png"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: 50
        //width: parent.width *.9
        //fillMode: Image.PreserveAspectFit

    }

    ColorDialog{//color picker
        id: cpFieldColor
        onSelectedColorChanged: {
            //colorFieldDay=@Variant(\0\0\0\x43\x1\xff\xff\x91\x91\x91\x91\x91\x91\0\0)
            //colorFieldNight=@Variant(\0\0\0\x43\x1\xff\xff<<<<<<\0\0)
            //just use the Day setting. AOG has them locked to the same color anyways
            //Settings.display_colorFieldDay = cpFieldColor.selectedColor;
            // Threading Phase 1: Field color configuration
            if (btnDayNight.checked){SettingsManager.display_colorFieldNight = cpFieldColor.selectedColor;}
            else {SettingsManager.display_colorFieldDay = cpFieldColor.selectedColor;}

            //change the color on the fly. In AOG, we had to cycle the sections off
            //and back on. This does for us.
        }
    }
    ColorDialog{//color picker
        id: cpFrameColor
        onSelectedColorChanged: {
            //colorFieldDay=@Variant(\0\0\0\x43\x1\xff\xff\x91\x91\x91\x91\x91\x91\0\0)
            //colorFieldNight=@Variant(\0\0\0\x43\x1\xff\xff<<<<<<\0\0)
            //just use the Day setting. AOG has them locked to the same color anyways
            //Settings.display_colorFieldDay = cpFieldColor.selectedColor;
            // Threading Phase 1: Field color configuration
            if (btnDayNight.checked){SettingsManager.display_colorNightFrame = cpFrameColor.selectedColor;}
            else {SettingsManager.display_colorDayFrame = cpFrameColor.selectedColor;}

            //change the color on the fly. In AOG, we had to cycle the sections off
            //and back on. This does for us.
        }
    }
    RowLayout{
        id: buttontop
        anchors.right: parent.right
        anchors.left: setColorsImage.right
        anchors.top: parent.top
        anchors.margins: 10
        height: children.height
        width: parent.width *.3
        IconButtonColor {
            id: btnColorField
            icon.source: prefix + "/images/ColourPick.png"
            onClicked: cpFieldColor.open()
            text: "Color Field"
            // Threading Phase 1: Display field color based on day/night mode
            color: btnDayNight.checked?SettingsManager.display_colorFieldNight:SettingsManager.display_colorFieldDay
        }
        IconButtonColor {
            id: btnColorFrame
            icon.source: prefix + "/images/ColourPick.png"
            onClicked: cpFrameColor.open()
            text: "Color Frame"
            // Threading Phase 1: Display field color based on day/night mode
            color: btnDayNight.checked?SettingsManager.display_colorNightFrame:SettingsManager.display_colorDayFrame
        }
        IconButtonColor {
            id: btnDayNight
            text: "Day/Night"
            icon.source: prefix + "/images/Config/ConD_AutoDayNight.png"
            checkable: true
            color1: "blue"
            colorChecked1: "black"
            colorChecked2: "black"
            colorChecked3: "black"
            onVisibleChanged: btnDayNight.checked = false
        }
    }

    RowLayout{
        id: buttons
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        height: children.height
        width: parent.width *.3
        IconButtonTransparent{
            icon.source: prefix + "/images/Cancel64.png"
            onClicked: setColors.visible = false
        }
        IconButtonTransparent{
            icon.source: prefix + "/images/OK64.png"
            //anchors.bottom: parent.bottom
            //anchors.right: parent.right
            onClicked: {
                aog.settingsSave() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                aog.settingsReload() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                setColors.visible = false
            }
        }
    }
    
}

