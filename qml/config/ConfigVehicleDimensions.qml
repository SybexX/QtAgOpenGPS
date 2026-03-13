// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Vehicle dimensions
import QtQuick
import QtQuick.Controls.Fusion
//import Settings

import ".."
import "../components"
/*todo:
  switch to something with configImplementDimensions
  */

Rectangle{
    id: configTractorDimensions
    objectName: "configVehicleDimensions"
    anchors.fill: parent
    color: aogInterface.backgroundColor

    visible: false

    // Qt 6.8 QProperty + BINDABLE: Direct binding to SettingsManager (no local properties needed)
    Image {
        id: dimImage
        // Threading Phase 1: Vehicle type dependent image selection (direct SettingsManager binding)
        source: SettingsManager.vehicle_vehicleType === 0 ? prefix + "/images/RadiusWheelBase.png":
                SettingsManager.vehicle_vehicleType === 1 ? prefix + "/images/RadiusWheelBaseHarvester.png" :
                SettingsManager.vehicle_vehicleType === 2 ? prefix + "/images/RadiusWheelBase4WD.png":
                prefix + "/images/Config/ConSt_Mandatory.png"
        anchors.fill: parent
        anchors.margins: 15
    }
    SpinBoxCM{
        id: wheelbase
        anchors.bottom: dimImage.bottom
        anchors.right: dimImage.right
        anchors.rightMargin: dimImage.width * .65
        anchors.bottomMargin: dimImage.height *.15
        from: 20
        to: 787
        // Threading Phase 1: Vehicle wheelbase dimension (direct SettingsManager binding)
        boundValue: SettingsManager.vehicle_wheelbase
        onValueModified: SettingsManager.vehicle_wheelbase = value
        text: qsTr("Wheelbase")
    }
    SpinBoxCM{
        id: trackWidth
        anchors.top: dimImage.top
        anchors.topMargin: dimImage.height *.25
        anchors.right: dimImage.right
        from: 50
        to: 9999
        // Threading Phase 1: Vehicle track width dimension (direct SettingsManager binding)
        boundValue: SettingsManager.vehicle_trackWidth
        onValueModified: SettingsManager.vehicle_trackWidth = value
        text: qsTr("Track")
    }
    SpinBoxCM{
        id: minTurningRadius
        anchors.bottom: dimImage.bottom
        anchors.bottomMargin: dimImage.height *.18
        anchors.right: dimImage.right
        from: 50
        to: 9999
        // Threading Phase 1: Vehicle minimum turning radius (direct SettingsManager binding)
        boundValue: SettingsManager.vehicle_minTurningRadius
        onValueModified: SettingsManager.vehicle_minTurningRadius = value
        text: qsTr("Turn Radius")
    }
    SpinBoxCM{
        id: hitchLength
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: dimImage.left
        from: 10
        to:3000
        // Threading Phase 1: Vehicle hitch length (absolute value, direct SettingsManager binding)
        boundValue: SettingsManager.vehicle_hitchLength < 0 ? -SettingsManager.vehicle_hitchLength : SettingsManager.vehicle_hitchLength
        onValueModified: SettingsManager.vehicle_hitchLength = -value
        // Threading Phase 1: Hitch visibility for trailing tools (direct SettingsManager binding)
        visible: SettingsManager.tool_isToolTrailing
    }
}
