// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts 1.1
//This is a the row of on-screen section-control buttonw

Rectangle {
    id: blockageRows
    anchors.horizontalCenter: parent.horizontalCenter
    //width: 15 * theme.scaleWidth * numRows
    width: (800 * theme.scaleWidth / numRows) < 50 ? (15 * theme.scaleWidth * numRows) : (20 * theme.scaleWidth * numRows)
    //height: childrenRect.height * theme.scaleHeight

    color: "transparent"


    property int numRows:  Number(settings.setBlockrow1 + settings.setBlockrow2 + settings.setBlockrow3 + settings.setBlockrow4)*100
    property int countMin: Number(settings.setBlockcountMin)*100
    property int countMax: Number(settings.setBlockcountMax)*100
    property var rowCount: aog.rowCount // need connect with settings
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
    function setSizes() {
        //same colors for sections and zones
        numRows = (Number(settings.setBlockrow1 + settings.setBlockrow2 + settings.setBlockrow3 + settings.setBlockrow4)*100)
        countMin =  Number(settings.setBlockcountMin)*100
        countMax =  Number(settings.setBlockcountMax)*100
        }



    onNumRowsChanged: {
        rowModel.clear()
        for (var i = 0; i < numRows; i++) {
            rowModel.append( { rowNo: i } )
        y}
    }

    //callbacks, connections, and signals
    Component.onCompleted:  {
        setColors()
        setSizes()
        rowModel.clear()
        for (var i = 0; i < numRows; i++) {
            rowModel.append( { rowNo: i } )
        }
    }

Connections {
    target: settings
    function onSetDisplay_isDayModeChanged() {
        setColors()
    }
    function onSetBlockrow1Changed() {
        setSizes()
    }
}

    ListModel {
        id: rowModel
    }

    Component {
        id: rowViewDelegate
        BlockageRow {
            width: (800 * theme.scaleWidth / numRows) < 50 ? (15 * theme.scaleWidth) : (20 * theme.scaleWidth)
            //width: 15 * theme.scaleWidth
            height: (blockageRows.rowCount[model.rowNo]+20) * theme.scaleHeight
            buttonText: (model.rowNo + 1).toFixed(0)
            visible: (model.rowNo < numRows) ? true : false
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
