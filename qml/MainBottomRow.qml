import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AOG
// Interface import removed - TracksInterface now QML_SINGLETON
import "components" as Comp

RowLayout{
    property bool hydLiftIsOn: VehicleInterface.isHydLiftOn
    id:bottomButtons
    visible: Backend.isJobStarted && leftColumn.visible

    Connections {
            target: mainWindow
            function onHotKeyPressed(index) {
                switch (index) {
                case 10:
                    btnFlag.clicked()
                    break
                case 13:
                    snapToPivot.clicked()
                    break
                case 15:
                    nudgeRight.clicked()
                    break
                case 14:
                    nudgeLeft.clicked()
                    break
                }
            }
        }

    onWidthChanged: {
        theme.btnSizes[1] = width / (children.length)
        theme.buttonSizesChanged()
    }
    onVisibleChanged: {
        if (visible === false)
            height = 0
        else
            height = children.height
    }

    ComboBox {
        id: cbYouSkipNumber
        editable: false
        Layout.alignment: Qt.AlignCenter
        implicitWidth: theme.buttonSize
        implicitHeight: theme.buttonSize

        model: ListModel {
            id: model
            ListElement {text: "0"}
            ListElement {text: "1"}
            ListElement {text: "2"}
            ListElement {text: "3"}
            ListElement {text: "4"}
            ListElement {text: "5"}
            ListElement {text: "6"}
            ListElement {text: "7"}
            ListElement {text: "8"}
            ListElement {text: "9"}
            ListElement {text: "10"}
        }

        Component.onCompleted: {
            cbYouSkipNumber.currentIndex = SettingsManager.youturn_skipWidth - 1
        }

        background: Rectangle {
            implicitWidth: theme.buttonSize
            implicitHeight: theme.buttonSize
            border.width: 2
            border.color: "black"
            radius: 10
            color: "transparent"
        }

        contentItem: Text {
            text: cbYouSkipNumber.displayText
            font: cbYouSkipNumber.font
            color: "black"
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            leftPadding: 8
            elide: Text.ElideRight
        }

        onCurrentIndexChanged: {
            if (visible) {
            SettingsManager.youturn_skipWidth = cbYouSkipNumber.currentIndex + 1}
        }
    }
    Comp.IconButton3State {
        id: btnYouSkip // the "Fancy Skip" button
        //property bool isOn: false
        //isChecked: isOn
        //checkable: true
        icon1: prefix + "/images/YouSkipOff.png"
        icon2: prefix + "/images/YouSkipOn.png"
        icon3: prefix + "/images/YouSkipOn2.png"
        icon4: prefix + "/images/YouSkipOn2.png"
        //iconChecked: prefix + "/images/YouSkipOn.png"
        //buttonText: qsTr("YouSkips")
        onClicked:
         {
            //isOn = !isOn
            Backend.yt.toggleYouSkip() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
        }
    }
    Comp.MainWindowBtns { //reset trailing tool to straight back
        id: btnResetTool
        icon.source: prefix + "/images/ResetTool.png"
        buttonText: qsTr("Reset Tool")
        onClicked: Backend.resetTool() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
        visible: SettingsManager.tool_isToolTrailing //hide if front or rear 3 pt
    }
    Comp.MainWindowBtns {
        id: btnSectionMapping
        icon.source: prefix + "/images/SectionMapping.png"
        onClicked: cpSectionColor.open()
    }
    Comp.MainWindowBtns {
        id: btnTramLines
        icon.source: prefix + "/images/TramLines.png"
        buttonText: qsTr("Tram Lines")
        Layout.alignment: Qt.AlignCenter
        implicitWidth: theme.buttonSize
        implicitHeight: theme.buttonSize
        visible: SettingsManager.feature_isTramOn
    }
    Comp.MainWindowBtns {
        id: btnHydLift
        isChecked: VehicleInterface.isHydLiftOn
        checkable: true
        disabled: btnHeadland.checked
        visible: SettingsManager.ardMac_isHydEnabled && btnHeadland.visible
        icon.source: prefix + "/images/HydraulicLiftOff.png"
        iconChecked: prefix + "/images/HydraulicLiftOn.png"
        buttonText: qsTr("HydLift")
        onClicked: Backend.toggleHydLift();
    }
    Comp.MainWindowBtns {
        id: btnHeadland
        isChecked: MainWindowState.isHeadlandOn
        checkable: true
        icon.source: prefix + "/images/HeadlandOff.png"
        iconChecked: prefix + "/images/HeadlandOn.png"
        buttonText: qsTr("Headland")
        onClicked: Backend.toggleHeadlandOn();
    }
    Comp.MainWindowBtns {
        id: btnFlag
        objectName: "btnFlag"
        isChecked: false
        icon.source: prefix + contextFlag.icon
        onClicked: {
            FlagsInterface.currentFlag = FlagsInterface.flag(Backend.fixFrame.latitude,
                                                             Backend.fixFrame.longitude,
                                                             Backend.fixFrame.easting,
                                                             Backend.fixFrame.northing,
                                                             Backend.fixFrame.heading,
                                                             FlagsInterface.Red,
                                                             Number(FlagsInterface.count+1).toLocaleString(Qt.locale(),'f',0))
            flags.show();
        }
        onPressAndHold: {
            if (contextFlag.visible) {
                contextFlag.visible = false;
            } else {
                contextFlag.visible = true;
            }
        }
        buttonText: qsTr("Flag")
    }

    Comp.MainWindowBtns{
        id: nudgeLeft
        icon.source: prefix + "/images/SnapLeft.png"
        onClicked: TracksInterface.nudge(SettingsManager.as_snapDistance/-100) //spinbox returns cm, convert to metres
        visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
    }
    Comp.MainWindowBtns{
        id: snapToPivot
        icon.source: prefix + "/images/SnapToPivot.png"
        onClicked: TracksInterface.nudge_center()
        visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
    }
    Comp.MainWindowBtns{
        id: nudgeRight
        icon.source: prefix + "/images/SnapRight.png"
        onClicked: TracksInterface.nudge(SettingsManager.as_snapDistance/100) //spinbox returns cm, convert to metres
        visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
    }

    Comp.MainWindowBtns {
        property bool isOn: false
        id: btnTrack
        icon.source: prefix + "/images/TrackOn.png"
        iconChecked: prefix + "/images/TrackOn.png"
        buttonText: qsTr("Track")
        //onClicked: trackButtons.visible = !trackButtons.visible
        onClicked: {
            // TRACK RESTORATION: Smart track selection behavior
            console.log("TRACK BUTTON: Clicked - isOn:", isOn, "count:", TracksInterface.count, "idx:", TracksInterface.idx);

            if (isOn == false && TracksInterface.count > 0) {
                // Check if we have a previously active track (restored from field)
                if (TracksInterface.idx >= 0) {
                    // Use the restored/previously active track
                    console.log("TRACK: Reactivating restored track index:", TracksInterface.idx);
                    TracksInterface.select(TracksInterface.idx);
                } else {
                    // No previous track, default to first track (index 0)
                    console.log("TRACK: No previous track, selecting first track (index 0)");
                    TracksInterface.select(0);
                }
                btnTrack.isChecked = false;
                isOn = true;
            } else {
                // Open track selection menu
                console.log("TRACK: Opening track menu - isOn:", isOn, "count:", TracksInterface.count);
                trackButtons.visible = !trackButtons.visible;
            }
        }
    }

}
