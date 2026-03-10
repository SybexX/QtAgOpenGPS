// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Menu displayed when the "Field Tools" button is clicked
import QtQuick
import QtQuick.Controls.Fusion
import AOG
//import Settings

import ".."
import "../components"

Drawer {
    id: fieldToolsMenu

    visible: false
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    modal: true

    // Qt 6.8 QProperty + BINDABLE: Simple properties to allow setProperty() updates from C++
    property bool featureIsBoundaryOn: true
    property bool featureIsHeadlandOn: true
    property bool featureIsTramOn: true

    contentItem: Rectangle {
        id: fieldToolsMenuRect
        anchors.bottom: parent.bottom
        height: parent.height
        anchors.top: parent.top
        anchors.left: parent.left

        color: "black"

        ScrollViewExpandableColumn {
            anchors.fill: fieldToolsMenuRect
            id: fieldToolsMenuColumn
            height: fieldToolsMenuRect.height
            spacing: 0
            IconButtonTextBeside{
                text: qsTr("Boundary")
                icon.source: prefix + "/images/MakeBoundary.png"
                //width: 300
                // Threading Phase 1: Boundary feature visibility
                visible: featureIsBoundaryOn
                onClicked: {
                    fieldToolsMenu.visible = false
                    boundaryMenu.show()
                }
            }
            IconButtonTextBeside{
                text: qsTr("Headland")
                icon.source: prefix + "/images/HeadlandMenu.png"
                //width: 300
                // Threading Phase 1: Headland feature visibility
                visible: featureIsHeadlandOn
                onClicked: {
                    fieldToolsMenu.visible = false
                    if (BoundaryInterface.count > 0) {
                        headlandDesigner.show()
                    }else{
                        timedMessage.addMessage(2000, qsTr("No Boundaries"), qsTr("Create A Boundary First"))
                    }
                }
            }
            IconButtonTextBeside{
                text: qsTr("Headland (Build)")
                icon.source: prefix + "/images/Headache.png"
                // Threading Phase 1: Headland feature visibility
                visible: featureIsHeadlandOn
                //width: 300
                onClicked: {
                    fieldToolsMenu.visible = false
                    if (BoundaryInterface.count > 0) {
                        headacheDesigner.show()
                    }else{
                        timedMessage.addMessage(2000, qsTr("No Boundaries"), qsTr("Create A Boundary First"))
                    }
                }
            }
            IconButtonTextBeside{
                text: qsTr("Tram Lines")
                icon.source: prefix + "/images/TramLines.png"
                //width: 300
                // Threading Phase 1: Tram lines feature visibility
                visible: featureIsTramOn
                onClicked: tramLinesEditor.visible = true
            }
            IconButtonTextBeside{
                text: qsTr("Recorded Path")
                icon.source: prefix + "/images/RecPath.png"
                //width: 300
                // Threading Phase 1: Headland feature visibility
                visible: featureIsHeadlandOn
                onClicked:{
                    fieldToolsMenu.visible = false
                    recPath.show()
                }
            }
            IconButtonTextBeside {
                id: delAppliedArea
                enabled: MainWindowState.manualBtnState == SectionState.Off && MainWindowState.autoBtnState == SectionState.Off
                icon.source: prefix + "/images/TrashApplied.png"
                text: qsTr("Delete Applied Area")
                onClicked: {Backend.deleteAppliedArea() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                fieldToolsMenu.visible = false}
            }
            IconButtonTextBeside {
                id: btnflagLatLon
                objectName: "btnFlag"
                icon.source:  prefix + "/images/FlagRed.png";
                text: qsTr("Flag by Lat Lon")
                visible: true
                onClicked: {
                    fieldToolsMenu.visible = false
                    flagLatLon.show();
                    //aog.btnFlag();
                }
            }
        }
    }
}
