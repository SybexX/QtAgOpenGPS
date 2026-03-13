// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main menu when we click the "Field" button on main screen. "New, Drive In, Open, Close, etc"
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
//import Settings
// Interface import removed - now QML_SINGLETON
import AOG

import ".."
import "../components"

Drawer {
    id: fieldMenu
    //modal: false
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true

    onVisibleChanged: {
        aog.fieldUpdateList() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
    }

    contentItem: Rectangle{
        id: mainFieldMenu
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        height: fieldMenu.height

        //border.color: "lightblue"
        //border.width: 2
        color: "black"

        ScrollViewExpandableColumn{
            id: fieldMenuColumn
            anchors.fill: parent
            IconButtonTextBeside{
                objectName: "btnFieldDriveIn"
                Layout.fillWidth: true
                isChecked: false
                text: qsTr("Drive In")
                icon.source: prefix + "/images/AutoManualIsAuto.png"
                onClicked: { fieldMenu.visible = false ; fieldOpen.sortBy = 2 ; fieldOpen.visible = true; TracksInterface.select(-1);}
            }
            IconButtonTextBeside{
                objectName: "btnFieldISOXML"
                isChecked: false
                text: qsTr("ISO-XML")
                icon.source: prefix + "/images/ISOXML.png"
            }
            IconButtonTextBeside{
                objectName: "btnFieldFromKML"
                isChecked: false
                text: qsTr("From KML")
                icon.source: prefix + "/images/BoundaryLoadFromGE.png"
                onClicked: fieldFromKML.visible = true
            }
            IconButtonTextBeside{
                objectName: "btnFieldFromExisting"
                isChecked: false
                text: qsTr("From Existing")
                icon.source: prefix + "/images/FileExisting.png"
                onClicked: {
                    fieldMenu.visible = false
                    fieldFromExisting.show()
                }
            }
            IconButtonTextBeside{
                objectName: "New"
                isChecked: false
                text: qsTr("New")
                icon.source: prefix + "/images/FileNew.png"
                onClicked: {
                    fieldMenu.visible = false
                    fieldNew.visible = true
                }
            }
            IconButtonTextBeside{
                objectName: "btnFieldResume"
                isChecked: false
                text: qsTr("Resume")
                icon.source: prefix + "/images/FilePrevious.png"
                // Threading Phase 1: Resume field enable check
                enabled: SettingsManager.f_currentDir !== "Default" && !Backend.isJobStarted
                onEnabledChanged: fieldToResumeText.visible = enabled

                onClicked: {
                    fieldMenu.visible = false
                    aog.fieldOpen(SettingsManager.f_currentDir) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                }
                Text{ //show which field will be enabled
                    id: fieldToResumeText
                    visible: false
                    onVisibleChanged: console.log("rtxt: " + fieldToResumeText.visible)
                    anchors.left: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    font.pixelSize: 20
                    // Threading Phase 1: Current field directory display
                    text: SettingsManager.f_currentDir
                }
            }
            IconButtonTextBeside{
                objectName: "btnFieldClose"
                isChecked: false
                text: qsTr("Close")
                icon.source: prefix + "/images/FileClose.png"
                enabled: Backend.isJobStarted
                onClicked: {
                    aog.fieldClose() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                    fieldMenu.visible = false
                }

            }
            IconButtonTextBeside{
                objectName: "btnFieldOpen"
                isChecked: false
                text: qsTr("Open")
                icon.source: prefix + "/images/FileOpen.png"
                onClicked: {
                    fieldMenu.visible = false
                    fieldOpen.visible = true;
                }
            }
        }
    }


    /*
        IconButtonTransparent{
            anchors.top: buttons1.bottom
            anchors.left: parent.left
            anchors.margins: 10
            anchors.leftMargin: 100
            id: fieldCancel
            icon.source: prefix + "/images/Cancel64.png"
            //buttonText: "Cancel"
            onClicked: fieldMenu.visible = false
            height: 75
        }
        */

}
