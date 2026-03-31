// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Steering Configuration Wizard
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Effects
import AOG

import ".."
import "../components"
import "../steerconfig"
Dialog {
    id: steerWizardDialog
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: true

    property int currentPage: 0
    readonly property int totalPages: 10

    function show() {
        steerWizardDialog.visible = true
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

    Rectangle {
        id: backRectangle
        color: aogInterface.backgroundColor
        anchors.fill: parent

    TopLine {
        id: steerWizardTopLine
        onBtnCloseClicked: steerWizardDialog.visible = false
        titleText: qsTr("Steering Wizard")
    }

    ProgressBar {
        id: progBar
        anchors.top: steerWizardTopLine.bottom
        from: 0
        to: steerWizardDialog.totalPages - 1
        value: steerWizardDialog.currentPage
        width: parent.width*0.8
        height: 15 * theme.scaleHeight
        Layout.margins: 5
        background: Rectangle {
            color: aogInterface.backgroundColor
            radius: 4
            border.color: "lightgray"
            border.width: 1
        }
    }

    Rectangle {
        id: wizardContent
        border.color: aogInterface.blackDayWhiteNight
        border.width: 2
        anchors.top: progBar.bottom
        anchors.left: parent.left
        anchors.bottom: bottomPanel.top
        anchors.bottomMargin: 5 * theme.scaleHeight
        width: parent.width*0.8

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            spacing: 5

            Rectangle {
                id: mainPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: aogInterface.backgroundColor
                border.color: aogInterface.blackDayWhiteNight
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5

                    // Page content
                    StackLayout {
                        id: pageStack
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: steerWizardDialog.currentPage
                        onCurrentIndexChanged: {
                            if (steerWizardDialog.currentPage === 0) steerWizardTopLine.titleText =  qsTr("Steering Wizard");
                            if (steerWizardDialog.currentPage === 1) steerWizardTopLine.titleText =  qsTr("Vehicle Dimensions");
                            if (steerWizardDialog.currentPage === 2) steerWizardTopLine.titleText =  qsTr("Antenna Settings");
                            if (steerWizardDialog.currentPage === 3) steerWizardTopLine.titleText =  qsTr("Hardware Configuration");
                            if (steerWizardDialog.currentPage === 4) steerWizardTopLine.titleText =  qsTr("Sensor Type");
                            if (steerWizardDialog.currentPage === 5) steerWizardTopLine.titleText =  qsTr("Wheel Angle Sensor");
                            if (steerWizardDialog.currentPage === 6) steerWizardTopLine.titleText =  qsTr("PWM Settings");
                            if (steerWizardDialog.currentPage === 7) steerWizardTopLine.titleText =  SettingsManager.vehicle_isStanleyUsed ? qsTr("Stanley Controller") : qsTr("Pure Pursuit Controller");
                            if (steerWizardDialog.currentPage === 8) steerWizardTopLine.titleText =  qsTr("IMU / Roll Settings");
                            if (steerWizardDialog.currentPage === 9) steerWizardTopLine.titleText =  qsTr("Test Steering");
                        }

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
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: qsTr("Follow the wizard steps to configure your steering system")
                                    wrapMode: Text.WordWrap
                                    width: 300
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                IconButton {
                                    text: qsTr("Start Wizard")
                                    icon.source: prefix + "/images/OK64.png"
                                    onClicked: steerWizardDialog.nextPage()
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }

                        // Page 1: Vehicle Dimensions
                        Item {
                            id: pageDimensions
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10 * theme.scaleHeight

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
                                spacing: 10 * theme.scaleHeight

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
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 20 * theme.scaleWidth

                                // Левая колонка: ComboBox'ы
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    spacing: 10 * theme.scaleHeight

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
                                        Layout.preferredWidth: 180 * theme.scaleWidth
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
                                        Layout.preferredWidth: 180 * theme.scaleWidth
                                    }

                                    Item { Layout.fillHeight: true }
                                }

                                ColumnLayout {
                                    Layout.fillHeight: true
                                    spacing: 10 * theme.scaleHeight

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
                        }

                        // Page 4: Sensor Type
                        Item {
                            id: pageSensor
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10 * theme.scaleHeight

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
                                spacing: 10 * theme.scaleHeight

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
                                SteerConfigSliderCustomized {
                                    id: propGainlider
                                    centerTopText: qsTr("Proportional Gain")
                                    from: 0
                                    leftText: value
                                    onValueChanged: SettingsManager.as_Kp = value, sendUdptimer.running = true
                                    stepSize: 1
                                    to: 200
                                    value: Math.round(SettingsManager.as_Kp, 0)
                                    visible: gainBtn.checked
                                    Layout.maximumWidth: 180 * theme.scaleWidth
                                    Layout.alignment: Qt.AlignLeft
                                }

                                SteerConfigSliderCustomized {
                                    id: maxLimitSlider
                                    centerTopText: qsTr("Maximum Limit")
                                    from: 0
                                    leftText: value
                                    onValueChanged: SettingsManager.as_highSteerPWM = value, sendUdptimer.running = true
                                    stepSize: 1
                                    to: 254
                                    value: Math.round(SettingsManager.as_highSteerPWM, 0)
                                    visible: gainBtn.checked
                                    Layout.maximumWidth: 180 * theme.scaleWidth
                                    Layout.alignment: Qt.AlignLeft
                                }

                                SteerConfigSliderCustomized {
                                    id: min2moveSlider
                                    centerTopText: qsTr("Minimum to Move")
                                    from: 0
                                    leftText: value
                                    onValueChanged: SettingsManager.as_minSteerPWM = value, sendUdptimer.running = true
                                    stepSize: 1
                                    to: 100
                                    value: Math.round(SettingsManager.as_minSteerPWM, 0)
                                    visible: gainBtn.checked
                                    Layout.maximumWidth: 180 * theme.scaleWidth
                                    Layout.alignment: Qt.AlignLeft
                                }
                            }
                        }

                        // Page 7: Controller Settings
                        Item {
                            id: pageController
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 5

                                // Stanley controls
                                ColumnLayout {
                                    visible: SettingsManager.vehicle_isStanleyUsed
                                    SteerConfigSliderCustomized {
                                        id: stanleyAggressivenessSlider
                                        centerTopText: qsTr("Agressiveness")
                                        from: .1
                                        onValueChanged: SettingsManager.vehicle_stanleyDistanceErrorGain = value
                                        stepSize: .1
                                        to: 4
                                        leftText: Math.round(value * 10)/10
                                        value: SettingsManager.vehicle_stanleyDistanceErrorGain
                                        visible: stanleyBtn.checked
                                        Layout.maximumWidth: 180 * theme.scaleWidth
                                        Layout.alignment: Qt.AlignLeft
                                    }

                                    SteerConfigSliderCustomized {
                                        id: overShootReductionSlider
                                        centerTopText: qsTr("OverShoot Reduction")
                                        from: .1
                                        onValueChanged: SettingsManager.vehicle_stanleyHeadingErrorGain = value
                                        stepSize: .1
                                        to: 1.5
                                        leftText: Math.round(value * 10) / 10
                                        value: SettingsManager.vehicle_stanleyHeadingErrorGain
                                        visible: stanleyBtn.checked
                                        Layout.maximumWidth: 180 * theme.scaleWidth
                                        Layout.alignment: Qt.AlignLeft
                                    }

                                    SteerConfigSliderCustomized {
                                        id: integralStanleySlider
                                        centerTopText: qsTr("Integral Gain")
                                        from: 0
                                        leftText: value
                                        onValueChanged: SettingsManager.vehicle_stanleyIntegralGainAB = value / 100
                                        stepSize: 1
                                        to: 100
                                        value: Math.round(SettingsManager.vehicle_stanleyIntegralGainAB * 100, 0)
                                        visible: stanleyBtn.checked
                                        Layout.maximumWidth: 180 * theme.scaleWidth
                                        Layout.alignment: Qt.AlignLeft
                                    }
                                }

                                // Pure Pursuit controls
                                ColumnLayout {
                                    visible: !SettingsManager.vehicle_isStanleyUsed
                                    SteerConfigSliderCustomized {
                                        id: lookAheadSpeedGainSlider
                                        centerTopText: qsTr("Look Ahead Speed Gain")
                                        from: .5
                                        onValueChanged: SettingsManager.vehicle_goalPointLookAheadMult = value
                                        stepSize: .1
                                        to: 3
                                        leftText: Math.round(value * 10) / 10
                                        value: SettingsManager.vehicle_goalPointLookAheadMult
                                        visible: ppBtn.checked
                                        Layout.maximumWidth: 180 * theme.scaleWidth
                                        Layout.alignment: Qt.AlignLeft
                                    }

                                    SteerConfigSliderCustomized {
                                        id: ppIntegralSlider
                                        centerTopText: qsTr("Integral Gain")
                                        from: 0
                                        onValueChanged: SettingsManager.vehicle_purePursuitIntegralGainAB = value / 100
                                        stepSize: 1
                                        to: 100
                                        leftText: Math.round(value *10) / 10
                                        value: SettingsManager.vehicle_purePursuitIntegralGainAB * 100
                                        visible: ppBtn.checked
                                        Layout.maximumWidth: 180 * theme.scaleWidth
                                        Layout.alignment: Qt.AlignLeft
                                    }
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
                                spacing: 10 * theme.scaleHeight

                                CheckBox {
                                    text: qsTr("Invert Roll")
                                    checked: SettingsManager.setIMU_invertRoll === true
                                    onCheckedChanged: SettingsManager.setIMU_invertRoll = checked
                                }

                                Text { text: qsTr("Roll Zero Offset: ") + (SettingsManager.setIMU_rollZero || 0) }

                                RowLayout {
                                    IconButton {
                                        text: qsTr("Zero Roll")
                                        icon.source: prefix + "/images/SteerZeroSmall.png"
                                        onClicked: {
                                            var roll = ModuleComm.actualRoll
                                            if (roll !== undefined) {
                                                SettingsManager.setIMU_rollZero = roll
                                            }
                                        }
                                    }
                                    IconButton {
                                        text: qsTr("Reset")
                                        icon.source: prefix + "/images/Trash.png"
                                        onClicked: SettingsManager.setIMU_rollZero = 0
                                    }
                                }

                                Text {
                                    text: qsTr("Current Roll: ") + ((ModuleComm.actualRoll !== undefined) ? ModuleComm.actualRoll.toFixed(1) : "---") + "°"
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
                                spacing: 10 * theme.scaleHeight

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
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }
                    }
                    // Navigation buttons
                    RowLayout {
                        Layout.fillWidth: true
                        height: 50 * theme.scaleHeight
                        spacing: 10 * theme.scaleHeight

                        Item { Layout.fillWidth: true }

                        IconButton {
                            text: qsTr("Previous")
                            icon.source: prefix + "/images/Previous.png"
                            visible: steerWizardDialog.currentPage >= 1
                            onClicked: steerWizardDialog.prevPage()
                        }

                        IconButton {
                            text: qsTr("Next")
                            icon.source: prefix + "/images/Next.png"
                            visible: {steerWizardDialog.currentPage < steerWizardDialog.totalPages - 1 && steerWizardDialog.currentPage >= 1}
                            onClicked: {
                                if (steerWizardDialog.currentPage === steerWizardDialog.totalPages - 1) {
                                    ModuleComm.modulesSend252()
                                    ModuleComm.modulesSend251()
                                    timedMessage.addMessage(2000, "Settings Saved", "Steering configuration saved")
                                }
                                steerWizardDialog.nextPage()
                            }
                        }

                        IconButton {
                            icon.source: prefix + "/images/OK64.png"
                            onClicked: {steerWizardDialog.visible = false
                                steerWizardDialog.currentPage = 0}
                            visible: steerWizardDialog.currentPage === steerWizardDialog.totalPages -1
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
    }
    // Bottom panel: WAS bar + Set/Act/Err/PWM


    Rectangle {
        id: bottomPanel
        width: parent.width*0.8
        height: 120 * theme.scaleHeight
        color: aogInterface.backgroundColor
        border.color: aogInterface.blackDayWhiteNight
        border.width: 1
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 10 * theme.scaleHeight

            // Steer Status indicator (WizSteerDot equivalent)
            Rectangle {
                height: 80 * theme.scaleHeight
                width: height
                color: aogInterface.backgroundColor

                Image {
                    id: steerStatusIndicator
                    anchors.fill: parent
                    source: prefix + "/images/WizSteerDot.png"
                    visible: true
                    property bool steerSwitchHigh: ModuleComm.steerSwitchHigh
                    property bool autoSteerOn: MainWindowState.isBtnAutoSteerOn
                    property int moduleCounter: ModuleComm.steerModuleConnectedCounter

                    onSteerSwitchHighChanged: updateColor()
                    onAutoSteerOnChanged: updateColor()
                    onModuleCounterChanged: updateColor()
                    onVisibleChanged: updateColor()
                    Component.onCompleted: updateColor()

                    function updateColor() {
                            if (steerSwitchHigh) {
                                parent.color = "#ff0000"
                            } else if (autoSteerOn) {
                                perent.color = "#00ff00"
                            } else if (moduleCounter > 30) {
                                parent.color = "#ff00ff"
                            } else {
                                parent.color = "#ffff00"
                            }
                    }
                }
            }

            // WAS bar (pbarLeft + pbarRight equivalent)
            WasBar {
                id: wizardWasBar
                wasvalue: ModuleComm.actualSteerAngleDegrees * 10
                Layout.preferredWidth: 200 * theme.scaleWidth
                height: 20 * theme.scaleHeight
            }

            // Values panel
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2* theme.scaleHeight

                // Set
                Text {
                    text: qsTr("Set: %1").arg(Math.round(VehicleInterface.driveFreeSteerAngle * 10) / 10)
                    font.bold: true
                    font.pixelSize: 14
                }

                // Act
                Text {
                    text: qsTr("Act: %1").arg(Math.round(ModuleComm.actualSteerAngleDegrees * 10) / 10)
                    font.bold: true
                    font.pixelSize: 14
                }

                // Error
                Text {
                    property double err: ModuleComm.actualSteerAngleDegrees - VehicleInterface.driveFreeSteerAngle
                    text: qsTr("Err: %1").arg(Math.round(err * 10) / 10)
                    font.bold: true
                    font.pixelSize: 14
                    color: err > 0 ? "red" : "darkgreen"
                }

                // PWM
                Text {
                    text: qsTr("PWM: %1").arg(ModuleComm.pwmDisplay)
                    font.bold: true
                    font.pixelSize: 16
                }
            }
        }
    }

    // Page 5: WAS Settings
    Rectangle {
        id: rightPanel
        width: parent.width*0.2
        //height: 120 * theme.scaleHeight
        color: aogInterface.backgroundColor
        anchors.right: parent.right
        anchors.top: progBar.bottom
        anchors.bottom: stopBtn.top
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10 * theme.scaleHeight

            Text { text: qsTr("WAS Offset:") }
            Text { text: SettingsManager.as_wasOffset
                font.bold: true}
            Spacer{}

            Text { text: qsTr("Counts P Deg:") }
            Text { text: SettingsManager.as_countsPerDegree
                font.bold: true}
            Spacer{}

            Text { text: qsTr("Ackermann:") }
            Text { text: SettingsManager.as_ackerman
                font.bold: true}

            Item { Layout.fillHeight: true }
        }
    }

    IconButtonTransparent {
        id: stopBtn
        icon.source: prefix + "/images/Stop.png"
        icon.color: "white"
        onClicked: {steerWizardDialog.visible = false
            steerWizardDialog.currentPage = 0}
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
    }
}
