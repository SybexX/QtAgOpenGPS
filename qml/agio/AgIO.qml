// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main menu when we click the "Field" button on main screen. "New, Drive In, Open, Close, etc"
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Window
import AOG

import "../components" as Comp


Drawer {
    id: mainWindowAgIO
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true

    Comp.Message {
        id: message
    }
    // NTrip{
    //     id: ntrip
    //     visible: false
    // }
    // EthernetConfig {
    //     id: ethernetConfig
    //     visible: false
    //     onVisibleChanged: {
    //         if(visible)
    //             ethernetConfig.load_settings()
    //     }
    // }



    // Comp.SerialTerminalAgio {
    //     id: gnssConfig
    //     visible: false
    //     portBaud: SettingsManager.gnss_BaudRate
    //     portName: SettingsManager.gnss_SerialPort
    //     saveConfig: false
    //     enableMonitoring: true
    //     moduleType: "GPS"
    //     onSaveConfigChanged: {
    //         SettingsManager.gnss_SerialPort = gnssConfig.portName
    //         SettingsManager.gnss_BaudRate = gnssConfig.portBaud
    //         }
    // }
    Comp.SerialTerminalAgio {
        id: imuConfig
        visible: false
        portBaud: SettingsManager.imu_BaudRate
        portName: SettingsManager.imu_SerialPort
        saveConfig: false
        enableMonitoring: true
        moduleType: "IMU"
        onSaveConfigChanged: {
            SettingsManager.imu_SerialPort = imuConfig.portName
            SettingsManager.imu_BaudRate = imuConfig.portBaud
            }
    }
    Comp.SerialTerminalAgio {
        id: autosteerConfig
        visible: false
        portBaud: SettingsManager.steer_BaudRate
        portName: SettingsManager.steer_SerialPort
        saveConfig: false
        enableMonitoring: true
        moduleType: "Steer"
        onSaveConfigChanged: {
            SettingsManager.steer_SerialPort = autosteerConfig.portName
            SettingsManager.steer_BaudRate = autosteerConfig.portBaud
            }
    }
    Comp.SerialTerminalAgio {
        id: machineConfig
        visible: false
        portBaud: SettingsManager.machine_BaudRate
        portName: SettingsManager.machine_SerialPort
        saveConfig: false
        enableMonitoring: true
        moduleType: "Machine"
        onSaveConfigChanged: {
            SettingsManager.machine_SerialPort = machineConfig.portName
            SettingsManager.machine_BaudRate = machineConfig.portBaud
            }
    }
    Comp.SerialTerminalAgio {
        id: blockageConfig
        visible: false
        portBaud: SettingsManager.blockage_BaudRate
        portName: SettingsManager.blockage_SerialPort
        saveConfig: false
        enableMonitoring: true
        moduleType: "Blockage"
        onSaveConfigChanged: {
            SettingsManager.blockage_SerialPort = blockageConfig.portName
            SettingsManager.blockage_BaudRate = blockageConfig.portBaud
            }
    }
    AgIOInterface {
        id: agio
        objectName: "agio"
    }
    // GPSInfo {
    //     id: gpsInfo
    //     visible: false
    // }
    AgDiag {
        id: agdiag
    }

    SettingsWindow{
        id: settingsWindow
        visible: false
    }
    AdvancedMenu{
        id: advancedMenu
        function showMenu(){
            advancedMenu.visible = true
        }
    }
    BluetoothMenu{
        id: bluetoothMenu
        visible: false
    }
    UnitConversion {
        id: utils
    }

    contentItem: Rectangle{
        id: windowAgIO
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        height: mainWindowAgIO.height

        //border.color: "lightblue"
        //border.width: 2
        color: "black"

        Comp.ScrollViewExpandableColumn{
            id: mainWindowAgIOColumn
            anchors.fill: parent
            Comp.IconButtonTextBeside{
                //objectName: bluetooth
                text: qsTr("Bluetooth")
                icon.source: prefix + "/images/BlueTooth.png"
              //  color:  AgIOService.bluetoothConnected ? "green" : "red"
                onClicked: {
                    settingsWindow.close()
                    if(!utils.isTrue(SettingsManager.setBluetooth_isOn)){ //start bt if off
                        SettingsManager.setBluetooth_isOn = true
                         SettingsManager.startBluetoothDiscovery()
                        console.log("ssb")
                    }
                    bluetoothMenu.visible = true
                }
            }
            Comp.IconButtonTextBeside {
                //objectName: btnModuleIMU
                isChecked: false
                text: qsTr("IMU")
                icon.source: prefix + "/images/B_IMU.png"
               // color:  AgIOService.imuConnected ? "green" : "red"
                onClicked: imuConfig.visible = !imuConfig.visible
            }
            Comp.IconButtonTextBeside {
                //objectName: btnModuleSteer
                isChecked: false
                text: qsTr("Steer")
                icon.source: prefix + "/images/Com_AutosteerModule.png"
                color:  AgIOService.steerConnected ? "green" : "red"
                onClicked: autosteerConfig.visible = !autosteerConfig.visible
            }
            Comp.IconButtonTextBeside {
                //objectName: btnModuleGPS
                isChecked: false
                text: qsTr("GPS")
                icon.source: prefix + "/images/B_GPS.png"
                color:  AgIOService.gpsConnected ? "green" : "red"
                onClicked: {gnssConfig.visible = !gnssConfig.visible
                            mainWindowAgIO.visible = false}

            }
            Comp.IconButtonTextBeside {
                //objectName: btnModuleMachine
                isChecked: false
                text: qsTr("Machine")
                icon.source: prefix + "/images/B_Machine.png"
                color:  AgIOService.machineConnected ? "green" : "red"
            }
            Comp.IconButtonTextBeside {
                //objectName: btnModuleBlockage
                isChecked: false
                text: qsTr("Blockage")
                icon.source: prefix + "/images/B_Blockage.png"
                color:  AgIOService.blockageConnected ? "green" : "red"
                // Threading Phase 1: Blockage visibility configuration
                visible: SettingsManager.seed_blockageIsOn
                onClicked: blockageConfig.visible = !blockageConfig.visible
            }
           
            Comp.IconButtonTextBeside {
                isChecked: false
                text: qsTr("GPS Info")
                icon.source: prefix + "/images/Nmea.png"
                onClicked: {gpsInfo.visible = !gpsInfo.visible
                    mainWindowAgIO.visible = false}
            }
            Comp.IconButtonTextBeside {
                //objectName: btnEthernetStatus
                isChecked: false
                text: qsTr("Ethernet")
                icon.source: prefix + "/images/B_UDP.png"
                color:  SettingsManager.ethernet_isOn ? "green" : "red"
                onClicked: {ethernetConfig.visible = !ethernetConfig.visible
                            mainWindowAgIO.visible = false}
            }
            Comp.IconButtonTextBeside {
                isChecked: false
                text: (SettingsManager.ntrip_isTCP === false ? "Off":
                     AgIOService.ntripStatus === 0 ? "Invalid" :
                     AgIOService.ntripStatus === 1 ? "Authorizing" :
                     AgIOService.ntripStatus === 2 ? "Waiting" :
                     AgIOService.ntripStatus === 3 ? "Send GGA" :
                     AgIOService.ntripStatus === 4 ? "Listening NTRIP":
                     AgIOService.ntripStatus === 5 ? "Wait GPS":
                    "Unknown")

                icon.source: prefix + "/images/NtripSettings.png"
                color:  (SettingsManager.setNTRIP_isOn === false ? "red":
                     AgIOService.ntripStatus === 0 ? "red" :
                     AgIOService.ntripStatus === 1 ? "yellow" :
                     AgIOService.ntripStatus === 2 ? "yellow" :
                     AgIOService.ntripStatus === 3 ? "yellow" :
                     AgIOService.ntripStatus === 4 ? "green":
                     AgIOService.ntripStatus === 5 ? "red":
                    "red")
                onClicked: {ntripMenu.visible = !ntripMenu.visible
                            mainWindowAgIO.visible = false}
            }
            
            // Test button for module wake-up
            Comp.IconButtonTextBeside {
                isChecked: SettingsManager.feature_isAgIOOn
                text: SettingsManager.feature_isAgIOOn ? qsTr("AgIO Service ON") : qsTr("AgIO Service OFF")
                icon.source: prefix + "/images/AgIO.png"
                color: SettingsManager.feature_isAgIOOn ? "green" : "red"
                onClicked: {
                    SettingsManager.feature_isAgIOOn = !SettingsManager.feature_isAgIOOn
                    console.log("🔧 AgIO Service toggled:", SettingsManager.feature_isAgIOOn)
                }
            }
            }

        Drawer {
            id: ntripMenu
            width: 270 * theme.scaleWidth
            height: mainWindow.height
            modal: true

            contentItem: Rectangle{
                id: ntripMenuContent
                anchors.fill: parent
                height: ntripMenu.height
                color: aogInterface.blackDayWhiteNight
            }

            Grid {
                id: grid2
                height: childrenRect.height
                width: childrenRect.width
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 5
                spacing: 10
                flow: Grid.TopToBottom
                rows: 2
                columns: 1

            Comp.IconButtonTextBeside{
                id: clientNTRIP
                icon.source: prefix + "/images/NTRIP_Client.png"
                text: qsTr("Client NTRIP")
                onClicked: { ntripMenu.visible = false, mainWindowAgIO.visible = false, clientNtripDialog.visible = true}
            }

            Comp.IconButtonTextBeside{
                id: serialNTRIP
                icon.source: prefix + "/images/NTRIP_Serial.png"
                text: qsTr("Serial NTRIP")
                onClicked: { ntripMenu.visible = false, mainWindowAgIO.visible = false, serialNtripDialog.visible = true}
            }
        }
    }
        }
    }
