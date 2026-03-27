import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AOG
import "components" as Comp

Dialog {
    id: allSettings
    x: 0
    y: 0
    width: mainWindow.width
    height: mainWindow.height
    modal: true
    title: qsTr("All Settings")

    function open() {
        allSettings.visible = true
    }

    function close() {
        allSettings.visible = false
    }

    Rectangle{
        id: configMain
        color: aogInterface.backgroundColor
        visible: true
        anchors.fill: parent

        RowLayout {
            width: parent.width
            spacing: 5

            ColumnLayout {
                width: parent.width / 4 - 20
                spacing: 2

                Comp.SectionHeader { text: qsTr("Steering") }

                Comp.SettingRow { name: qsTr("Max Steer"); value: SettingsManager.vehicle_maxSteerAngle }
                Comp.SettingRow { name: qsTr("Counts/Deg"); value: SettingsManager.as_countsPerDegree }
                Comp.SettingRow { name: qsTr("Ackerman"); value: SettingsManager.as_ackerman }
                Comp.SettingRow { name: qsTr("WAS Offset"); value: SettingsManager.as_wasOffset }
                Comp.SettingRow { name: qsTr("High PWM"); value: SettingsManager.as_highSteerPWM }
                Comp.SettingRow { name: qsTr("Low PWM"); value: SettingsManager.as_lowSteerPWM }
                Comp.SettingRow { name: qsTr("Min PWM"); value: SettingsManager.as_minSteerPWM }
                Comp.SettingRow { name: qsTr("Kp"); value: SettingsManager.as_Kp }
                Comp.SettingRow { name: qsTr("Side Hill"); value: SettingsManager.as_sideHillCompensation }
                Comp.SettingRow { name: qsTr("Snap Dist"); value: SettingsManager.as_snapDistance }
                Comp.SettingRow { name: qsTr("Snap Ref"); value: SettingsManager.as_snapDistanceRef }
                Comp.SettingRow { name: qsTr("Steer Reverse"); value: SettingsManager.as_isSteerInReverse }
                Comp.SettingRow { name: qsTr("DeadZone Dly"); value: SettingsManager.as_deadZoneDelay }
                Comp.SettingRow { name: qsTr("DeadZone Hdg"); value: SettingsManager.as_deadZoneHeading }

                Comp.SectionHeader { text: qsTr("Algorithm") }

                Comp.SettingRow { name: qsTr("Stanley Hdg"); value: SettingsManager.vehicle_stanleyHeadingErrorGain; decimals: 2 }
                Comp.SettingRow { name: qsTr("Stanley Dist"); value: SettingsManager.vehicle_stanleyDistanceErrorGain; decimals: 2 }
                Comp.SettingRow { name: qsTr("Stanley Int"); value: SettingsManager.vehicle_stanleyIntegralGainAB; decimals: 2 }
                Comp.SettingRow { name: qsTr("PP Integral"); value: SettingsManager.vehicle_purePursuitIntegralGainAB; decimals: 2 }
                Comp.SettingRow { name: qsTr("UTurn Radius"); value: SettingsManager.youturn_radius; decimals: 2 }


            }

            // Column 2: Algorithm + IMU + Display
            ColumnLayout {
                width: parent.width / 5 - 20
                spacing: 2



                Comp.SectionHeader { text: qsTr("IMU") }

                Comp.SettingRow { name: qsTr("Roll Zero"); value: SettingsManager.imu_rollZero; decimals: 1 }
                Comp.SettingRow { name: qsTr("Invert Roll"); value: SettingsManager.imu_invertRoll }
                Comp.SettingRow { name: qsTr("Dual IMU"); value: SettingsManager.imu_isDualAsIMU }
                Comp.SettingRow { name: qsTr("Reverse On"); value: SettingsManager.imu_isReverseOn }
                Comp.SettingRow { name: qsTr("Roll Filter"); value: SettingsManager.imu_rollFilter; decimals: 2 }
                Comp.SettingRow { name: qsTr("Fusion W2"); value: SettingsManager.imu_fusionWeight2; decimals: 2 }

                Comp.SectionHeader { text: qsTr("Display") }

                Comp.SettingRow { name: qsTr("AutoStart AgIO"); value: SettingsManager.display_isAutoStartAgIO }
                Comp.SettingRow { name: qsTr("AutoOff AgIO"); value: SettingsManager.display_isAutoOffAgIO }

                Comp.SectionHeader { text: qsTr("Vehicle Live") }

                Comp.LiveDataRow { name: qsTr("Tool On"); value: SettingsManager.vehicle_toolLookAheadOn }
                Comp.LiveDataRow { name: qsTr("Tool Off"); value: SettingsManager.vehicle_toolLookAheadOff }
                Comp.LiveDataRow { name: qsTr("Tool Delay"); value: SettingsManager.vehicle_toolOffDelay }
                Comp.LiveDataRow { name: qsTr("Pivot Behind"); value: SettingsManager.vehicle_isPivotBehindAntenna }
                Comp.LiveDataRow { name: qsTr("Steer Ahead"); value: SettingsManager.vehicle_isSteerAxleAhead }
                Comp.LiveDataRow { name: qsTr("Stanley"); value: SettingsManager.vehicle_isStanleyUsed }
                Comp.LiveDataRow { name: qsTr("Veh Type"); value: SettingsManager.vehicle_vehicleType }
                Comp.LiveDataRow { name: qsTr("Num Sec"); value: SettingsManager.vehicle_numSections }
                Comp.LiveDataRow { name: qsTr("Tank Hitch"); value: SettingsManager.vehicle_tankTrailingHitchLength }


            }

            // Column 3: GPS + Tool
            ColumnLayout {
                width: parent.width / 5 - 20
                spacing: 2

                Comp.SectionHeader { text: qsTr("GPS") }

                Comp.SettingRow { name: qsTr("Age Alarm"); value: SettingsManager.gps_ageAlarm }
                Comp.SettingRow { name: qsTr("Dual Hdg Ofs"); value: SettingsManager.gps_dualHeadingOffset; decimals: 1 }
                Comp.SettingRow { name: qsTr("Dual Rev Dist"); value: SettingsManager.gps_dualReverseDetectionDistance; decimals: 2 }
                Comp.SettingRow { name: qsTr("Hdg Source"); value: SettingsManager.gps_headingFromWhichSource }
                Comp.SettingRow { name: qsTr("Is RTK"); value: SettingsManager.gps_isRTK }
                Comp.SettingRow { name: qsTr("RTK Kill"); value: SettingsManager.gps_isRTKKillAutoSteer }
                Comp.SettingRow { name: qsTr("Min Step"); value: SettingsManager.gps_minimumStepLimit }

                Comp.SectionHeader { text: qsTr("Tool") }

                Comp.SettingRow { name: qsTr("Sec Off Out"); value: SettingsManager.tool_isSectionOffWhenOut }
                Comp.SettingRow { name: qsTr("Sec Not Zone"); value: SettingsManager.tool_isSectionsNotZones }
                Comp.SettingRow { name: qsTr("Tool Front"); value: SettingsManager.tool_isToolFront }
                Comp.SettingRow { name: qsTr("Tool Rear"); value: SettingsManager.tool_isToolRearFixed }
                Comp.SettingRow { name: qsTr("Tool TBT"); value: SettingsManager.tool_isTBT }
                Comp.SettingRow { name: qsTr("Tool Trail"); value: SettingsManager.tool_isToolTrailing }
                Comp.SettingRow { name: qsTr("Trail Hitch"); value: SettingsManager.tool_toolTrailingHitchLength; decimals: 2 }
                Comp.SettingRow { name: qsTr("Trail Pivot"); value: SettingsManager.tool_trailingToolToPivotLength; decimals: 2 }
                Comp.SettingRow { name: qsTr("Hyd Lift LA"); value: SettingsManager.vehicle_hydraulicLiftLookAhead; decimals: 2 }

                Comp.SectionHeader { text: qsTr("Headland") }

                Comp.SettingRow { name: qsTr("Sec Control"); value: SettingsManager.headland_isSectionControlled }
                Comp.SettingRow { name: qsTr("Sec Fast"); value: SettingsManager.section_isFast }


            }

            // Column 4: Live GPS Data
            ColumnLayout {
                width: parent.width / 5 - 20
                spacing: 2

                Comp.SectionHeader { text: qsTr("Live GPS") }

                Comp.LiveDataRow { name: qsTr("Frame Time"); value: Backend.fixFrame.frameTime; format: "N1" }
                Comp.LiveDataRow { name: qsTr("Time Slice"); value: 1.0 / Backend.fixFrame.timeSlice; format: "N3" }
                Comp.LiveDataRow { name: qsTr("GPS Hz"); value: Backend.fixFrame.hz; format: "N1" }
                Comp.LiveDataRow { name: qsTr("Easting"); value: Backend.fixFrame.easting; format: "N2" }
                Comp.LiveDataRow { name: qsTr("Northing"); value: Backend.fixFrame.northing; format: "N2" }
                Comp.LiveDataRow { name: qsTr("Sats"); value: Backend.fixFrame.satellitesTracked }
                Comp.LiveDataRow { name: qsTr("HDOP"); value: Backend.fixFrame.hdop; format: "N2" }
                Comp.LiveDataRow { name: qsTr("Altitude"); value: Backend.fixFrame.altitude; format: "N2" }
                Comp.LiveDataRow { name: qsTr("Fix Qual"); value: Backend.fixFrame.fixQuality }
                Comp.LiveDataRow { name: qsTr("Dropped"); value: Backend.fixFrame.droppedSentences }
                Comp.LiveDataRow { name: qsTr("GPS Hdg"); value: Backend.fixFrame.gpsHeading * 180.0 / Math.PI; format: "N1" }
                Comp.LiveDataRow { name: qsTr("IMU Hdg"); value: Backend.fixFrame.imuHeading > 360 ? "#INV" : Backend.fixFrame.imuHeading; format: "N1" }
                Comp.LiveDataRow { name: qsTr("Veh Hdg"); value: VehicleInterface.fixHeading * 180.0 / Math.PI; format: "N1" }
                Comp.LiveDataRow { name: qsTr("Ang Vel"); value: Backend.fixFrame.yawRate; format: "N2" }

                Comp.SectionHeader { text: qsTr("Field") }

                Comp.SettingRow { name: qsTr("Min Hdg Step"); value: SettingsManager.f_minHeadingStepDistance; decimals: 3 }
                Comp.SettingRow { name: qsTr("Remote Work"); value: SettingsManager.f_isRemoteWorkSystemOn }
                Comp.SettingRow { name: qsTr("Steer Work"); value: SettingsManager.f_isSteerWorkSwitchEnabled }
                Comp.SettingRow { name: qsTr("Steer Sec"); value: SettingsManager.f_isSteerWorkSwitchManualSections }
                Comp.SettingRow { name: qsTr("Work Low"); value: SettingsManager.f_isWorkSwitchActiveLow }
                Comp.SettingRow { name: qsTr("Work On"); value: SettingsManager.f_isWorkSwitchEnabled }
                Comp.SettingRow { name: qsTr("Work Sec"); value: SettingsManager.f_isWorkSwitchManualSections }


            }

            ColumnLayout {
                width: parent.width / 5 - 20
                spacing: 2

                Comp.SectionHeader { text: qsTr("Vehicle") }

                Comp.SettingRow { name: qsTr("Wheelbase"); value: SettingsManager.vehicle_wheelbase; decimals: 2 }
                Comp.SettingRow { name: qsTr("Track Width"); value: SettingsManager.vehicle_trackWidth; decimals: 2 }
                Comp.SettingRow { name: qsTr("Ant Pivot"); value: SettingsManager.vehicle_antennaPivot; decimals: 2 }
                Comp.SettingRow { name: qsTr("Ant Height"); value: SettingsManager.vehicle_antennaHeight; decimals: 2 }
                Comp.SettingRow { name: qsTr("Ant Offset"); value: SettingsManager.vehicle_antennaOffset; decimals: 2 }
                Comp.SettingRow { name: qsTr("Panic Speed"); value: SettingsManager.vehicle_panicStopSpeed }
                Comp.SettingRow { name: qsTr("Goal Acquire"); value: SettingsManager.vehicle_goalPointAcquireFactor; decimals: 2 }
                Comp.SettingRow { name: qsTr("Goal LookAhead"); value: SettingsManager.vehicle_goalPointLookAheadHold }
                Comp.SettingRow { name: qsTr("Goal Mult"); value: SettingsManager.vehicle_goalPointLookAheadMult }
                Comp.SettingRow { name: qsTr("Max Ang Vel"); value: SettingsManager.vehicle_maxAngularVelocity }
                Comp.SettingRow { name: qsTr("Slow Speed"); value: SettingsManager.vehicle_slowSpeedCutoff }
                Comp.SettingRow { name: qsTr("Tool Width"); value: SettingsManager.vehicle_toolWidth; decimals: 2 }
                Comp.SettingRow { name: qsTr("Tool Offset"); value: SettingsManager.vehicle_toolOffset; decimals: 2 }
                Comp.SettingRow { name: qsTr("Tool Overlap"); value: SettingsManager.vehicle_toolOverlap; decimals: 2 }
                Comp.SettingRow { name: qsTr("Hitch Len"); value: SettingsManager.vehicle_hitchLength; decimals: 2 }

            }

        }
        Row {
            id: bottomRow
            spacing: 10 * theme.scaleWidth
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: btnSave.height + 20 * theme.scaleHeight



            Comp.IconButtonTransparent{
                id: btnSave
                anchors.right: saveAndClose.left
                anchors.topMargin: 20 * theme.scaleHeight
                anchors.bottomMargin: 5 * theme.scaleHeight
                anchors.rightMargin: 20 * theme.scaleHeight
                anchors.leftMargin: 20 * theme.scaleHeight
                anchors.bottom: parent.bottom
                enabled: false
                icon.source: prefix + "/images/ToolAcceptChange.png"
                Text{
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.left
                    anchors.rightMargin: 5
                    text: qsTr("Send + Save")
                }
                onClicked: {
                    saveScreenshot()
                }
            }

            Comp.IconButtonTransparent{
                id: saveAndClose
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.topMargin: 20 * theme.scaleHeight
                anchors.bottomMargin: 5 * theme.scaleHeight
                anchors.rightMargin: 20 * theme.scaleHeight
                anchors.leftMargin: 20 * theme.scaleHeight
                icon.source: prefix + "/images/OK64.png"
                onClicked: {
                    allSettings.close()
                }
            }
        }
    }
}

