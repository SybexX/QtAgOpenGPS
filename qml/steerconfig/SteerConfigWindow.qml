// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// The window where we set WAS, Stanley, PP, PWM
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
//import Settings
import AOG


import ".."
import "../components"

Drawer {
    id: steerConfigWindow
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true

    // Local computed properties for steering angle display
    readonly property double steerAngleActualRounded: Math.round(ModuleComm.actualSteerAngleDegrees*100)/100
    readonly property double steerAngleSetRounded: Math.round(VehicleInterface.driveFreeSteerAngle*100)/100

    function show (){
        steerConfigWindow.visible = true
        steerBtn.isChecked = true
    }

    contentItem: Rectangle{
        id: steerConfigFirst
        anchors.fill: parent

        border.color: aogInterface.blackDayWhiteNight
        border.width: 1
        color: aogInterface.backgroundColor
        visible: true

        Item{
            id: steerSlidersConfig
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height *0.75
            width: steerConfigFirst.width
            anchors.rightMargin: 2 * theme.scaleHeight
            anchors.leftMargin: 2 * theme.scaleHeight
            ButtonGroup {
                buttons: buttonsTop.children
            }

            RowLayout{
                id: buttonsTop
                anchors.top: parent.top
                anchors.topMargin: 5
                //anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 2 * theme.scaleWidth
                IconButtonColor{
                    id: steerBtn
                    checkable: true
                    //checked: true
                    colorChecked: "lightgray"
                    icon.source: prefix + "/images/Steer/ST_SteerTab.png"
                    implicitHeight: 50 * theme.scaleHeight
                    implicitWidth: parent.width /3 - 5 * theme.scaleWidth
                }
                IconButtonColor{
                    id: gainBtn
                    checkable: true
                    colorChecked: "lightgray"
                    icon.source: prefix + "/images/Steer/ST_GainTab.png"
                    implicitHeight: 50 * theme.scaleHeight
                    implicitWidth: parent.width /3 - 5 * theme.scaleWidth
                }
                IconButtonColor{
                    id: stanleyBtn
                    checkable: true
                    colorChecked: "lightgray"
                    icon.source: prefix + "/images/Steer/ST_StanleyTab.png"
                    implicitHeight: 50 * theme.scaleHeight
                    implicitWidth: parent.width /3 - 5 * theme.scaleWidth
                    visible: SettingsManager.vehicle_isStanleyUsed
                }
                IconButtonColor{
                    id: ppBtn
                    checkable: true
                    colorChecked: "lightgray"
                    icon.source: prefix + "/images/Steer/Sf_PPTab.png"
                    implicitHeight: 50 * theme.scaleHeight
                    implicitWidth: parent.width /3 - 5 * theme.scaleWidth
                    visible: !SettingsManager.vehicle_isStanleyUsed
                }
            }

            WasBar{
                id: wasbar
                wasvalue: ModuleComm.actualSteerAngleDegrees*10
                width: steerConfigFirst.width - 20 * theme.scaleWidth
                visible: steerBtn.checked
                anchors.top: buttonsTop.bottom
                anchors.bottomMargin: 8 * theme.scaleHeight
                anchors.topMargin: 8 * theme.scaleHeight
                anchors.horizontalCenter: parent.horizontalCenter
            }

            IconButtonTransparent { //was zero button
                id: waszerobtn
                implicitWidth: 80 * theme.scaleWidth
                implicitHeight: 50 * theme.scaleHeight
                anchors.horizontalCenter: parent.horizontalCenter
                icon.source: prefix + "/images/SteerCenter.png"
                anchors.top: wasbar.bottom
                anchors.topMargin: 8 * theme.scaleHeight
                visible: steerBtn.checked
                onClicked: {
                    let newOffset = SettingsManager.as_wasOffset - cpDegSlider.value * ModuleComm.actualSteerAngleDegrees;
                    if (Math.abs(newOffset) > 3900) {
                        timedMessage.addMessage(2000, "Exceeded Range", "Excessive Steer Angle - Cannot Zero");
                    } else {
                        SettingsManager.as_wasOffset = newOffset;
                        sendUdptimer.running = true;
                    }
                }
            }

            Rectangle{
                id: slidersArea
                color: aogInterface.backgroundColor
                anchors.top: waszerobtn.bottom
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.bottom: angleInfo.top
                anchors.topMargin: 8 * theme.scaleHeight

                ColumnLayout{
                    id: slidersColumn
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5 * theme.scaleHeight
                    anchors.left: parent.left
                    anchors.leftMargin: 15 * theme.scaleWidth
                    anchors.top: parent.top
                    anchors.topMargin: 5 * theme.scaleHeight
                    width: parent.width * 0.4
                    Layout.alignment: Qt.AlignLeft

                    /* Here, we just set which Sliders we want to see, and the
                      ColumnLayout takes care of the rest. No need for
                      4 ColumnLayouts*/
                    //region WAStab

                    SteerConfigSliderCustomized {
                        property int wasOffset: 0
                        id: wasZeroSlider
                        centerTopText: qsTr("WAS Zero")
                        from: -4000
                        leftText: Utils.decimalRound(value / cpDegSlider.value, 2)
                        //onValueChanged: Settings.as_wasOffset = value * cpDegSlider.value, ModuleComm.moduleSend252()
                        onValueChanged: SettingsManager.as_wasOffset = value * cpDegSlider.value, sendUdptimer.running = true
                        to: 4000
                        stepSize: 20
                        value: SettingsManager.as_wasOffset / cpDegSlider.value
                        visible: steerBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }
                    SteerConfigSliderCustomized {
                        id: cpDegSlider
                        centerTopText: qsTr("Counts per Degree")
                        from: 1
                        leftText: value
                        onValueChanged: SettingsManager.as_countsPerDegree = value, sendUdptimer.running = true
                        stepSize: 1
                        to: 255
                        value: Math.round(SettingsManager.as_countsPerDegree, 0)
                        visible: steerBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }
                    SteerConfigSliderCustomized {
                        id: ackermannSlider
                        centerTopText: qsTr("AckerMann")
                        from: 1
                        leftText: value
                        onValueChanged: SettingsManager.as_ackerman = value, sendUdptimer.running = true
                        stepSize: 1
                        to: 200
                        value: Math.round(SettingsManager.as_ackerman, 0)
                        visible: steerBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }
                    SteerConfigSliderCustomized {
                        id: maxSteerSlider
                        centerTopText:qsTr("Max Steer Angle")
                        from: 10
                        leftText: value
                        onValueChanged: SettingsManager.vehicle_maxSteerAngle = value
                        stepSize: 1
                        to: 80
                        value: Math.round(SettingsManager.vehicle_maxSteerAngle)
                        visible: steerBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }

                    //endregion WAStab

                    //region PWMtab
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

                    //endregion PWMtab

                    //region StanleyTab
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
                        centerTopText: qsTr("Integral")
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

                    //endregion StanleyTab
                    //
                    //region PurePursuitTab
                    SteerConfigSliderCustomized {
                        id: acqLookAheadSlider
                        centerTopText: qsTr("Acquire Look Ahead")
                        from: 1
                        onValueChanged: SettingsManager.vehicle_goalPointLookAhead = value
                        stepSize: .1
                        leftText: Math.round(value * 10) / 10
                        to: 7
                        value: SettingsManager.vehicle_goalPointLookAhead
                        visible: ppBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }
                    SteerConfigSliderCustomized {
                        id: holdLookAheadSlider
                        centerTopText: qsTr("Hold Look Ahead")
                        from: 1
                        stepSize: .1
                        leftText: Math.round(value * 10) / 10
                        onValueChanged: SettingsManager.vehicle_goalPointLookAheadHold = Utils.decimalRound(value, 1)
                        to: 7
                        value: SettingsManager.vehicle_goalPointLookAheadHold
                        visible: ppBtn.checked
                        Layout.maximumWidth: 180 * theme.scaleWidth
                        Layout.alignment: Qt.AlignLeft
                    }
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
                        centerTopText: qsTr("Integral")
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
                    //endregion PurePursuitTab
                }
                Image {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    height: slidersColumn.height
                    source: prefix + (steerBtn.checked === true ? "/images/Steer/Sf_SteerTab.png" :
                                                                  gainBtn.checked === true ? "/images/Steer/Sf_GainTab.png" :
                                                                                             stanleyBtn.checked === true ? "/images/Steer/Sf_Stanley.png" :
                                                                                                                           "/images/Steer/Sf_PP.png")
                    width: parent.width
                }
            }

            Rectangle{
                id: angleInfo
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 50 * theme.scaleHeight
                width: parent.width - 2 * theme.scaleWidth
                anchors.horizontalCenter: parent.horizontalCenter

                MouseArea{
                    id: angleInfoMouse
                    anchors.fill: parent
                    onClicked: pwmWindow.visible = !pwmWindow.visible

                }
                RowLayout{
                    id: angleInfoRow
                    anchors.fill: parent
                    spacing: 5 * theme.scaleWidth
                    width: parent.width - 2 * theme.scaleWidth

                    Text {
                        text: qsTr("Set: " + steerConfigWindow.steerAngleSetRounded)
                        //text: qsTr("Set: " + VehicleInterface.driveFreeSteerAngle
                        Layout.fillWidth: true
                    }
                    Text {
                        text: qsTr("Act: " + steerConfigWindow.steerAngleActualRounded)
                        Layout.fillWidth: true
                    }
                    Text {
                        property double err: steerConfigWindow.steerAngleActualRounded - steerConfigWindow.steerAngleSetRounded
                        id: errorlbl
                        Layout.fillWidth: true
                        onErrChanged: err > 0 ? errorlbl.color = "red" : errorlbl.color = "darkgreen"
                        text: qsTr("Err: " + Math.round(err*10)/10)
                    }
                    // Item { Layout.fillWidth: true }
                    IconButtonTransparent{
                        //show angle info window
                        anchors.right: angleInfo.right
                        icon.source: prefix + "/images/ArrowRight.png"
                        implicitHeight: parent.height
                        implicitWidth: parent.width /4 - 5 * theme.scaleWidth
                        onClicked: steerConfigSettings.open()
                    }
                }
            }
        }
        Rectangle{
            id: pwmWindow
            color: aogInterface.backgroundColor
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8 * theme.scaleHeight
            anchors.top: steerSlidersConfig.bottom
            anchors.topMargin: 8 * theme.scaleHeight
            visible: false
            height: parent.height *0.25
            width: steerConfigFirst.width-2 * theme.scaleWidth
            anchors.horizontalCenter: parent.horizontalCenter

            RowLayout{
                id: pwmRow
                anchors.bottomMargin: 10 * theme.scaleHeight
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 10 * theme.scaleHeight
                height: 50 * theme.scaleHeight
                width: parent.width - 2 * theme.scaleWidth
                anchors.horizontalCenter: parent.horizontalCenter

                IconButton{
                    id: btnFreeDrive
                    border: 2
                    color3: "white"
                    icon.source: prefix + "/images/SteerDriveOff.png"
                    iconChecked: prefix + "/images/SteerDriveOn.png"
                    implicitHeight: parent.height
                    implicitWidth:  parent.width /4 - 4
                    isChecked: false
                    checkable: true
                    onClicked: {
                        VehicleInterface.isInFreeDriveMode = ! VehicleInterface.isInFreeDriveMode;
                        VehicleInterface.driveFreeSteerAngle = 0;
                    }
                }
                IconButton{
                    id: btnSteerAngleDown
                    border: 2
                    color3: "white"
                    icon.source: prefix + "/images/SnapLeft.png"
                    implicitHeight: parent.height
                    implicitWidth:  parent.width /4 - 5 * theme.scaleWidth
                    onClicked: {
                        if ( --VehicleInterface.driveFreeSteerAngle < -40)
                            VehicleInterface.driveFreeSteerAngle = -40;
                    }
                    enabled: btnFreeDrive.checked
                }
                IconButton{
                    id: btnSteerAngleUp
                    border: 2
                    color3: "white"
                    icon.source: prefix + "/images/SnapRight.png"
                    implicitHeight: parent.height
                    implicitWidth:  parent.width /4 - 5 * theme.scaleWidth
                    onClicked: {
                        if ( ++VehicleInterface.driveFreeSteerAngle > 40)
                            VehicleInterface.driveFreeSteerAngle = 40;
                    }
                    enabled: btnFreeDrive.checked
                }
                IconButton{
                    id: btnFreeDriveZero
                    border: 2
                    color3: "white"
                    icon.source: prefix + "/images/SteerZeroSmall.png"
                    implicitHeight: parent.height
                    implicitWidth:  parent.width /4 - 5 * theme.scaleWidth
                    onClicked: {
                        if (VehicleInterface.driveFreeSteerAngle === 0) {
                            VehicleInterface.driveFreeSteerAngle = 5;
                        } else {
                            VehicleInterface.driveFreeSteerAngle = 0;
                        }
                    }
                }
            }
            Text{
                anchors.left: pwmRow.left
                anchors.top: pwmRow.bottom
                text: qsTr("PWM: %1").arg(ModuleComm.pwmDisplay)
            }
            Text{
                anchors.right: pwmRow.right
                anchors.rightMargin: 50 * theme.scaleWidth
                anchors.top: pwmRow.bottom
                font.pixelSize: 15
                text: qsTr("0r +5")
            }
            IconButton{
                id: btnStartSA
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                border: 2
                color3: "white"
                height: 75 * theme.scaleHeight
                icon.source: prefix + "/images/BoundaryRecord.png"
                iconChecked: prefix + "/images/Stop.png"
                isChecked: SteerConfig.isSA
                checkable: true
                width: 75 * theme.scaleWidth
                onClicked: {
                    if (checked) SteerConfig.startSA();
                    else SteerConfig.stopSA();
                }
            }
            Text{
                anchors.top: btnStartSA.top
                anchors.left: btnStartSA.right
                anchors.leftMargin: 5 * theme.scaleWidth
                text: SteerConfig.isSA ? qsTr("Drive Steady") :
                                         qsTr("Steer Angle: %1°").arg(SteerConfig.calcSteerAngleInner.toLocaleString(Qt.locale(), 'f', 1))
                Layout.alignment: Qt.AlignCenter
            }
            Text{
                anchors.bottom: btnStartSA.bottom
                anchors.left: btnStartSA.right
                anchors.leftMargin: 5 * theme.scaleWidth
                text: qsTr("Diameter: %1").arg(SteerConfig.diameter.toLocaleString(Qt.locale(), 'f', 1))
                Layout.alignment: Qt.AlignCenter
            }
        }
    }

    Timer {
        id: sendUdptimer
        interval: 1000;
        onTriggered: ModuleComm.modulesSend252() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
    }
}
