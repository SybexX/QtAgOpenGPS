// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Menu when we want to open a port
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import ".."
import "../components"

Dialog {
    id:portMenu
    //color: "ghostwhite"
    visible: false
    height: 500  * theme.scaleHeight
    width:700  * theme.scaleWidth
    anchors.centerIn: parent
    modal: false
    function show(){
        parent.visible = true
    }

    property int sortBy: 1
    property string portBaud: ""
    property string portName: ""
    property bool saveConfig: false
    property string moduleType: ""        // Module type (GPS, IMU, Steer, Machine, Blockage)
    property bool isBluetoothScanning: true

    TopLine{
        id: topLine
        titleText: qsTr(moduleType + " Input Source")
        onBtnCloseClicked:  portMenu.close()
    }

    SourseTable {
        id: sourseTable
        anchors.top: topLine.bottom
        anchors.bottom: grid3.top
        width: parent.width - 10
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 5
        anchors.rightMargin: 15
        anchors.bottomMargin: 10
        portName:portMenu.portName
        portBaud:portMenu.portBaud
        adjustWidth: - scrollbar.width
        sortBy: portMenu.sortBy
        anchors.right: topLine.right

        ScrollBar.vertical: ScrollBar {
            id: scrollbar
            anchors.left: portMenu.right
            anchors.rightMargin: 10
            width: 10
            policy: ScrollBar.AlwaysOn
            active: true
            contentItem.opacity: 1
        }
    }

    Dialog {
        id: bluetoothScanningDialog
        title: qsTr("Scan for Bluetooth devices")
        modal: false
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape

        ColumnLayout {
            anchors.centerIn: parent

            BusyIndicator {
                id: control
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 80 * theme.scaleHeight
                implicitHeight: implicitWidth

                contentItem: Item {
                    anchors.fill: parent

                    Item {
                        id: item
                        anchors.centerIn: parent
                        width: parent.width
                        height: parent.height
                        opacity: control.running ? 1 : 0

                        Behavior on opacity {
                            OpacityAnimator {
                                duration: 250
                            }
                        }

                        RotationAnimator {
                            target: item
                            running: control.visible && control.running
                            from: 0
                            to: 360
                            loops: Animation.Infinite
                            duration: 1250
                        }

                        Repeater {
                            id: repeater
                            model: 6

                            Rectangle {
                                x: item.width / 2 - width / 2
                                y: item.height / 2 - height / 2
                                implicitWidth: 10
                                implicitHeight: 10
                                radius: 5
                                color: "dark blue"
                                transform: [
                                    Translate {
                                        y: -Math.min(item.width, item.height) * 0.5 + 5
                                    },
                                    Rotation {
                                        angle: index / repeater.count * 360
                                        origin.x: 5
                                        origin.y: 5
                                    }
                                ]
                            }
                        }
                    }
                }
            }

            Button {
                text: qsTr("Cancel")
                visible: true
                Layout.alignment: Qt.AlignHCenter
                onClicked: {
                    bluetoothScanningDialog.close();
                }
            }
        }

        onClosed: {
            console.log("Bluetooth scanning dialog closed");
            if (isBluetoothScanning) {
                //deviceManager.stopBluetoothDiscovery();
                isBluetoothScanning = false;
            }
        }
    }

    Rectangle{
        id: grid3
        z: 4
        width: parent.width - 10
        height: deleteport.height + 10
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.bottom: parent.bottom
        //color: "white"
        color: Material.backgroundColor
        Row {
            anchors.fill: parent
            spacing: 4
            //flow: Grid.TopToBottom
            //rows: 1
            IconButtonTransparent {
                id: deleteport
                objectName: "btnDeleteport"
                icon.source: prefix + "/images/Trash.png"
                text: qsTr("Delete port")
                enabled: sourseTable.currentIndex > -1 && sourseTable.currentType === "BT"
                onClicked: {
                    portInterface.deleteport(sourseTable.currentFieldName)
                    // После удаления список должен обновиться (через backend)
                }
            }
            IconButtonTransparent {
                id: btSearch
                icon.source: prefix + "/images/BlueTooth.png"
                text: qsTr("Discover BT")
                enabled: sourseTable.currentIndex > -1
                onClicked: {bluetoothScanningDialog.open(); // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                }
            }
        }
        Row {
            spacing: 5
            anchors.right: parent.right
            IconButtonTransparent {
                id: sort
                icon.source: prefix + "/images/Sort.png"
                //color3: "white"
                //height: 75
                text: qsTr("Toggle Sort")
                //radius: 0

                //border: 1
                onClicked: {
                    sourseTable.sortBy = (sourseTable.sortBy % 3) + 1
                }
            }

            IconButtonTransparent {
                id: cancel
                objectName: "btnCancel"
                icon.source: prefix + "/images/Cancel64.png"
                text: qsTr("Cancel")
                //color3: "white"
                //radius: 0
                //border: 1
                //height: 75
                onClicked: {
                    sourseTable.clear_selection()
                    portMenu.close()
                    //closeDialog()
                }
            }
            IconButtonTransparent {
                id: useSelected
                objectName: "btnUseSelected"
                icon.source: prefix + "/images/OK64.png"
                text: qsTr("Use Selected")
                //radius: 0
                //color3: "white"
                //border: 1
                //height: 75
                enabled: sourseTable.currentIndex > -1
                onClicked: {
                    portInterface.openport(sourseTable.currentportName) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                    sourseTable.clear_selection()
                    portMenu.close()
                }
            }
        }
    }
}
