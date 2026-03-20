import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AOG 1.0
import "components" as Comp

Item {
    id: bottomButtons
    visible: Backend.isJobStarted && leftColumn.visible
    height: visible ? row.implicitHeight + 2 * padding : 0
    width: parent ? parent.width : 0
    readonly property real padding: 5 * theme.scaleHeight
    property bool hydLiftIsOn: VehicleInterface.isHydLiftOn

    onWidthChanged: {
        if (row.children.length > 0) {
            theme.btnSizes[1] = width / row.children.length
            theme.buttonSizesChanged()
        }
        row.positionButtons();
    }

    Rectangle {
        id: backgroundRect
        x: row.x - 3
        y: row.y - 3
        width: row.width + 6
        height: row.height + 6
        color: SettingsManager.display_isDayMode?SettingsManager.display_colorDayFrame:SettingsManager.display_colorNightFrame
        opacity: 0.5
        radius: 10
        z: -1
    }

    Row {
        id: row
        spacing:  10 * theme.scaleWidth
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        y: padding + 3

        function positionButtons() {
            var totalWidth = 0;
            var visibleCount = 0;

            for (var i = 0; i < children.length; i++) {
                if (children[i].visible) {
                    totalWidth += children[i].width;
                    visibleCount++;
                }
            }

            totalWidth += (visibleCount - 1) * spacing;

            anchors.horizontalCenterOffset = 0;

            var availableWidth = parent.width;
            if (totalWidth < availableWidth) {
                anchors.horizontalCenterOffset = 0;
            }
        }

        Connections {
            target: mainWindow
            function onHotKeyPressed(index) {
                switch (index) {
                case 10: btnFlag.clicked(); break
                case 13: snapToPivot.clicked(); break
                case 15: nudgeRight.clicked(); break
                case 14: nudgeLeft.clicked(); break
                }
            }
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
                    SettingsManager.youturn_skipWidth = cbYouSkipNumber.currentIndex + 1
                }
            }
        }

        Comp.IconButton3State {
            id: btnYouSkip
            icon1: prefix + "/images/YouSkipOff.png"
            icon2: prefix + "/images/YouSkipOn.png"
            icon3: prefix + "/images/YouSkipOn3.png"
            icon4: prefix + "/images/YouSkipOn2.png"
            onClicked: {
                Backend.yt.toggleYouSkip()
            }
        }

        Comp.MainWindowBtns {
            id: btnResetTool
            icon.source: prefix + "/images/ResetTool.png"
            buttonText: qsTr("Reset Tool")
            onClicked: Backend.resetTool()
            visible: SettingsManager.tool_isToolTrailing
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
            //disabled: btnHeadland.checked
            //visible: SettingsManager.ardMac_isHydEnabled && btnHeadland.visible
            icon.source: prefix + "/images/HydraulicLiftOff.png"
            iconChecked: prefix + "/images/HydraulicLiftOn.png"
            buttonText: qsTr("HydLift")
            onClicked: Backend.toggleHydLift()
        }

        Comp.MainWindowBtns {
            id: btnHeadland
            isChecked: MainWindowState.isHeadlandOn
            checkable: true
            icon.source: prefix + "/images/HeadlandOff.png"
            iconChecked: prefix + "/images/HeadlandOn.png"
            buttonText: qsTr("Headland")
            onClicked: {Backend.toggleHeadlandOn()
            btnHydLift.visible = !btnHydLift.visible && btnHeadland.visible && SettingsManager.ardMac_isHydEnabled
            }
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
                flags.show()
            }
            onPressAndHold: {
                contextFlag.visible = !contextFlag.visible
            }
            buttonText: qsTr("Flag")
        }

        Comp.MainWindowBtns {
            id: nudgeLeft
            icon.source: prefix + "/images/SnapLeft.png"
            onClicked: TracksInterface.nudge(SettingsManager.as_snapDistance / -100)
            visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
        }

        Comp.MainWindowBtns {
            id: snapToPivot
            icon.source: prefix + "/images/SnapToPivot.png"
            onClicked: TracksInterface.nudge_center()
            visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
        }

        Comp.MainWindowBtns {
            id: nudgeRight
            icon.source: prefix + "/images/SnapRight.png"
            onClicked: TracksInterface.nudge(SettingsManager.as_snapDistance / 100)
            visible: SettingsManager.feature_isNudgeOn && TracksInterface.idx > -1
        }

        Comp.MainWindowBtns {
            property bool isOn: false
            id: btnTrack
            icon.source: prefix + "/images/TrackOn.png"
            iconChecked: prefix + "/images/TrackOn.png"
            buttonText: qsTr("Track")
            onClicked: {
                console.log("TRACK BUTTON: Clicked - isOn:", isOn, "count:", TracksInterface.count, "idx:", TracksInterface.idx)

                if (!isOn && TracksInterface.count > 0) {
                    if (TracksInterface.idx >= 0) {
                        console.log("TRACK: Reactivating restored track index:", TracksInterface.idx)
                        TracksInterface.select(TracksInterface.idx)
                    } else {
                        console.log("TRACK: No previous track, selecting first track (index 0)")
                        TracksInterface.select(0)
                    }
                    btnTrack.isChecked = false
                    isOn = true
                } else {
                    console.log("TRACK: Opening track menu - isOn:", isOn, "count:", TracksInterface.count)
                    trackButtons.visible = !trackButtons.visible
                }
            }
        }
    }
}
