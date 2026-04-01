// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Settings in the expanded steer config. **Not the sliders**
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Dialogs
//import Settings
import AOG


import ".."
import "../components"


Dialog {
    id: steerConfig
    x: 0
    y: 0
    width: mainWindow.width
    height: mainWindow.height
    modal: true
    title: qsTr("Auto Steer Config")

    property bool notSaved: false

    onVisibleChanged: {
        if(visible) {
            settingsArea.load_settings()
            sensorsBtn.isChecked = true
            console.log("Settings loaded")
            notSaved = false
        }
    }

    Rectangle {
        anchors.fill: parent
        color: aogInterface.backgroundColor
    }

    ButtonGroup{
        buttons: settingsBtns.children
    }

    RowLayout{
        id: settingsBtns
        spacing: 3 * theme.scaleWidth
        width: parent.width
        anchors.top: parent.top
        anchors.topMargin: 20 * theme.scaleHeight
        SteerConfigTopButtons{
            id: sensorsBtn
            buttonText: qsTr("Sensors")
            icon.source: prefix + "/images/Config/ConD_Speedometer.png"
            implicitWidth: parent.width /5 -5
            //checked: true //because one has to be to start things off
            onClicked: settingsWindow.visible = false
        }
        SteerConfigTopButtons{
            id: configBtn
            implicitWidth: parent.width /5 -5
            buttonText: qsTr("Config")
            icon.source: prefix + "/images/Config/ConS_Pins.png"
            onClicked: settingsWindow.visible = false
        }
        SteerConfigTopButtons{
            id: settingsBtn
            implicitWidth: parent.width /5 -5
            buttonText: qsTr("Settings")
            icon.source: prefix + "/images/Config/ConS_ModulesSteer.png"
            onClicked: settingsWindow.show()
        }
        SteerConfigTopButtons{
            id: steerSafetyBtn
            implicitWidth: parent.width /5 -5
            buttonText: qsTr("Safety")
            icon.source: prefix + "/images/Config/ConS_Alarm.png"
            onClicked: settingsWindow.visible = false
        }
        SteerConfigTopButtons{
            id: steerSettingsBtn
            implicitWidth: parent.width /5 -5
            buttonText: qsTr("Steer Settings")
            icon.source: prefix + "/images/Config/ConS_ImplementConfig.png"
            onClicked: settingsWindow.visible = false
        }
    }
    Item{
        id: settingsArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: settingsBtns.bottom
        anchors.bottom: bottomRightButtons.top
        anchors.topMargin: 10 * theme.scaleHeight
        anchors.bottomMargin: 10 * theme.scaleHeight
        anchors.leftMargin: 10 * theme.scaleWidth
        anchors.rightMargin: 10 * theme.scaleWidth

        function load_settings(){
            if (visible) {
                var sett = SettingsManager.ardSteer_setting0

                if ((sett & 1) == 0) chkInvertWAS.checked = false;
                else chkInvertWAS.checked = true;

                if ((sett & 2) == 0) chkSteerInvertRelays.checked = false;
                else chkSteerInvertRelays.checked = true;

                if ((sett & 4) == 0) chkInvertSteer.checked = false;
                else chkInvertSteer.checked = true;

                //if ((sett & 8) == 0) cboxConv.currentText = "Differential";
                //else cboxConv.currentText = "Single";
                if ((sett & 8) == 0) cboxConv.currentIndex = 1;
                else cboxConv.currentIndex = 0;

                //if ((sett & 16) == 0) cboxMotorDrive.currentText = "IBT2";
                //else cboxMotorDrive.currentText = "Cytron";
                if ((sett & 16) == 0) cboxMotorDrive.currentIndex = 1;
                else cboxMotorDrive.currentIndex = 0;

                if ((sett & 32) == 32) cboxSteerEnable.currentIndex = 1;
                else if ((sett & 64) == 64) cboxSteerEnable.currentIndex = 2;
                else cboxSteerEnable.currentIndex = 0;

                if ((sett & 128) == 0){
                    console.log("encoder not checked")
                    cboxEncoder.checked = false;
                }
                else {
                    console.log("encoder set to checked")
                    cboxEncoder.checked = true;
                }

                sett = SettingsManager.ardSteer_setting1;

                if ((sett & 1) == 0) cboxDanfoss.checked = false;
                else cboxDanfoss.checked = true;

                //if ((sett & 8) == 0) cboxXY.Text = "X";
                //else cboxXY.Text = "Y";
                if ((sett & 8) == 0) cboxXY.currentIndex = 0;
                else cboxXY.currentIndex = 1;

                if ((sett & 2) == 0) cboxPressureSensor.checked = false;
                else cboxPressureSensor.checked = true;

                if ((sett & 4) == 0) cboxCurrentSensor.checked = false;
                else cboxCurrentSensor.checked = true;

                if (cboxCurrentSensor.checked || cboxPressureSensor.checked)
                {
                    hsbarSensor.value = SettingsManager.ardSteer_maxPulseCounts;
                }
                else
                {
                    nudMaxCounts.value = SettingsManager.ardSteer_maxPulseCounts;
                }

                /*the display logic that went here has been moved to the individual
                 *component files. They are not needed in QML
                */
            }
        }

        function save_settings() {
            var set = 1;
            var reset = 2046;
            var sett = 0;

            if (chkInvertWAS.checked) sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (chkSteerInvertRelays.checked) sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (chkInvertSteer.checked) sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxConv.currentText === "Single") sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxMotorDrive.currentText === "Cytron") sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxSteerEnable.currentText === "Switch") sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxSteerEnable.currentIndex === 2) sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxEncoder.checked){
                console.log("encoder checked")
                sett |= set;
            }
            else {
                console.log("encoder not checked")
                sett &= reset;
            }

            //set = (set << 1);
            //reset = (reset << 1);
            //reset = (reset + 1);
            //if ( ) sett |= set;
            //else sett &= reset;

            SettingsManager.ardSteer_setting0 = sett;
            SettingsManager.ardMac_isDanFoss = cboxDanfoss.checked;

            if (cboxCurrentSensor.checked || cboxPressureSensor.checked)
            {
                SettingsManager.ardSteer_maxPulseCounts = hsbarSensor.value;
            }
            else
            {
                SettingsManager.ardSteer_maxPulseCounts = nudMaxCounts.value;
            }

            // Settings1
            set = 1;
            reset = 2046;
            sett = 0;

            if (cboxDanfoss.checked) sett |= set;
            else sett &= reset;

            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxPressureSensor.checked) sett |= set;
            else sett &= reset;

            //bit 2
            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxCurrentSensor.checked) sett |= set;
            else sett &= reset;

            //bit 3
            set <<= 1;
            reset <<= 1;
            reset += 1;
            if (cboxXY.currentIndex === 1) sett |= set;
            else sett &= reset;

            SettingsManager.ardSteer_setting1 = sett;

            //Properties.Settings.Default.Save(); not sure what happens here?? David

            ModuleComm.modulesSend251()

            notSaved = false;
        }
        function reset_all() {
            timedMessage.addMessage(2000, "Reset To Default", "Values Set to Inital Default");
            //Settings.vehicle_maxSteerAngle = mf.vehicle_maxSteerAngle = 45; TODO
            SettingsManager.vehicle_maxSteerAngle = 45;
            SettingsManager.as_countsPerDegree = 110;

            SettingsManager.as_ackerman = 100;

            SettingsManager.as_wasOffset = 3;

            SettingsManager.as_highSteerPWM = 180;
            SettingsManager.as_Kp = 50;
            SettingsManager.as_minSteerPWM = 25;

            SettingsManager.ardSteer_setting0 = 56;
            SettingsManager.ardSteer_setting1 = 0;
            SettingsManager.ardMac_isDanFoss = false;

            SettingsManager.ardSteer_maxPulseCounts = 3;

            SettingsManager.vehicle_goalPointAcquireFactor = 0.85;
            SettingsManager.vehicle_goalPointLookAheadHold = 3;
            SettingsManager.vehicle_goalPointLookAheadMult = 1.5;

            SettingsManager.vehicle_stanleyHeadingErrorGain = 1;
            SettingsManager.vehicle_stanleyDistanceErrorGain = 1;
            SettingsManager.vehicle_stanleyIntegralGainAB = 0;

            SettingsManager.vehicle_purePursuitIntegralGainAB = 0;

            SettingsManager.as_sideHillCompensation = 0;

            SettingsManager.as_uTurnCompensation = 1;

            SettingsManager.imu_invertRoll = false;

            SettingsManager.imu_rollZero = 0;

            SettingsManager.as_minSteerSpeed = 0;
            SettingsManager.as_maxSteerSpeed = 15;
            SettingsManager.as_functionSpeedLimit = 12;
            SettingsManager.display_lightbarCmPerPixel = 5;
            SettingsManager.display_lineWidth = 2;
            SettingsManager.as_snapDistance = 20;
            SettingsManager.as_guidanceLookAheadTime = 1.5;
            SettingsManager.as_uTurnCompensation = 1;

            SettingsManager.vehicle_isStanleyUsed = false;
            //mf.isStanleyUsed = false; TODO

            SettingsManager.as_isSteerInReverse = false;
            //mf.isSteerInReverse = false; TODO

            //save current vehicle
            //RegistrySettings.Save();

            /*TODO mf.vehicle = new CVehicle(mf);

            FormSteer_Load(this, e);

            toSend = true; counter = 6;


            pboxSendSteer.Visible = true;

            tabControl1.SelectTab(1);
            tabControl1.SelectTab(0);
            tabSteerSettings.SelectTab(1);
            tabSteerSettings.SelectTab(0);
            */
        }

        //region sensorsTab
        Item {
            visible: sensorsBtn.checked
            id: sensorWindowItem
            anchors.fill: parent
            ButtonGroup{
                buttons: sensorsBtnsRow.children
            }

            RowLayout{
                id: sensorsBtnsRow
                width: parent.width
                IconButtonColor{
                    id: cboxEncoder
                    icon.source: prefix + "/images/Config/ConSt_TurnSensor.png"
                    checkable: true
                    buttonText: qsTr("Count Sensor")
                    Layout.alignment: Qt.AlignCenter
                    onClicked: notSaved = true
                    onCheckedChanged: nudMaxCounts.visible = checked
                }
                IconButtonColor{
                    id: cboxPressureSensor
                    icon.source: prefix + "/images/Config/ConSt_TurnSensorPressure.png"
                    checkable: true
                    buttonText: qsTr("Pressure Turn Sensor")
                    Layout.alignment: Qt.AlignCenter
                    onClicked: notSaved = true
                }
                IconButtonColor{
                    id: cboxCurrentSensor
                    icon.source: prefix + "/images/Config/ConSt_TurnSensorCurrent.png"
                    checkable: true
                    buttonText: qsTr("Current Turn Sensor")
                    Layout.alignment: Qt.AlignCenter
                    onClicked: notSaved = true
                }
            }
            Text{
                //id: label61
                anchors.top: sensorsDisplayColumn.top
                anchors.right: sensorsDisplayColumn.left
                anchors.rightMargin: 10
                text: qsTr("Off at %")
                font.bold: true
                visible: cboxPressureSensor.checked || cboxCurrentSensor.checked
            }

            Column {
                id: sensorsDisplayColumn
                anchors.top: sensorsBtnsRow.bottom
                anchors.topMargin: 30 * theme.scaleHeight
                anchors.horizontalCenter: sensorsBtnsRow.horizontalCenter
                anchors.bottom: parent.bottom
                spacing: 5 * theme.scaleHeight
                width: childrenRect.width
                SpinBoxCustomized {
                    id: nudMaxCounts
                    text: qsTr("Counts")
                    implicitWidth: 100 * theme.scaleWidth
                    implicitHeight: 65 * theme.scaleHeight
                    from: 0
                    value: 0
                    visible: false
                    to: 255
                    decimals: 0
                    editable: true
                    onValueChanged: notSaved = true
                }
                ProgressBar {
                    //id: pbarSensor
                    //just mirror width/height
                    width: 250 * theme.scaleWidth
                    height: 50 * theme.scaleWidth
                    visible: cboxPressureSensor.checked || cboxCurrentSensor.checked
                    value: ModuleComm.sensorData
                    from: 0
                    to: 255
                    Text {
                        //id: lblPercentFS
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.right
                        anchors.leftMargin: 5
                        text: (ModuleComm.sensorData < 0 ? "0" : ModuleComm.sensorData) + " %"
                        font.bold: true
                    }
                }
                SliderCustomized{
                    id: hsbarSensor
                    from: 0
                    to: 255
                    stepSize: 1
                    width: 250 * theme.scaleWidth
                    height: 50 * theme.scaleWidth
                    visible: cboxPressureSensor.checked || cboxCurrentSensor.checked
                    Text {
                        //id: lblhsbarSensor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.right
                        anchors.leftMargin: 5
                        text: Math.round((hsbarSensor.value * 0.3921568627) * 100) / 100  + " %"
                        font.bold: true
                    }
                    onValueChanged: notSaved = true
                }
            }
        }
        //endregion sensorsTab
        //
        //region configTab
        Item{
            anchors.fill: parent
            visible: configBtn.checked
            id: configWindow
            GridLayout{
                anchors.fill: parent
                rows: 4
                columns: 2
                flow: Grid.TopToBottom
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    IconButtonColor{
                        id: cboxDanfoss
                        anchors.centerIn: parent
                        icon.source: prefix + "/images/Config/ConST_Danfoss.png"
                        checkable: true
                        buttonText: qsTr("Danfoss")
                        Layout.alignment: Qt.AlignCenter
                        onClicked: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    IconButtonColor{
                        id: chkInvertWAS
                        anchors.centerIn: parent
                        icon.source: prefix + "/images/Config/ConSt_InvertWAS.png"
                        checkable: true
                        Layout.alignment: Qt.AlignCenter
                        buttonText: "Invert WAS"
                        onClicked: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    IconButtonColor{
                        id: chkInvertSteer
                        anchors.centerIn: parent
                        icon.source: prefix + "/images/Config/ConSt_InvertDirection.png"
                        checkable: true
                        buttonText: qsTr("Invert Motor Dir")
                        Layout.alignment: Qt.AlignCenter
                        onClicked: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    IconButtonColor{
                        id: chkSteerInvertRelays
                        anchors.centerIn: parent
                        icon.source: prefix + "/images/Config/ConSt_InvertRelay.png"
                        checkable: true
                        buttonText: qsTr("Invert Relays")
                        Layout.alignment: Qt.AlignCenter
                        onClicked: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ComboBoxCustomized {
                        id: cboxMotorDrive
                        anchors.centerIn: parent
                        editable: false
                        // Component.onCompleted: currentIndex = ((settings.setArdSteer_setting0 & 16) == 0) ? 1 : 0
                        model: ListModel {
                            id: modelmotorDriver
                            ListElement {text: "Cytron"}
                            ListElement {text: "IBT2"}
                        }
                        text: ("Motor Driver")
                        onActivated: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ComboBoxCustomized {
                        id: cboxConv
                        anchors.centerIn: parent
                        editable: false
                        // Component.onCompleted: currentIndex = ((settings.setArdSteer_setting0 & 8) == 0) ? 1 : 0
                        model: ListModel {
                            id: a2Dmodel
                            ListElement {text: qsTr("Single")}
                            ListElement {text: qsTr("Differential")}
                        }
                        text: qsTr("A2D Converter")
                        onActivated: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ComboBoxCustomized {
                        id: cboxXY
                        anchors.centerIn: parent
                        editable: false
                        // Component.onCompleted: currentIndex = ((settings.setArdSteer_setting1 & 8) == 0) ? 0 : 1
                        model: ListModel {
                            id: imuAxismodel
                            ListElement {text: "X"}
                            ListElement {text: "Y"}
                        }
                        text: qsTr("IMU X or Y Axis")
                        onActivated: notSaved = true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ComboBoxCustomized {
                        id: cboxSteerEnable
                        anchors.centerIn: parent
                        editable: false
                        /* Component.onCompleted: if((Settings.ardSteer_setting0 & 32) == 32)
                           currentIndex = 1
                           else if((Settings.ardSteer_setting0 & 64) == 64)
                           currentIndex = 2
                           else
                           currentIndex = 0*/
                        model: ListModel {
                            //   id: steerEnablemodel
                            ListElement {text: qsTr("None")}
                            ListElement {text: qsTr("Switch")}
                            ListElement {text: qsTr("Button")}
                        }
                        text: qsTr("Steer Enable")
                        onActivated: notSaved = true
                        Text{
                            anchors.top: cboxSteerEnable.bottom
                            anchors.left: cboxSteerEnable.left
                            text: qsTr("Button- Push On, Push Off\nSwitch- Pushed On, Release Off")
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }
        //endregion configTab
        //
        //region settingsTab
        Item{
            id: settingsWindow
            visible: false
            anchors.fill: parent
            function show(){
                settingsWindow.visible = true
                stanleyPure.isChecked = !SettingsManager.vehicle_isStanleyUsed
            }

            Column {
                anchors.top: parent.top
                anchors.topMargin: 30 * theme.scaleHeight
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                spacing: 70 * theme.scaleHeight
                width: childrenRect.width
                SliderCustomized{
                    //Uturn Compensation
                    width: 250 * theme.scalewidth
                    centerTopText: qsTr("UTurn Compensation")
                    from: 2
                    to: 20
                    value: SettingsManager.as_uTurnCompensation * 10
                    onValueChanged: SettingsManager.as_uTurnCompensation = (value / 10)
                    leftText: value - 10
                }
                SliderCustomized {
                    //Sidehill deg
                    width: 250 * theme.scalewidth
                    centerTopText: qsTr("Sidehill Deg Turn per Deg of Roll")
                    from: 0.00
                    to:1.00
                    stepSize: .01
                    value: SettingsManager.as_sideHillCompensation
                    onValueChanged: SettingsManager.as_sideHillCompensation = value
                }
                Row{
                    spacing: 30 * theme.scaleWidth
                    IconButtonColor{
                        id: stanleyPure
                        text: qsTr("Stanley/Pure")
                        //stanleyPure.isChecked: !Settings.vehicle_isStanleyUsed
                        checkable: true
                        onCheckedChanged: SettingsManager.vehicle_isStanleyUsed = !checked
                        colorChecked: "white"
                        icon.source: prefix + "/images/ModeStanley.png"
                        iconChecked: prefix + "/images/ModePurePursuit.png"
                    }
                    IconButtonColor {
                        text: qsTr("Steer In Reverse?")
                        isChecked: SettingsManager.as_isSteerInReverse
                        checkable: true
                        onCheckedChanged: SettingsManager.as_isSteerInReverse = checked
                        icon.source: prefix + "/images/Config/ConV_RevSteer.png"
                    }
                }
            }
        }
        //endregion settingsTab
        //region safetySettingsTab
        Item{
            id: steerSafetyWindow
            anchors.fill: parent
            visible: steerSafetyBtn.checked

            GridLayout{
                id: safety
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 10 * theme.scaleHeight
                anchors.bottomMargin: 100 * theme.scaleHeight
                anchors.leftMargin: 70 * theme.scaleWidth
                anchors.rightMargin: 70 * theme.scaleWidth
                flow: Grid.TopToBottom
                rows: 8
                columns: 2
                Text{ text: qsTr("Manual Turns Limit"); Layout.alignment: Qt.AlignCenter}
                Image{
                    source: prefix + "/images/Config/con_VehicleFunctionSpeedLimit.png"
                    width: parent.width
                    height: 90 * theme.scaleHeight
                    Layout.alignment: Qt.AlignCenter
                    fillMode: Image.PreserveAspectFit
                }
                SpinBoxKM{
                    from: 0
                    to: 20
                    editable: true
                    boundValue: SettingsManager.as_functionSpeedLimit
                    onValueModified: SettingsManager.as_functionSpeedLimit = value
                    Layout.alignment: Qt.AlignCenter
                }
                Text{ text: qsTr(Utils.speed_unit()); Layout.alignment: Qt.AlignCenter}
                Text{ text: qsTr("Min AutoSteer Speed"); Layout.alignment: Qt.AlignCenter}
                Image{
                    id: minAutoSteerImage
                    source: prefix + "/images/Config/ConV_MinAutoSteer.png"
                    width: parent.width
                    height: 90 * theme.scaleHeight
                    Layout.alignment: Qt.AlignCenter
                    fillMode: Image.PreserveAspectFit
                }
                SpinBoxKM{
                    from: 0
                    to: 50
                    editable: true
                    boundValue: SettingsManager.as_minSteerSpeed
                    onValueModified: SettingsManager.as_minSteerSpeed = value
                    Layout.alignment: Qt.AlignCenter
                }
                Text{ text: qsTr(Utils.speed_unit()); Layout.alignment: Qt.AlignCenter}
                Text{ text: qsTr("Max AutoSteer Speed"); Layout.alignment: Qt.AlignCenter}
                Image{
                    id: maxAutoSteerImage
                    source: prefix + "/images/Config/ConV_MaxAutoSteer.png"
                    height: 90 * theme.scaleHeight
                    width: parent.width
                    Layout.alignment: Qt.AlignCenter
                    fillMode: Image.PreserveAspectFit
                }
                SpinBoxKM{
                    from: 0
                    to: 50
                    editable: true
                    boundValue: SettingsManager.as_maxSteerSpeed
                    onValueModified: SettingsManager.as_maxSteerSpeed = value
                    Layout.alignment: Qt.AlignCenter
                }
                Text{ text: qsTr(Utils.speed_unit()); Layout.alignment: Qt.AlignCenter}
                Text{ text: qsTr("Max Turn Rate"); Layout.alignment: Qt.AlignCenter}
                Image{
                    source: prefix + "/images/Config/ConV_MaxAngVel.png"
                    width: parent.width
                    height: 90 * theme.scaleHeight
                    Layout.alignment: Qt.AlignCenter
                    fillMode: Image.PreserveAspectFit
                }

                //The from and to values are deg/sec, but the final value output is in radians always
                SpinBoxCustomized {
                    Layout.alignment: Qt.AlignCenter
                    id: spinner
                    from: 5
                    to: 100
                    editable: true
                    value: Utils.radians_to_deg(SettingsManager.vehicle_maxAngularVelocity) // should be in radians!
                    onValueChanged: SettingsManager.vehicle_maxAngularVelocity = Utils.deg_to_radians(value)
                }
                Text{ text: qsTr("Degrees/sec"); Layout.alignment: Qt.AlignCenter}
            }
        }
        //endregion steerSafetySettings
    }
    //region steerSettingsTab
    Item{
        id: steerSettingsWindow
        anchors.fill: parent
        visible: steerSettingsBtn.checked
        /*    Rectangle{
                        id: lightbarrect
                        anchors.left: parent.left
                        anchors.top: parent.top
                        height: 150
                        width: 400
                        anchors.margins: 20
                        color: "transparent"
                        Text{
                            id: lightbartitletxt
                            text: qsTr("LightBar - Distance per pixel")
                            anchors.top: parent.top
                            anchors.left: parent.left
                        }

                        Image {
                            id: lightbarimage
                            source: prefix + "/images/Config/ConV_CmPixel.png"
                            anchors.left: parent.left
                            anchors.top: lightbartitletxt.bottom
                            anchors.bottom: parent.bottom
                            width: parent.width*.5
                            SpinBoxCM{
                                id: lightbarCmPerPixel
                                anchors.top: parent.top
                                anchors.topMargin: 25
                                height: 50
                                anchors.left: parent.right
                                anchors.leftMargin: 10
                                from: 0
                                to: 15
                                boundValue: SettingsManager.display_lightbarCmPerPixel
                                onValueModified: SettingsManager.display_lightbarCmPerPixel = value
                                editable: true
                                text: Utils.cm_unit() + " " + qsTr("per pixel","As in units per pixel")
                            }
                        }
                    }
                    */

        GridLayout{
            //id: safety
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 100 * theme.scaleHeight
            anchors.bottomMargin: 100 * theme.scaleHeight
            anchors.leftMargin: 70 * theme.scaleWidth
            anchors.rightMargin: 70 * theme.scaleWidth
            flow: Grid.TopToBottom
            rows: 2
            columns: 2



            //}
            //}
            Rectangle{
                id: linewidthrect
                // anchors.left: parent.left
                // anchors.top: nudgedistrect.bottom
                height: 150 * theme.scaleHeight
                width: 350 * theme.scaleWidth
                // anchors.topMargin: 20 * theme.scaleHeight
                // anchors.bottomMargin: 20 * theme.scaleHeight
                // anchors.leftMargin: 20 * theme.scaleWidth
                // anchors.rightMargin: 20 * theme.scaleWidth
                color: "transparent"
                TextLine{
                    id: linewidthtitletxt
                    text: qsTr("Line Width")
                    anchors.top: parent.top
                    //anchors.left: parent.left
                    Layout.alignment: Qt.AlignCenter
                }

                Image {
                    id: linewidthimage
                    source: prefix + "/images/Config/ConV_LineWith.png"
                    anchors.left: parent.left
                    anchors.top: linewidthtitletxt.bottom
                    anchors.bottom: parent.bottom
                    width: parent.width*.5
                    SpinBoxCustomized{
                        id: linewidthSetting
                        anchors.top: parent.top
                        anchors.topMargin: 25
                        height: 50 * theme.scaleHeight
                        anchors.left: parent.right
                        anchors.leftMargin: 10 * theme.scaleWidth
                        from: 1
                        to: 8
                        boundValue: SettingsManager.display_lineWidth
                        onValueModified: SettingsManager.display_lineWidth = value
                        text: qsTr("pixels")
                        editable: true
                    }
                }
            }

            Rectangle{
                id: nudgedistrect
                // anchors.top: parent.top
                // anchors.horizontalCenter: parent.horizontalCenter
                height: 150 * theme.scaleHeight
                width: 350 * theme.scaleWidth
                // anchors.topMargin: 20 * theme.scaleHeight
                // anchors.bottomMargin: 20 * theme.scaleHeight
                // anchors.leftMargin: 20 * theme.scaleWidth
                // anchors.rightMargin: 20 * theme.scaleWidth
                color: "transparent"
                TextLine{
                    id: nudgedisttitletxt
                    text: qsTr("Nudge Distance")
                    anchors.top: parent.top
                    //anchors.left: parent.left
                    Layout.alignment: Qt.AlignCenter
                }

                Image {
                    id: nudgedistimage
                    source: prefix + "/images/Config/ConV_SnapDistance.png"
                    anchors.left: parent.left
                    anchors.top: nudgedisttitletxt.bottom
                    anchors.bottom: parent.bottom
                    width: parent.width*.5
                    SpinBoxCM{
                        id: snapDistance
                        anchors.top: parent.top
                        anchors.topMargin: 25
                        height: 50 * theme.scaleHeight
                        anchors.left: parent.right
                        anchors.leftMargin: 10 * theme.scaleWidth
                        from: 0
                        to: 1000
                        boundValue: SettingsManager.as_snapDistance
                        onValueModified: SettingsManager.as_snapDistance = value
                        editable: true
                        text: Utils.cm_unit()
                    }
                }
            }
            IconButtonColor{
                // anchors.right: parent.right
                // anchors.top:parent.top
                // anchors.topMargin: 20 * theme.scaleHeight
                // anchors.bottomMargin: 20 * theme.scaleHeight
                // anchors.leftMargin: 20 * theme.scaleWidth
                // anchors.rightMargin: 20 * theme.scaleWidth
                icon.source: prefix + "/images/AutoSteerOff.png"
                iconChecked: prefix + "/images/AutoSteerOn.png"
                checkable: true
                visible: false
                color: "red"
                isChecked: SettingsManager.as_isAutoSteerAutoOn
                onCheckableChanged: SettingsManager.as_isAutoSteerAutoOn = checked
                //text: qsTr("Steer Switch Control")
                font.pixelSize:15
                implicitWidth: 120 * theme.scaleWidth
                implicitHeight: 150 * theme.scaleHeight
                Layout.alignment: Qt.AlignCenter
                TextLine{
                    text: qsTr("Steer Switch Control")
                    anchors.bottom: parent.top
                    anchors.bottomMargin: 20 * theme.scaleHeight
                    //anchors.left: parent.left
                    Layout.alignment: Qt.AlignCenter
                }
            }
            Rectangle{
                id: lineacqLAheadrect
                // anchors.left: linewidthrect.right
                // anchors.verticalCenter: linewidthrect.verticalCenter
                // anchors.topMargin: 50 * theme.scaleHeight
                // anchors.bottomMargin: 50 * theme.scaleHeight
                // anchors.leftMargin: 50 * theme.scaleWidth
                // anchors.rightMargin: 50 * theme.scaleWidth
                height: 150 * theme.scaleHeight
                width: 350 * theme.scaleWidth
                color: "transparent"
                TextLine{
                    id: lineacqLAheadtitletxt
                    text: qsTr("Line Acquire Look Ahead")
                    anchors.top: parent.top
                    //anchors.left: parent.left
                    Layout.alignment: Qt.AlignCenter
                }

                Image {
                    id: lineacqLAheadimage
                    source: prefix + "/images/Config/ConV_GuidanceLookAhead.png"
                    anchors.left: parent.left
                    anchors.top: lineacqLAheadtitletxt.bottom
                    anchors.bottom: parent.bottom
                    width: parent.width*.5
                    SpinBoxOneDecimal{
                        id: lineacqLAheadSetting
                        anchors.top: parent.top
                        anchors.topMargin: 40 * theme.scaleHeight
                        //height: 50 * theme.scaleHeight
                        anchors.left: parent.right
                        anchors.leftMargin: 10 * theme.scaleWidth
                        from: 0.1
                        to: 10
                        boundValue: SettingsManager.as_guidanceLookAheadTime
                        onValueModified: SettingsManager.as_guidanceLookAheadTime = value
                        editable: true
                        text: qsTr("Seconds")
                        decimals: 1
                    }
                }
            }
        }

        //endregion steerSettings
    }
    RowLayout{
        id: bottomRightButtons
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10 * theme.scaleHeight
        anchors.leftMargin: 10 * theme.scaleWidth
        anchors.rightMargin: 10 * theme.scaleWidth
        height: wizard.height
        IconButtonText{
            id: wizard
            text: qsTr("Wizard")
            icon.source: prefix + "/images/WizardWand.png"
            Layout.alignment: Qt.AlignCenter
            visible: false //TODO: because the wizard isn't implemented
        }

        IconButtonTransparent{
            id: reset
            icon.source: prefix + "/images/UpArrow64.png"
            Layout.alignment: Qt.AlignLeft
            Text {
                text: qsTr("Reset All To Defaults")
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.right
                anchors.leftMargin: 5
            }
            onClicked: {
                steerMessageDialog.close()
                steerMessageDialog.text = "Reset This Page to Defaults";
                steerMessageDialog.title = "Are you Sure";
                steerMessageDialog.buttons = MessageDialog.Yes | MessageDialog.No;

                // Connect new handlers for this instance
                steerMessageDialog.accepted.connect(() => {
                                                        settingsArea.reset_all();

                                                    });

                steerMessageDialog.visible = true;
            }
        }
        IconButtonTransparent{
            id: send
            //enabled: false
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: 10 * theme.scaleWidth
            icon.source: notSaved?prefix + "/images/ToolAcceptNotSaved.png":prefix + "/images/ToolAcceptChange.png"
            implicitWidth: 130
            onClicked: { settingsArea.save_settings() ; notSaved = false ;
                aog.settingsReload();} // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            Text {
                text: qsTr("Send + Save")
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.left
                anchors.rightMargin: 5
            }
        }

        IconButtonTransparent{
            id: ok
            icon.source: prefix + "/images/OK64.png"
            Layout.alignment: Qt.AlignRight
            onClicked: {
                steerConfig.visible = false
            }
        }
    }

    MessageDialog{
        id: steerMessageDialog
        visible: false
    }
}
