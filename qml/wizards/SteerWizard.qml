// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Steering Configuration Wizard
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import ".."
import "../components"
Dialog {
    id: steerWizardDrawer
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: false

    property int currentPage: 0
    readonly property int totalPages: 10

    function show() {
        steerWizardDrawer.visible = true
    }

    function nextPage() {
        if (currentPage < totalPages - 1) {
            currentPage++
        }
    }

    function prevPage() {
        if (currentPage > 0) {
            currentPage--
        }
    }

    contentItem: Rectangle {
        id: wizardContent
        color: aogInterface.backgroundColor
        border.color: aogInterface.blackDayWhiteNight
        border.width: 2

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            spacing: 5

            // Header
            Rectangle {
                id: header
                Layout.fillWidth: true
                height: 50 * theme.scaleHeight
                color: aogInterface.primaryColor
                RowLayout {
                    anchors.fill: parent
                    Text {
                        text: qsTr("Steering Wizard")
                        color: "white"
                        font.bold: true
                        font.pixelSize: 18
                        Layout.leftMargin: 10
                    }
                    Item { Layout.fillWidth: true }
                    IconButtonTransparent {
                        icon.source: prefix + "/images/Close.png"
                        icon.color: "white"
                        onClicked: steerWizardDrawer.visible = false
                    }
                }
            }

            // // Progress indicator
            // RowLayout {
            //     Layout.fillWidth: true
            //     height: 30 * theme.scaleHeight
            //     Repeater {
            //         model: totalPages
            //         Rectangle {
            //             Layout.fillWidth: true
            //             Layout.fillHeight: true
            //             Layout.margins: 2
            //             color: index <= steerWizardDrawer.currentPage ? aogInterface.primaryColor : "gray"
            //             Text {
            //                 anchors.centerIn: parent
            //                 text: index + 1
            //                 color: "white"
            //                 font.pixelSize: 10
            //             }
            //         }
            //     }
            // }

            // Page content
            StackLayout {
                id: pageStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: steerWizardDrawer.currentPage

                // Page 0: Start
                Item {
                    id: pageStart
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 20
                        Text {
                            text: qsTr("Auto Steer Configuration")
                            font.bold: true
                            font.pixelSize: 20
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: qsTr("Follow the wizard steps to configure your steering system")
                            wrapMode: Text.WordWrap
                            width: 300
                            horizontalAlignment: Text.AlignHCenter
                        }
                        IconButton {
                            text: qsTr("Start Wizard")
                            icon.source: prefix + "/images/StartUp.png"
                            onClicked: steerWizardDrawer.nextPage()
                        }
                    }
                }

                // Page 1: Vehicle Dimensions
                Item {
                    id: pageDimensions
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Vehicle Dimensions")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Text { text: qsTr("Wheelbase (m)") }
                        SpinBoxOneDecimal {
                            id: wheelbaseSpin
                            from: 0.5
                            to: 20
                            boundValue: SettingsManager.vehicle_wheelbase
                            onValueModified: SettingsManager.vehicle_wheelbase = value
                        }

                        Text { text: qsTr("Track Width (m)") }
                        SpinBoxOneDecimal {
                            id: trackWidthSpin
                            from: 0.5
                            to: 20
                            boundValue: SettingsManager.vehicle_trackWidth
                            onValueModified: SettingsManager.vehicle_trackWidth = value
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 2: Antenna
                Item {
                    id: pageAntenna
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Antenna Settings")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Text { text: qsTr("Antenna Pivot (m)") }
                        SpinBoxOneDecimal {
                            id: antennaPivotSpin
                            from: 0
                            to: 5
                            boundValue: SettingsManager.vehicle_antennaPivot
                            onValueModified: SettingsManager.vehicle_antennaPivot = value
                        }

                        Text { text: qsTr("Antenna Height (m)") }
                        SpinBoxOneDecimal {
                            id: antennaHeightSpin
                            from: 0
                            to: 10
                            boundValue: SettingsManager.vehicle_antennaHeight
                            onValueModified: SettingsManager.vehicle_antennaHeight = value
                        }

                        Text { text: qsTr("Antenna Offset (m)") }
                        SpinBoxOneDecimal {
                            id: antennaOffsetSpin
                            from: -5
                            to: 5
                            boundValue: SettingsManager.vehicle_antennaOffset
                            onValueModified: SettingsManager.vehicle_antennaOffset = value
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 3: Hardware Config
                Item {
                    id: pageHardware
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Hardware Configuration")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Text { text: qsTr("Motor Driver") }
                        ComboBox {
                            id: motorDriverCombo
                            model: ["IBT2", "Cytron"]
                            currentIndex: SettingsManager.setArdSteer_setting0 & 16 ? 1 : 0
                            onCurrentIndexChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                if (currentIndex === 1) sett |= 16
                                else sett &= ~16
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        Text { text: qsTr("Steer Enable") }
                        ComboBox {
                            id: steerEnableCombo
                            model: ["None", "Switch", "Button"]
                            onCurrentIndexChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                sett &= ~(32 | 64)
                                if (currentIndex === 1) sett |= 32
                                else if (currentIndex === 2) sett |= 64
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Invert WAS")
                            checked: SettingsManager.setArdSteer_setting0 & 1
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                if (checked) sett |= 1
                                else sett &= ~1
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Invert Steer")
                            checked: SettingsManager.setArdSteer_setting0 & 4
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                if (checked) sett |= 4
                                else sett &= ~4
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Invert Relays")
                            checked: SettingsManager.setArdSteer_setting0 & 2
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                if (checked) sett |= 2
                                else sett &= ~2
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 4: Sensor Type
                Item {
                    id: pageSensor
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Sensor Type")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        CheckBox {
                            text: qsTr("Encoder")
                            checked: SettingsManager.setArdSteer_setting0 & 128
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting0
                                if (checked) sett |= 128
                                else sett &= ~128
                                SettingsManager.setArdSteer_setting0 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Pressure Sensor")
                            checked: SettingsManager.setArdSteer_setting1 & 2
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting1
                                if (checked) sett |= 2
                                else sett &= ~2
                                SettingsManager.setArdSteer_setting1 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Current Sensor")
                            checked: SettingsManager.setArdSteer_setting1 & 4
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting1
                                if (checked) sett |= 4
                                else sett &= ~4
                                SettingsManager.setArdSteer_setting1 = sett
                            }
                        }

                        CheckBox {
                            text: qsTr("Danfoss Valve")
                            checked: SettingsManager.setArdSteer_setting1 & 1
                            onCheckedChanged: {
                                var sett = SettingsManager.setArdSteer_setting1
                                if (checked) sett |= 1
                                else sett &= ~1
                                SettingsManager.setArdSteer_setting1 = sett
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 5: WAS Settings
                Item {
                    id: pageWAS
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Wheel Angle Sensor")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        RowLayout {
                            Text { text: qsTr("WAS Offset:") }
                            Text { text: SettingsManager.as_wasOffset }
                        }

                        RowLayout {
                            Text { text: qsTr("Counts Per Degree:") }
                            Text { text: SettingsManager.as_countsPerDegree }
                        }

                        RowLayout {
                            Text { text: qsTr("Ackermann:") }
                            Text { text: SettingsManager.as_ackerman }
                        }

                        Text {
                            text: qsTr("Turn steering wheel full left, then right to center. Click Zero WAS when centered.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                        }

                        RowLayout {
                            IconButton {
                                text: qsTr("Zero WAS")
                                icon.source: prefix + "/images/SteerZeroSmall.png"
                                onClicked: {
                                    let newOffset = SettingsManager.as_wasOffset - SettingsManager.as_countsPerDegree * ModuleComm.actualSteerAngleDegrees;
                                    if (Math.abs(newOffset) > 3900) {
                                        timedMessage.addMessage(2000, "Exceeded Range", "Excessive Steer Angle - Cannot Zero");
                                    } else {
                                        SettingsManager.as_wasOffset = newOffset;
                                    }
                                }
                            }
                            IconButton {
                                text: qsTr("Reset")
                                icon.source: prefix + "/images/Trash.png"
                                onClicked: SettingsManager.as_wasOffset = 0
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 6: PWM Settings
                Item {
                    id: pagePWM
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        Text {
                            text: qsTr("PWM Settings")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Text { text: qsTr("Proportional Gain (Kp)") }
                        Slider {
                            id: pGainSlider
                            from: 0
                            to: 200
                            value: SettingsManager.as_Kp
                            onValueChanged: SettingsManager.as_Kp = value
                        }
                        Text { text: pGainSlider.value }

                        Text { text: qsTr("Minimum PWM") }
                        Slider {
                            id: minPWMSlider
                            from: 0
                            to: 100
                            value: SettingsManager.as_minSteerPWM
                            onValueChanged: SettingsManager.as_minSteerPWM = value
                        }
                        Text { text: minPWMSlider.value }

                        Text { text: qsTr("Low PWM") }
                        Slider {
                            id: lowPWMSlider
                            from: 0
                            to: 200
                            value: SettingsManager.as_lowSteerPWM
                            onValueChanged: SettingsManager.as_lowSteerPWM = value
                        }
                        Text { text: lowPWMSlider.value }

                        Text { text: qsTr("High PWM") }
                        Slider {
                            id: highPWMSlider
                            from: 0
                            to: 254
                            value: SettingsManager.as_highSteerPWM
                            onValueChanged: SettingsManager.as_highSteerPWM = value
                        }
                        Text { text: highPWMSlider.value }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 7: Controller Settings
                Item {
                    id: pageController
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        Text {
                            text: SettingsManager.vehicle_isStanleyUsed ? qsTr("Stanley Controller") : qsTr("Pure Pursuit Controller")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        // Stanley controls
                        ColumnLayout {
                            visible: SettingsManager.vehicle_isStanleyUsed
                            Text { text: qsTr("Distance Error Gain") }
                            Slider {
                                id: stanleyDistSlider
                                from: 0.1
                                to: 4
                                stepSize: 0.1
                                value: SettingsManager.vehicle_stanleyDistanceErrorGain
                                onValueChanged: SettingsManager.vehicle_stanleyDistanceErrorGain = value
                            }
                            Text { text: Math.round(stanleyDistSlider.value * 10) / 10 }

                            Text { text: qsTr("Heading Error Gain") }
                            Slider {
                                id: stanleyHeadingSlider
                                from: 0.1
                                to: 1.5
                                stepSize: 0.1
                                value: SettingsManager.vehicle_stanleyHeadingErrorGain
                                onValueChanged: SettingsManager.vehicle_stanleyHeadingErrorGain = value
                            }
                            Text { text: Math.round(stanleyHeadingSlider.value * 10) / 10 }

                            Text { text: qsTr("Integral Gain") }
                            Slider {
                                id: stanleyIntSlider
                                from: 0
                                to: 100
                                value: SettingsManager.vehicle_stanleyIntegralGainAB * 100
                                onValueChanged: SettingsManager.vehicle_stanleyIntegralGainAB = value / 100
                            }
                            Text { text: stanleyIntSlider.value + "%" }
                        }

                        // Pure Pursuit controls
                        ColumnLayout {
                            visible: !SettingsManager.vehicle_isStanleyUsed
                            Text { text: qsTr("Look Ahead Multiplier") }
                            Slider {
                                id: ppLookAheadSlider
                                from: 0.5
                                to: 3
                                stepSize: 0.1
                                value: SettingsManager.vehicle_goalPointLookAheadMult
                                onValueChanged: SettingsManager.vehicle_goalPointLookAheadMult = value
                            }
                            Text { text: Math.round(ppLookAheadSlider.value * 10) / 10 }

                            Text { text: qsTr("Integral Gain") }
                            Slider {
                                id: ppIntSlider
                                from: 0
                                to: 100
                                value: SettingsManager.vehicle_purePursuitIntegralGainAB * 100
                                onValueChanged: SettingsManager.vehicle_purePursuitIntegralGainAB = value / 100
                            }
                            Text { text: ppIntSlider.value + "%" }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 8: IMU/Roll Settings
                Item {
                    id: pageIMU
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("IMU / Roll Settings")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        CheckBox {
                            text: qsTr("Invert Roll")
                            checked: SettingsManager.setIMU_invertRoll
                            onCheckedChanged: SettingsManager.setIMU_invertRoll = checked
                        }

                        Text { text: qsTr("Roll Zero Offset: ") + SettingsManager.setIMU_rollZero }

                        RowLayout {
                            IconButton {
                                text: qsTr("Zero Roll")
                                icon.source: prefix + "/images/SteerZeroSmall.png"
                                onClicked: {
                                    SettingsManager.setIMU_rollZero = ModuleComm.actualRoll
                                }
                            }
                            IconButton {
                                text: qsTr("Reset")
                                icon.source: prefix + "/images/Trash.png"
                                onClicked: SettingsManager.setIMU_rollZero = 0
                            }
                        }

                        Text {
                            text: qsTr("Current Roll: ") + ModuleComm.actualRoll.toFixed(1) + "°"
                            font.pixelSize: 16
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Page 9: Free Drive / Test
                Item {
                    id: pageTest
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: qsTr("Test Steering")
                            font.bold: true
                            font.pixelSize: 16
                        }

                        RowLayout {
                            Text { text: qsTr("Set Angle: ") }
                            Text { text: VehicleInterface.driveFreeSteerAngle }
                        }

                        RowLayout {
                            Text { text: qsTr("Actual: ") }
                            Text { text: ModuleComm.actualSteerAngleDegrees.toFixed(1) + "°" }
                        }

                        RowLayout {
                            IconButton {
                                icon.source: prefix + "/images/SnapLeft.png"
                                onClicked: {
                                    if (--VehicleInterface.driveFreeSteerAngle < -40)
                                        VehicleInterface.driveFreeSteerAngle = -40
                                }
                            }
                            IconButton {
                                text: qsTr("Zero")
                                icon.source: prefix + "/images/SteerZeroSmall.png"
                                onClicked: VehicleInterface.driveFreeSteerAngle = 0
                            }
                            IconButton {
                                icon.source: prefix + "/images/SnapRight.png"
                                onClicked: {
                                    if (++VehicleInterface.driveFreeSteerAngle > 40)
                                        VehicleInterface.driveFreeSteerAngle = 40
                                }
                            }
                        }

                        Text { text: qsTr("Max Steer Angle") }
                        SpinBox {
                            id: maxSteerSpin
                            from: 10
                            to: 80
                            value: SettingsManager.vehicle_maxSteerAngle
                            onValueChanged: SettingsManager.vehicle_maxSteerAngle = value
                        }

                        Item { Layout.fillHeight: true }

                        Text {
                            text: qsTr("Wizard Complete!")
                            font.bold: true
                            font.pixelSize: 18
                            color: "green"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }

            // Navigation buttons
            RowLayout {
                Layout.fillWidth: true
                height: 50 * theme.scaleHeight
                spacing: 10

                IconButton {
                    text: qsTr("Previous")
                    icon.source: prefix + "/images/ArrowLeft.png"
                    enabled: steerWizardDrawer.currentPage > 0
                    onClicked: steerWizardDrawer.prevPage()
                }

                Item { Layout.fillWidth: true }

                IconButton {
                    text: qsTr("Next")
                    icon.source: prefix + "/images/ArrowRight.png"
                    enabled: steerWizardDrawer.currentPage < steerWizardDrawer.totalPages - 1
                    onClicked: {
                        if (steerWizardDrawer.currentPage === steerWizardDrawer.totalPages - 1) {
                            ModuleComm.modulesSend252()
                            ModuleComm.modulesSend251()
                            timedMessage.addMessage(2000, "Settings Saved", "Steering configuration saved")
                        }
                        steerWizardDrawer.nextPage()
                    }
                }
            }
        }
    }

    Timer {
        id: sendUdptimer
        interval: 1000
        onTriggered: {
            ModuleComm.modulesSend252()
            ModuleComm.modulesSend251()
        }
    }
}
