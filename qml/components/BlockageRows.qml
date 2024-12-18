// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts 1.1
//This is a the row of on-screen section-control buttonw

Rectangle {
    id: blockageRows
    anchors.horizontalCenter: parent.horizontalCenter
    //width: 15 * theme.scaleWidth * numSections
    width: (800 * theme.scaleWidth / numSections) < 50 ? (15 * theme.scaleWidth * numSections) : (20 * theme.scaleWidth * numSections)
    //height: childrenRect.height * theme.scaleHeight

    color: "transparent"


    property int numSections: 48  // need connect with settings Dim
    property int countMin: 10     // need connect with settings Dim
    property int countMax: 50     // need connect with settings Dim
    property var rowCount: [ 0,90,90,15,19,15,90,90,15,19,15,20,0,15,90,90,15,19,15,20,0,60,90,90,15,19,15,20,0,70,90,90,15,19,15,20,0,78,90,90,15,19,15,20,0,0,78,90 ] // need connect with settings
    property color offColor: "Crimson"
    property color offTextColor: "White"
    property color onColor: "DarkGoldenrod"
    property color onTextColor: "White"
    property color autoColor: "ForestGreen"
    property color autoTextColor: "White"


    //methods
    function setColors() {
        //same colors for sections and zones
        if (settings.setDisplay_isDayMode) {
            blockageRows.offColor = "Red"
            blockageRows.offTextColor = "Black"
            blockageRows.onColor = "Yellow"
            blockageRows.onTextColor = "Black"
            blockageRows.onColor = "Yellow"
            blockageRows.onTextColor = "Black"
        } else {
            blockageRows.offColor = "Crimson"
            blockageRows.offTextColor = "White"
            blockageRows.onColor = "DarkGoldenRod"
            blockageRows.onTextColor = "White"
            blockageRows.onColor = "DarkGoldenRod"
            blockageRows.onTextColor = "White"
        }
    }
/*
    function setRowCount (sectionNo: int, new_state: int) {
        //states: 0 = off, 1 = auto, 2 = on
        var temp1 = aog.sectionRowCount
        var temp2 = blockageRows.rowCount
        var j


            //1:1 correlation between buttons and sections
            temp1[sectionNo] = new_state //this is the tie-in back to the C++ land
            temp2[sectionNo] = new_state //this is the onscreen button

        aog.rowCount = temp1
        sectionButtons.rowCount = temp2
    }
*/

    onNumSectionsChanged: {
        rowModel.clear()
        for (var i = 0; i < numSections; i++) {
            rowModel.append( { rowNo: i } )
        y}
    }

    //callbacks, connections, and signals
    Component.onCompleted:  {
        setColors()
        rowModel.clear()
        for (var i = 0; i < numSections; i++) {
            rowModel.append( { rowNo: i } )
        }
    }

    Connections {
        target: settings
        function onSetDisplay_isDayModeChanged() {
            setColors()
        }
    }

    ListModel {
        id: rowModel
    }

    Component {
        id: rowViewDelegate
        BlockageRow {
            width: (800 * theme.scaleWidth / numSections) < 50 ? (15 * theme.scaleWidth) : (20 * theme.scaleWidth)
            //width: 15 * theme.scaleWidth
            height: (blockageRows.rowCount[model.rowNo]+20 * theme.scaleHeight)
            buttonText: (model.rowNo + 1).toFixed(0)
            visible: (model.rowNo < numSections) ? true : false
            color: (blockageRows.rowCount[model.rowNo] < countMin ? offColor : (blockageRows.rowCount[model.rowNo] < countMax ? autoColor : onColor))
            textColor: (blockageRows.rowCount[model.rowNo]===0 ? offTextColor : (blockageRows.rowCount[model.rowNo] === 1 ? autoTextColor : onTextColor))
        }
    }

    ListView {
        id: blockageRowList
        orientation: Qt.Horizontal
        //width: rowViewDelegate.width
        width: rowViewDelegate.width
        height: 100 * theme.scaleHeight
        anchors.left: parent.left
        anchors.right: parent.right
        //anchors.top: parent.top
        model: rowModel
        //spacing: 1
        anchors.horizontalCenter: blockageRows.horizontalCenter
        boundsMovement: Flickable.StopAtBounds
        delegate: rowViewDelegate
    }
}
