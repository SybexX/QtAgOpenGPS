import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import AOG

import "../components" as Comp
import "interfaces"

Drawer {
    id: advancedMenu
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true
    function show(){
        parent.visible = true
    }

    // Comp.TopLine{
    //     id: topLine
    //     titleText: qsTr("Advanced settings")
    //     onBtnCloseClicked:  advancedMenu.close()
    // }


    Rectangle{
        id: content
        visible: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        border.width: 2
        border.color: aogInterface.blackDayWhiteNight
        color: aogInterface.backgroundColor
        ColumnLayout{
            id: contGrid
            anchors.fill: parent
            anchors.margins: 10
            //flow: Grid.TopToBottom
            //columns: 2
            Comp.CheckBoxCustomized{
                id: ckUDPListenOnly
                text: "UDP Listen Only"
                checked: false
                onCheckedChanged:  AgIOService.udpListenOnly = ckUDPListenOnly.checked
                //Layout.alignment: Qt.AlignLeft
            }
            Comp.CheckBoxCustomized {
                id: ckNtripDebug
                text: "Console NTRIP Debug"
                checked: false
                onCheckedChanged:  AgIOService.ntripDebugEnabled = ckNtripDebug.checked
                //Layout.alignment: Qt.AlignRight
            }
            Comp.CheckBoxCustomized {
                id: ckBluetoothDebug
                text: "Console Bluetooth Debug"
                checked: false
                onCheckedChanged:  AgIOService.bluetoothDebugEnabled = ckBluetoothDebug.checked
                //Layout.alignment: Qt.AlignLeft
            }
        }
    }
}

