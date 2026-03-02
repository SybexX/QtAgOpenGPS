// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// GPS heading source (dual/single)
import QtQuick
import QtQuick.Controls.Fusion
//import Settings

import ".."
import "../components"

Rectangle{
    id: configSources
    anchors.fill: parent
    visible: true
    color: aogInterface.backgroundColor
    TitleFrame {
        id:antennaType
        //width: 360 * theme.scaleWidth
        height:150 * theme.scaleHeight
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.horizontalCenter
        anchors.topMargin: 5 * theme.scaleHeight
        anchors.leftMargin: 5 * theme.scaleWidth
        anchors.rightMargin: 5 * theme.scaleWidth
        anchors.bottomMargin: 5 * theme.scaleHeight
        color: aogInterface.backgroundColor
        border.color: aogInterface.blackDayWhiteNight
        visible: true
        title: qsTr("Antenna Type", "GPS Antenna type, fixed or dual")
        font.pointSize: 16

        Row{
            id: sourceRow
            anchors.top: parent.top
            anchors.topMargin: 10 * theme.scaleHeight
            anchors.leftMargin: 10 * theme.scaleWidth
            anchors.rightMargin: 10 * theme.scaleWidth
            anchors.bottomMargin: 10 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            height: childrenRect.height  + 25 * (theme.scaleHeight)
            spacing: 10 * theme.scaleWidth

            ButtonGroup {
                buttons: [ dualBtn, fixBtn ]
            }

            IconButtonColor{
                id: dualBtn
                width:150 * theme.scaleWidth
                height:100 * theme.scaleHeight
                text: qsTr("Dual")
                checkable: true
                icon.source: prefix + "/images/Config/Con_SourcesGPSDual.png"

                // Threading Phase 1: GPS heading source configuration for dual
                property string headingSource: "GPS"

                isChecked: (SettingsManager.gps_headingFromWhichSource === "Dual" ? true: false)

                onCheckedChanged: {
                    if (checked){
                        SettingsManager.gps_headingFromWhichSource = "Dual"
                        if(SettingsManager.vehicle_antennaOffset < 0)
                            timedMessage.addMessage(7000, qsTr("Antenna Offset error!"), qsTr('You have antenna offset set to "left". Dual requires it set to "right". Change it or you will have offset errors'))
                    }
                }

                onHeadingSourceChanged: {
                    if(headingSource === "Dual"){
                        dualBtn.checked = true
                    }else{
                        dualBtn.checked = false
                    }
                }

            }
            IconButtonColor{
                id: fixBtn
                width:150 * theme.scaleWidth
                height:100 * theme.scaleHeight
                text: qsTr("Fix")
                checkable: true
                icon.source: prefix + "/images/Config/Con_SourcesGPSSingle.png"

                // Threading Phase 1: GPS heading source configuration for fix
                property string headingSource: "GPS"

                isChecked: (SettingsManager.gps_headingFromWhichSource === "Fix" ? true : false)

                onCheckedChanged: {
                    if(checked)
                        SettingsManager.gps_headingFromWhichSource = "Fix"
                }

                onHeadingSourceChanged: {
                    if(headingSource === "Fix"){
                        fixBtn.checked = true
                    }else{
                        fixBtn.checked = false
                    }
                }
            }
        }
    }

    TitleFrame {
        id:rtkAlarm
        width: parent.width /2
        height: rtkAlarmRow.height + titleHeight + (10 * theme.scaleHeight)
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: 5 * theme.scaleHeight
        anchors.leftMargin: 5 * theme.scaleWidth
        anchors.rightMargin: 5 * theme.scaleWidth
        anchors.bottomMargin: 5 * theme.scaleHeight
        color: aogInterface.backgroundColor
        visible: true
        title: qsTr("RTK Alarm")
        font.pointSize: 16

        Row{
            id: rtkAlarmRow
            anchors.top: parent.top
            anchors.topMargin: 10 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10 * theme.scaleWidth
            height: childrenRect.height + (10 * theme.scaleHeight)

            Component.onCompleted: console.debug(height)

            IconButtonColor{
                width:150 * theme.scaleWidth
                height:100 * theme.scaleHeight
                id: alarm
                icon.source: prefix + "/images/Config/Con_SourcesRTKAlarm.png"
                // Threading Phase 1: RTK GPS status
                isChecked: SettingsManager.gps_isRTK
                onClicked: SettingsManager.gps_isRTK = true
            }
            Rectangle {
                height: 100 * theme.scaleHeight
                width: childrenRect.width

                Text{
                    anchors.verticalCenter: parent.verticalCenter
                    text: "  ->  "
                }
            }
            IconButtonColor{
                width:150 * theme.scaleWidth
                height:100 * theme.scaleHeight
                checkable: true
                id: killAutoSteer
                icon.source: prefix + "/images/AutoSteerOff.png"
                // Threading Phase 1: RTK kill autosteer setting
                isChecked: SettingsManager.gps_isRTKKillAutoSteer
                onClicked: SettingsManager.gps_isRTKKillAutoSteer = true
            }
        }
    }
    TitleFrame{
        id: singleAntennaSettings
        enabled: fixBtn.checked
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5 * theme.scaleHeight
        anchors.leftMargin: 5 * theme.scaleWidth
        anchors.rightMargin: 5 * theme.scaleWidth
        anchors.bottomMargin: 5 * theme.scaleHeight
        //onEnabledChanged: visible = enabled
        visible: true

        border.color: enabled ? aogInterface.blackDayWhiteNight : "grey"

        color: aogInterface.backgroundColor

        title: qsTr("Single Antenna Settings")
        font.pointSize: 16

        Row {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 10 * theme.scaleHeight
            anchors.leftMargin: 10 * theme.scaleWidth

            id: minGPSStep
            Label {
                color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
                text: qsTr("Minimum GPS Step")
                width: singleAntennaSettings.width * 0.5
            }
            ButtonColor{
                id: minGPSStepBtn
                width: 180 * theme.scaleWidth
                height: 50 * theme.scaleHeight
                checkable: true
                isChecked: SettingsManager.f_minHeadingStepDistance === 1
                color: "light blue"

                onCheckedChanged: {
                    if(checked){
                        // Threading Phase 1: GPS minimum step settings
                        SettingsManager.gps_minimumStepLimit = 0.1
                        SettingsManager.f_minHeadingStepDistance = 1
                    }else{
                        SettingsManager.gps_minimumStepLimit = 0.05
                        SettingsManager.f_minHeadingStepDistance = 0.5
                    }
                }
                text: SettingsManager.gps_minimumStepLimit * 100 + " " + qsTr("cm", "centimeter abbreviation")
            }
        }
        Row {
            id: headingDistance
            anchors.top: minGPSStep.bottom
            anchors.left: parent.left
            anchors.topMargin: 10 * theme.scaleHeight
            anchors.leftMargin: 10 * theme.scaleWidth

            Label {
                color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
                text: qsTr("Heading Distance")
                width: singleAntennaSettings.width * 0.5
            }
            Label {
                id: headingDistanceText
                width: minGPSStepBtn.width
                color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
                text: SettingsManager.f_minHeadingStepDistance * 100 + " " + qsTr("cm", "centimeter abbreviation")
            }
        }

        Rectangle{//fusion adjuster
            id: fusionRow
            anchors.top: headingDistance.bottom
            anchors.topMargin: 20 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            width: 360 * theme.scaleWidth
            height: 100 * theme.scaleHeight
            border.color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            color: aogInterface.backgroundColor
            SliderCustomized{
                leftText: 100 - value
                rightText: value
                leftTopText: qsTr("IMU <")
                colorLeftTopText: "green"
                anchors.centerIn: parent
                rightTopText: qsTr("> GPS")
                colorRightTopText: "red"
                centerTopText: qsTr("Fusion")
                from: 20
                to: 40

                // Threading Phase 1: IMU fusion weight configuration
                property int fusionWeight: 0

                onFusionWeightChanged: {
                    value = fusionWeight * 100
                }

                value: SettingsManager.imu_fusionWeight2 * 100
                onValueChanged: {
                    SettingsManager.imu_fusionWeight2 = value / 100
                }
                stepSize: 1
            }
        }

        Text{
            anchors.left: fusionRow.left
            anchors.top: fusionRow.bottom
            anchors.topMargin: 15 * theme.scaleHeight
            color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            text: qsTr("Default: 70%")
        }

        Rectangle{
            // PHASE 6.0.35: Forward compensation slider (wheel angle → heading correction)
            // Used to compensate GPS heading based on actual wheel angle during movement
            id: forwardCompRow
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: fusionRow.bottom
            anchors.topMargin: 50 * theme.scaleHeight
            width: 360 * theme.scaleWidth
            height: 100 * theme.scaleHeight
            border.color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            color: aogInterface.backgroundColor
            SliderCustomized{
                leftText: (value * 100).toFixed(0) + "%"
                centerTopText: qsTr("Forward Comp")
                anchors.centerIn: parent
                from: 0.05
                to: 0.25
                value: SettingsManager.gps_forwardComp
                onValueChanged: {
                    SettingsManager.gps_forwardComp = value
                }
                stepSize: 0.01
            }
        }

        Text{
            anchors.left: forwardCompRow.left
            anchors.top: forwardCompRow.bottom
            anchors.topMargin: 5 * theme.scaleHeight
            color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            text: qsTr("Default: 15%")
        }

        Rectangle{
            // PHASE 6.0.35: Reverse compensation slider (wheel angle → heading correction in reverse)
            // Higher value than forward due to different geometry when reversing
            id: reverseCompRow
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: forwardCompRow.bottom
            anchors.topMargin: 30 * theme.scaleHeight
            width: 360 * theme.scaleWidth
            height: 100 * theme.scaleHeight
            border.color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            color: aogInterface.backgroundColor
            SliderCustomized{
                leftText: (value * 100).toFixed(0) + "%"
                centerTopText: qsTr("Reverse Comp")
                anchors.centerIn: parent
                from: 0.15
                to: 0.45
                value: SettingsManager.gps_reverseComp
                onValueChanged: {
                    SettingsManager.gps_reverseComp = value
                }
                stepSize: 0.01
            }
        }

        Text{
            anchors.left: reverseCompRow.left
            anchors.top: reverseCompRow.bottom
            anchors.topMargin: 5 * theme.scaleHeight
            color: fixBtn.checked ? aogInterface.blackDayWhiteNight : "grey"
            text: qsTr("Default: 30%")
        }

        ButtonColor{
            id: reverseDet
            text: qsTr("Reverse Detection")
            checkable: true
            anchors.top: reverseCompRow.bottom
            anchors.topMargin: 30 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            color: aogInterface.backgroundColor
            colorChecked: "green"
            width: 250*theme.scaleWidth
            isChecked: SettingsManager.imu_isReverseOn
        }
    }
    TitleFrame {
        id: dualAntennaSettings
        anchors.bottom: rtkAlarm.top
        anchors.right: parent.horizontalCenter
        anchors.left: parent.left
        anchors.top: antennaType.bottom
        anchors.topMargin: 5 * theme.scaleHeight
        anchors.leftMargin: 5 * theme.scaleWidth
        anchors.rightMargin: 5 * theme.scaleWidth
        anchors.bottomMargin: 5 * theme.scaleHeight
        border.color: aogInterface.blackDayWhiteNight
        visible: true
        //onEnabledChanged: visible = enabled
        color: aogInterface.backgroundColor
        title: qsTr("Dual Antenna Settings")
        font.pointSize: 16
        enabled: dualBtn.checked

        Image {
            id: head
            source: prefix + "/images/Config/Con_SourcesHead.png"
            width: 100 * theme.scaleWidth
            height: 100 * theme.scaleHeight
            anchors.top: parent.top
            anchors.topMargin: 40 * theme.scaleHeight
            anchors.left: parent.left
            anchors.leftMargin: 30 * theme.scaleWidth
        }
        SpinBoxOneDecimal{
            id: headingOffSet
            anchors.left: head.right
            anchors.verticalCenter: head.verticalCenter
            anchors.leftMargin: 10 * theme.scaleWidth
            decimals: 1
            from: -1800
            to: 1800
            // Threading Phase 1: Dual antenna heading offset
            boundValue: SettingsManager.gps_dualHeadingOffset
            onValueChanged: SettingsManager.gps_dualHeadingOffset = value
            editable: true
            text: qsTr("Heading Offset (Degrees)")
        }
    }
}
