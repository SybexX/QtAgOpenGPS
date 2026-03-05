// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Pinout for hyd lift/sections
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import ".."
import "../components"

Rectangle{
    id: configModulesBlockage
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false

    property bool notSaved: false

    onVisibleChanged: {
        if (visible) load_settings()
    }

    function load_settings() {
        // Threading Phase 1: Seed blockage configuration
        graincountMin.boundValue = SettingsManager.seed_blockCountMin
        graincountMax.boundValue = SettingsManager.seed_blockCountMax
        modulerows1.boundValue = SettingsManager.seed_blockRow1
        modulerows2.boundValue = SettingsManager.seed_blockRow2
        modulerows3.boundValue = SettingsManager.seed_blockRow3
        modulerows4.boundValue = SettingsManager.seed_blockRow4
        cboxIsBlockageOn.checked = SettingsManager.seed_blockageIsOn

        notSaved = false
    }

    function save_settings() {
        // Threading Phase 1: Save seed blockage configuration
        SettingsManager.seed_blockCountMin = graincountMin.value
        SettingsManager.seed_blockCountMax = graincountMax.value
        SettingsManager.seed_blockRow1 = modulerows1.value
        SettingsManager.seed_blockRow2 = modulerows2.value
        SettingsManager.seed_blockRow3 = modulerows3.value
        SettingsManager.seed_blockRow4 = modulerows4.value
        SettingsManager.seed_blockageIsOn = cboxIsBlockageOn.checked
        SettingsManager.seed_numRows = Number(SettingsManager.seed_blockRow1 + SettingsManager.seed_blockRow2 + SettingsManager.seed_blockRow3 + SettingsManager.seed_blockRow4)
        blockageRows.setSizes()
        notSaved = false

        ModuleComm.modulesSend245()
    }

    ColumnLayout {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: btnPinsSave.top
        anchors.top: parent.top
        anchors.margins: 10 * theme.scaleWidth

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10 * theme.scaleHeight
            Layout.bottomMargin: 20 * theme.scaleHeight
            text: qsTr("Planter Monitor")
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20 * theme.scaleWidth

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.5
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 15 * theme.scaleHeight
                    Layout.preferredHeight: 30 * theme.scaleHeight
                    text: qsTr("Module Rows Configuration")
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

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: modulerows1
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockRow1
                                onValueModified: {
                                    SettingsManager.seed_blockRow1 = value
                                    notSaved = true
                                }

                                Text {
                                    text: qsTr("Rows on module 1")
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: modulerows2
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockRow2
                                onValueModified: {
                                    SettingsManager.seed_blockRow2 = value
                                    notSaved = true
                                }
                                Text {
                                    text: qsTr("Rows on module 2")
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: modulerows3
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockRow3
                                onValueModified: {
                                    SettingsManager.seed_blockRow3 = value
                                    notSaved = true
                                }
                                Text {
                                    text: qsTr("Rows on module 3")
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: modulerows4
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockRow4
                                onValueModified: {
                                    SettingsManager.seed_blockRow4 = value
                                    notSaved = true
                                }
                                Text {
                                    text: qsTr("Rows on module 4")
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
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
                Layout.preferredWidth: parent.width * 0.5
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 15 * theme.scaleHeight
                    Layout.preferredHeight: 30 * theme.scaleHeight
                    text: qsTr("Grain Count Thresholds")
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

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: graincountMin
                                Layout.fillWidth: true
                                from: 0
                                to: 10000
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockCountMin
                                onValueModified: {
                                    SettingsManager.seed_blockCountMin = value
                                    notSaved = true
                                }
                                Text {
                                    text: qsTr("Minimum grain count " + Utils.per_unit())
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            //spacing: 5 * theme.scaleHeight

                            SpinBoxCustomized {
                                id: graincountMax
                                Layout.fillWidth: true
                                from: 0
                                to: 10000
                                editable: true
                                enabled: cboxIsBlockageOn.checked
                                boundValue: SettingsManager.seed_blockCountMax
                                onValueModified: {
                                    SettingsManager.seed_blockCountMax = value
                                    notSaved = true
                                }

                                Text {
                                    text: qsTr("Maximum grain count " + Utils.per_unit())
                                    font.bold: true
                                    color: aogInterface.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    anchors.bottomMargin: 5 * theme.scaleWidth
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                    }
                }
            }
        }
        }
    IconButtonTransparent {
        id: back
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        enabled: cboxIsBlockageOn.checked
        icon.source: prefix + "/images/back-button.png"
        onClicked: {
            graincountMin.boundValue = 0
            graincountMax.boundValue = 0
            modulerows1.boundValue = 0
            modulerows2.boundValue = 0
            modulerows3.boundValue = 0
            modulerows4.boundValue = 0
            crops.currentIndex[0] = 0
            notSaved = true
        }
    }

    IconButtonTransparent {
        id: loadSetBlockage
        anchors.bottom: parent.bottom
        anchors.left: back.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        enabled: cboxIsBlockageOn.checked
        icon.source: prefix + "/images/UpArrow64.png"
        onClicked: {
            load_settings()
            notSaved = true
        }
    }


    IconButtonColor {
        id: cboxIsBlockageOn
        height: loadSetBlockage.height
        anchors.bottom: parent.bottom
        anchors.left: loadSetBlockage.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/SwitchOff.png"
        iconChecked: prefix + "/images/SwitchOn.png"
        checkable: true
        onClicked: notSaved = true
        Text {
            text: qsTr("Enable Blockage Monitoring")
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.top
            anchors.bottomMargin: 5 * theme.scaleWidth
            color: aogInterface.textColor
        }
    }

    IconButtonTransparent {
        id: btnPinsSave
        anchors.right: parent.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        anchors.bottom: parent.bottom
        icon.source: notSaved?prefix + "/images/ToolAcceptNotSaved.png":prefix + "/images/ToolAcceptChange.png"
        onClicked: save_settings()
        Text{
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.left
            anchors.rightMargin: 5
            text: qsTr("Send + Save");
        }
    }
}
