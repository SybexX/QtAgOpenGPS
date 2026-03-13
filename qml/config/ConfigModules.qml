// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Config Hyd lift timing etc
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
//import Settings
import AOG

import ".."
import "../components"

/*todo:
 can't find setting for "invert relays"
 not sure about the "send button"
 */
Rectangle{
    id: configModules
    anchors.fill: parent
    color: aogInterface.backgroundColor

    property bool notSaved: false

    onVisibleChanged: {
        load_settings()
    }

    function load_settings()
    {
        if (visible) {
            var sett = SettingsManager.ardMac_setting0

            if ((sett & 1) === 0 ) cboxMachInvertRelays.checked = false
            else cboxMachInvertRelays.checked = true

            if ((sett & 2) === 0 ) cboxIsHydOn.checked = false
            else cboxIsHydOn.checked = true

            nudRaiseTime.value = SettingsManager.ardMac_hydRaiseTime
            nudLowerTime.value = SettingsManager.ardMac_hydLowerTime

            nudUser1.value = SettingsManager.ardMac_user1
            nudUser2.value = SettingsManager.ardMac_user2
            nudUser3.value = SettingsManager.ardMac_user3
            nudUser4.value = SettingsManager.ardMac_user4
            cboxIsHydOn.checked = SettingsManager.ardMac_isHydEnabled

            notSaved = false
        }
    }

    function save_settings() {
        var set = 1
        var reset = 2046
        var sett = 0

        if (cboxMachInvertRelays.checked) sett |= set
        else sett &= reset

        set <<=1
        reset <<= 1
        reset += 1

        if(cboxIsHydOn.checked) sett |= set
        else sett &= reset

        SettingsManager.ardMac_setting0 = sett
        SettingsManager.ardMac_hydRaiseTime = nudRaiseTime.value
        SettingsManager.ardMac_hydLowerTime = nudLowerTime.value

        SettingsManager.ardMac_user1 = nudUser1.value
        SettingsManager.ardMac_user2 = nudUser2.value
        SettingsManager.ardMac_user3 = nudUser3.value
        SettingsManager.ardMac_user4 = nudUser4.value

        SettingsManager.vehicle_hydraulicLiftLookAhead = nudHydLiftLookAhead.value
        SettingsManager.ardMac_isHydEnabled = cboxIsHydOn.checked

        ModuleComm.modulesSend238()
    }

    ColumnLayout {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: btnPinsSave.top
        anchors.top: parent.top
        anchors.margins: 10 * theme.scaleWidth

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10 * theme.scaleHeight
            Layout.bottomMargin: 20 * theme.scaleHeight
            text: qsTr("Machine Module")
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20 * theme.scaleWidth

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.7

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 15 * theme.scaleHeight
                    text: qsTr("Hydraulic Lift Config")
                    font.bold: true
                    color: aogInterface.textColor
                }

                GridLayout {
                    id: hydGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 3
                    rows: 2
                    columnSpacing: 15 * theme.scaleWidth
                    rowSpacing: 15 * theme.scaleHeight

                    IconButtonColor {
                        id: cboxIsHydOn
                        Layout.preferredWidth: 150 * theme.scaleWidth
                        Layout.preferredHeight: 120 * theme.scaleHeight
                        Layout.alignment: Qt.AlignCenter
                        icon.source: prefix + "/images/SwitchOff.png"
                        iconChecked: prefix + "/images/SwitchOn.png"
                        checkable: true
                        onClicked: notSaved = true
                    }

                    SpinBoxCustomized {
                        id: nudRaiseTime
                        Layout.fillWidth: true
                        from: 1
                        to: 255
                        editable: true
                        enabled: cboxIsHydOn.checked
                        text: qsTr("Raise Time (secs)")
                        onValueChanged: notSaved = true
                    }

                    Image {
                        Layout.preferredWidth: 180 * theme.scaleWidth
                        Layout.preferredHeight: 180 * theme.scaleHeight
                        Layout.alignment: Qt.AlignCenter
                        source: prefix + "/images/Config/ConMa_LiftRaiseTime.png"
                        fillMode: Image.PreserveAspectFit
                    }

                    SpinBoxCustomized {
                        id: nudHydLiftLookAhead
                        Layout.fillWidth: true
                        from: 1
                        to: 20
                        editable: true
                        enabled: cboxIsHydOn.checked
                        text: qsTr("Look Ahead (secs)")
                        onValueChanged: notSaved = true
                        decimals: 1
                    }

                    SpinBoxCustomized {
                        id: nudLowerTime
                        Layout.fillWidth: true
                        from: 1
                        to: 255
                        editable: true
                        enabled: cboxIsHydOn.checked
                        text: qsTr("Lower Time (secs)")
                        onValueChanged: notSaved = true
                    }

                    Image {
                        Layout.preferredWidth: 180 * theme.scaleWidth
                        Layout.preferredHeight: 180 * theme.scaleHeight
                        Layout.alignment: Qt.AlignCenter
                        source: prefix + "/images/Config/ConMa_LiftLowerTime.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }


            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: aogInterface.borderColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.3
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 15 * theme.scaleHeight
                    text: qsTr("User Settings")
                    font.bold: true
                    color: aogInterface.textColor
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        SpinBoxCustomized {
                            id: nudUser1
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            from: 0
                            to: 255
                            editable: true
                            text: qsTr("User 1")
                            onValueChanged: notSaved = true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        SpinBoxCustomized {
                            id: nudUser2
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            from: 0
                            to: 255
                            editable: true
                            text: qsTr("User 2")
                            onValueChanged: notSaved = true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        SpinBoxCustomized {
                            id: nudUser3
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            from: 0
                            to: 255
                            editable: true
                            text: qsTr("User 3")
                            onValueChanged: notSaved = true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        SpinBoxCustomized {
                            id: nudUser4
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            from: 0
                            to: 255
                            editable: true
                            text: qsTr("User 4")
                            onValueChanged: notSaved = true
                        }
                    }



                }
            }
        }
    }
    IconButtonTransparent {
        id: cboxMachInvertRelays
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/Config/ConSt_InvertRelay.png"
        checkable: true
        enabled: cboxIsHydOn.checked
        onClicked: notSaved = true
        Text{
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.right
            anchors.leftMargin: 5
            text: qsTr("Invert Relays")
        }
    }
    IconButtonTransparent{
        id: btnPinsSave
        anchors.right: parent.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        anchors.bottom: parent.bottom
        icon.source: notSaved?prefix + "/images/ToolAcceptNotSaved.png":prefix + "/images/ToolAcceptChange.png"
        Text{
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.left
            anchors.rightMargin: 5
            text: qsTr("Send + Save");
        }
        onClicked: { save_settings(); notSaved = false }
    }
}

