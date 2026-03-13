// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// IMU settings (reset, offset, zero, etc
import QtQuick
import QtQuick.Controls.Fusion
//import Settings
import AOG

import ".."
import "../components"

/*todo:
  not sure how to handle "remove offset, zero roll, and reset IMU". Will leave for now
  */
Rectangle{
    id: configSourcesRoll
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false
    IconButtonColor{
        objectName: "btnRemoveOffset"
        anchors.top: parent.top
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.left: parent.left
        anchors.leftMargin: 20 * theme.scaleWidth
        text: qsTr("Remove Offset")
        icon.source: prefix + "/images/Config/ConDa_RemoveOffset.png"
        onClicked: {
            // Threading Phase 1: Remove IMU roll offset
            SettingsManager.imu_rollZero = 0
        }
    }
    IconButtonColor{
        id: zeroRollBtn
        text: qsTr("Zero Roll")
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 20 * theme.scaleWidth
        icon.source: prefix + "/images/Config/ConDa_RollSetZero.png"
        isChecked: false
        onClicked: {
            if (Backend.fixFrame.imuRollDegrees != 88888) {
                // Threading Phase 1: Calculate roll with current zero offset
                var roll = Backend.fixFrame.imuRollDegrees + SettingsManager.imu_rollZero
                SettingsManager.imu_rollZero = roll;
                Backend.fixFrame.imuRoll = roll;
            }
        }
    }

    Label {
        id: rollZeroDisplay
        anchors.left: zeroRollBtn.right
        anchors.verticalCenter: zeroRollBtn.verticalCenter
        anchors.leftMargin: 20 * theme.scaleWidth
        // Threading Phase 1: Display current roll zero value
        text: Number(SettingsManager.imu_rollZero).toLocaleString(Qt.locale(), 'f', 2);
    }

    IconButtonTransparent {
        id: rollOffsetUpBtn
        anchors.left: rollZeroDisplay.right
        anchors.verticalCenter: zeroRollBtn.verticalCenter
        anchors.leftMargin: 20 * theme.scaleWidth

        icon.source: prefix + "/images/UpArrow64.png"
        // Threading Phase 1: Increment roll zero offset
        onClicked: SettingsManager.imu_rollZero = SettingsManager.imu_rollZero + 0.1
    }

    IconButtonTransparent {
        id: rollOffsetDownBtn
        anchors.left: rollOffsetUpBtn.right
        anchors.verticalCenter: zeroRollBtn.verticalCenter
        anchors.leftMargin: 5 * theme.scaleWidth

        icon.source: prefix + "/images/DnArrow64.png"
        // Threading Phase 1: Decrement roll zero offset
        onClicked: SettingsManager.imu_rollZero = SettingsManager.imu_rollZero - 0.1
    }

    IconButtonColor{
        objectName: "btnResetIMU"
        anchors.left: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/Config/ConDa_ResetIMU.png"
        isChecked: false
        onClicked: {
            Backend.fixFrame.imuHeading = 88888;
            Backend.fixFrame.imuRoll = 99999;
        }
    }

    IconButtonColor{
        objectName: "btnInvertRoll"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.horizontalCenter
        anchors.topMargin: 20 * theme.scaleHeight
        text: qsTr("Invert Roll")
        icon.source: prefix + "/images/Config/ConDa_InvertRoll.png"
        checkable: true
        // Threading Phase 1: Invert roll setting
        checked: SettingsManager.imu_invertRoll
        onCheckedChanged: SettingsManager.imu_invertRoll = checked
    }
    Rectangle{
        id: rollFilterSlider
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: 50 * theme.scaleHeight
        anchors.leftMargin: 50 * theme.scaleWidth
        width: 270 * theme.scaleWidth
        height: 50 * theme.scaleHeight
        //color: "lightgray"
        SliderCustomized{
            objectName: "rollFilterSlider"
            anchors.fill: parent
            from: 0
            to: 98
            // Threading Phase 1: Roll filter value binding
            property double boundValue: 0.0
            value: SettingsManager.imu_rollFilter * 100
            onValueChanged: SettingsManager.imu_rollFilter = value / 100.0
            leftTopText: qsTr("Less")
            centerTopText: qsTr("Roll Filter")
            rightTopText: qsTr("More")
        }
    }
    Image {
        source: prefix + "/images/Config/ConD_RollHelper.png"
        anchors.right: parent.right
        anchors.rightMargin: 50 * theme.scaleWidth
        width: 150 * theme.scaleWidth
        height: 200 * theme.scaleHeight
        anchors.verticalCenter: parent.verticalCenter
    }
}
