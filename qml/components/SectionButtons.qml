// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
//import Settings
import AOG

//This is a the row of on-screen section-control buttonw

Rectangle {
    id: sectionButtons
    objectName: "sectionButtons"

    /*
    MockSettings { //for testing with qmlscene only
        id: settings
    }

    AOGInterface { //for testing with qmlscene only
        id: aog
    }*/

    // Tool index - which tool's sections to display
    property int toolIndex: 0

    width: 600
    height: childrenRect.height * theme.scaleHeight

    color: "red"

    property color offColor: "Crimson"
    property color offTextColor: "White"
    property color onColor: "DarkGoldenrod"
    property color onTextColor: "White"
    property color autoColor: "ForestGreen"
    property color autoTextColor: "White"

    //methods
    function setColors() {
        //same colors for sections and zones
        if (theme.displayIsDayMode) {
            sectionButtons.offColor = "Red"
            sectionButtons.offTextColor = "Black"

            sectionButtons.autoColor = "Lime"
            sectionButtons.autoTextColor = "Black"

            sectionButtons.onColor = "Yellow"
            sectionButtons.onTextColor = "Black"
        } else {
            sectionButtons.offColor = "Crimson"
            sectionButtons.offTextColor = "White"

            sectionButtons.autoColor = "ForestGreen"
            sectionButtons.autoTextColor = "White"

            sectionButtons.onColor = "DarkGoldenRod"
            sectionButtons.onTextColor = "White"
        }
    }

    Component.onCompleted: {
        console.debug(qmlLog, "toolIndex is ", toolIndex);
    }

    Connections {
        target: mainWindow
        function onHotKeyPressed(index) {
            var model = Tools.toolsProperties.tools[toolIndex].sectionButtonsModel;
            if (!model || index < 0 || index >= 8) return;

            var idx = model.index(index, 0);
            if (!idx || !idx.valid) return;

            // StateRole = Qt.UserRole + 2 = 0x0102
            var currentState = model.data(idx, 0x0102);
            if (currentState === undefined) return;

            var newState = (currentState + 1) % 3;
            Tools.setSectionButtonState(toolIndex, index, newState);
        }
    }

    Component {
        id: sectionViewDelegate
        SectionButton {

            Component.onCompleted: {
                console.log(qmlLog, model);

            }
            property int numSections: Tools.toolsProperties.tools[toolIndex].sectionButtonsModel.rowCount()
            width: (sectionButtons.width / numSections) > 40 ? (sectionButtons.width / numSections) : 40
            buttonText: (model.buttonNumber + 1).toFixed(0)
            visible: (model.buttonNumber < numSections) ? true : false
            color: (model.state === SectionState.Off ? offColor : (model.state === SectionState.Auto ? autoColor : onColor))
            textColor: (model.state ===SectionState.Off ? offTextColor : (model.state === SectionState.Auto ? autoTextColor : onTextColor))

            onButtonClicked: {
                //toggle tri state
                Tools.setSectionButtonState(toolIndex,model.buttonNumber, (model.state + 1 ) % 3)
                console.debug(qmlLog, "button clicked: ",model.buttonNumber, " new state is ", model.state);
            }
        }
    }

    ListView {
        id: sectionButtonList
        orientation: Qt.Horizontal
        width: parent.width
        height: 40
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        model: Tools.toolsProperties.tools[toolIndex].sectionButtonsModel

        boundsMovement: Flickable.StopAtBounds

        ScrollBar.horizontal: ScrollBar {
            parent: sectionButtonList.parent
            anchors.top: sectionButtonList.bottom
            anchors.left: sectionButtonList.left
            anchors.right: sectionButtonList.right
        }

        clip: true

        delegate: sectionViewDelegate
        flickableDirection: Flickable.HorizontalFlick
    }
}
