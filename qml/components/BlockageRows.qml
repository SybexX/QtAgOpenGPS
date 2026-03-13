// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG
//This is a the row of on-screen section-control buttonw

Rectangle {
    id: blockageRows
    objectName: "blockageRows"
    anchors.horizontalCenter: parent.horizontalCenter
    width: 15 * theme.scaleWidth * numRows
    //width: (800 * theme.scaleWidth / numRows) < 50 ? (15 * theme.scaleWidth * numRows) : (20 * theme.scaleWidth * numRows)
    //height: childrenRect.height * theme.scaleHeight

    color: "transparent"

    // Qt 6.8 QProperty + BINDABLE: Simple properties to allow setProperty() updates from C++
    property bool viewSwitch: false
    property bool isDayMode: SettingsManager.display_isDayMode
    property int seedBlockRow1: SettingsManager.seed_blockRow1
    property int seedBlockRow2: SettingsManager.seed_blockRow2
    property int seedBlockRow3: SettingsManager.seed_blockRow3
    property int seedBlockRow4: SettingsManager.seed_blockRow4
    property int seedBlockCountMin: SettingsManager.seed_blockCountMin
    property int seedBlockCountMax: SettingsManager.seed_blockCountMax

    // Threading Phase 1: Seed blockage configuration
    property int numRows:  Number(seedBlockRow1 + seedBlockRow2 + seedBlockRow3 + seedBlockRow4)
    property int countMin: Number(seedBlockCountMin)
    property int countMax: Number(seedBlockCountMax)
    property color offColor: "Crimson"
    property color offTextColor: "White"
    property color onColor: "DarkGoldenrod"
    property color onTextColor: "White"
    property color autoColor: "ForestGreen"
    property color autoTextColor: "White"

    //methods
    function setColors() {
        //same colors for sections and zones
        if (isDayMode) {
            blockageRows.offColor = "Red"
            blockageRows.offTextColor = "Black"
            blockageRows.onColor = "Yellow"
            blockageRows.onTextColor = "Black"
            blockageRows.autoColor = "Lime"
            blockageRows.autoTextColor = "Black"
        } else {
            blockageRows.offColor = "Crimson"
            blockageRows.offTextColor = "White"
            blockageRows.onColor = "DarkGoldenRod"
            blockageRows.onTextColor = "White"
            blockageRows.autoColor = "ForestGreen"
            blockageRows.autoTextColor = "White"
        }
    }
    function setSizes() {
        //same colors for sections and zones
        numRows = Number(seedBlockRow1 + seedBlockRow2 + seedBlockRow3 + seedBlockRow4)
        countMin =  Number(seedBlockCountMin)
        countMax =  Number(seedBlockCountMax)
    }

    //callbacks, connections, and signals
    Component.onCompleted:  {
        setColors()
        setSizes()
    }

    // Qt 6.8 QProperty + BINDABLE: Functions called in existing Component.onCompleted above

    Component {
        id: rowViewDelegate
        BlockageRow {
            width: (800 * theme.scaleWidth / numRows) < 50 ? (800 * theme.scaleWidth / numRows) : (20 * theme.scaleWidth)
            //height: (10 * theme.scaleWidth)
            //anchors.bottom: parent.bottom
            height: (model.count * 40/(Blockage.max+1)+20)*theme.scaleHeight<45*theme.scaleHeight?(model.count * 40/(Blockage.max+1)+20)*theme.scaleHeight:40*theme.scaleHeight
            useColorBasedAnchors: viewSwitch
            buttonText: (model.index + 1).toFixed(0)
            // visible: (model.rowNo < numRows) ? true : false
            color: {
                var dayMode = isDayMode;
                var off = dayMode ? "Red" : "Crimson";
                var auto = dayMode ? "Lime" : "ForestGreen";
                var on = dayMode ? "Yellow" : "DarkGoldenRod";
                return model.count < countMin ? off : (model.count < countMax ? auto : on);
            }
            textColor: "black"
        }
    }

    ListView {
        id: blockageRowList
        orientation: Qt.Horizontal
        width: contentWidth
        height: 100 * theme.scaleHeight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        model: Blockage.blockageModel
        delegate: rowViewDelegate
    }
}
