import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Window
import QtQuick.Layouts
import QtQml.Models
import AOG

import "../components" as Comp

Drawer {
    objectName: "bluetoothMenu"
    id: bluetoothMenu
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true

    // Qt 6.8 QProperty + BINDABLE: Use AgIOInterface instead of direct AgIOService access
    AgIOInterface {
        id: agioInterface
    }

    // Qt 6.8 QProperty + BINDABLE: Simple properties to allow setProperty() updates from C++
    property var setBluetoothDeviceList: []



    Comp.TitleFrame{
        id: devicesTitleFrame
        title: qsTr("Connect to Device:")
        height: mainWindow.height*0.7
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        border.width: 2
        ListView{
            id: bluetoothDevices
            anchors.fill: parent

            // Custom ListModel to store Bluetooth devices
            model: ListModel {
                id: deviceListModel
            }
            delegate: RadioButton{
                width: bluetoothDevices.width
                id: control

                indicator: Rectangle{
                    anchors.fill: parent
                    color: control.down ? "blue" : devicesTitleFrame.color
                    visible: true
                    anchors.margins: 5
                    border.color: "black"
                    border.width: 1
                    radius: 3
                    Text{
                        text: model.name
                        anchors.centerIn: parent
                    }
                    MouseArea{
                        anchors.fill: parent
                        onClicked:  SettingsManager.bt_search = model.name
                    }
                }
            }

            // Connections {
            //     target: bluetoothDeviceList
            // 
            //     // When the backend model changes, update the ListModel
            //     function onModelChanged() {
            //         bluetoothDevices.update_list()
            //     }
            // }
            function update_list(){
                // Clear the QML model
                deviceListModel.clear()

                // TODO: Populate with SettingsManager.setBluetooth_deviceList data
                // for (let i = 0; i < SettingsManager.setBluetooth_deviceList.length; ++i) {
                //     let name = SettingsManager.setBluetooth_deviceList[i];
                //     deviceListModel.append({"name": name});
                // }

            }

            // Initial data load when the component is created
            Component.onCompleted: {
                update_list()
            }

        }

    }
    Comp.TitleFrame{
        id: knownTitleFrame
        title: qsTr("Known Devices (Click to remove)")
        anchors.left: parent.left
        anchors.top: devicesTitleFrame.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        border.width: 2
        ListView{
            property var deviceList: []
            id: knownDevicesList
            anchors.fill: parent
            // Qt 6.8 QProperty + BINDABLE: Direct property binding (automatic updates)
            model: {
                var rawList = setBluetoothDeviceList;
                return Array.isArray(rawList) ? rawList : [rawList];
            }

            delegate: RadioButton{
                width: knownDevicesList.width
                id: knownControl

                indicator: Rectangle{
                    anchors.fill: parent
                    color: knownControl.down ? "blue" : devicesTitleFrame.color
                    visible: true
                    anchors.margins: 5
                    border.color: "black"
                    border.width: 1
                    radius: 3
                    Text{
                        text: modelData
                        anchors.centerIn: parent
                    }
                    MouseArea{
                        anchors.fill: parent
                        onClicked:  SettingsManager.bt_remove_device = modelData
                    }
                }
            }
        }
    }
}
