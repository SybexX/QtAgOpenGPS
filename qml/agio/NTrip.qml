import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import "../components" as Comp
Dialog {
    id: ntrip
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: false

    property bool notSaved: false
    property bool ntripEnabled: false

    function show(){
        parent.visible = true
        notSaved = false
    }

    function save_settings() {
        // Save ALL form values to SettingsManager (batch update)
        SettingsManager.ntrip_url = ntripURL.text
        SettingsManager.ntrip_userName = ntripUser.text
        SettingsManager.ntrip_password = ntripPass.text
        SettingsManager.ntrip_mount = ntripMount.text
        SettingsManager.ntrip_port = ntripPort.value
        SettingsManager.ntrip_isTCP = ntripEnabled
    }

    Rectangle {
        id: main
        visible: true
        color: aogInterface.backgroundColor
        anchors.fill: parent

        Comp.TopLine{
            id: topLine
            titleText: qsTr("NTRIP RTK settings")
            onBtnCloseClicked:  ntrip.close()
        }

        Rectangle {
            id: source
            visible: true
            color: aogInterface.backgroundColor
            anchors.top: topLine.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomButtons.top

            //TODO: all the ntrip diagnostics stuff. I'm too lazy to do it now.
            /*Comp.ScrollableTextArea{
            id: rawTripTxt
            property int rawTripCount: 0
            property double blah: 0
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: leftColumn.top
            anchors.right: rightColumn.left
            anchors.margins: 40
            onRawTripCountChanged: {
                rawTripTxt.append(rawTripCount)
            }
        }*/

            ColumnLayout {
                id: leftColumn
                anchors.top: parent.top
                anchors.left: parent.left
                width: parent.width/2
                anchors.bottom: parent.bottom
                spacing: 5
                anchors.topMargin: 15 * theme.scaleHeight
                anchors.bottomMargin: 15 * theme.scaleHeight
                // NTRIP Connection Status Indicator
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 35
                    Layout.maximumWidth: 240 * theme.scaleWidth
                    Layout.alignment: Qt.AlignHCenter
                    color: {
                        if (AgIOService.ntripConnected) return "#28a745"      // Green - Connected
                        if (AgIOService.ntripStatus === 5) return "#dc3545"   // Red - Error
                        if (AgIOService.ntripStatus === 0) return "#6c757d"   // Gray - Disconnected
                        return "#ffc107"                                       // Yellow - Connecting
                    }
                    radius: 5
                    border.width: 2
                    border.color: "#000000"

                    Text {
                        anchors.centerIn: parent
                        text: AgIOService.ntripStatusText || "Disconnected"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                Comp.TextLine {
                    text: "Ntrip Stat: " +
                          ( AgIOService.ntripStatus === 0 ? "Invalid" :
                                                            AgIOService.ntripStatus === 1 ? "Authorizing" :
                                                                                            AgIOService.ntripStatus === 2 ? "Waiting" :
                                                                                                                            AgIOService.ntripStatus === 3 ? "Send GGA" :
                                                                                                                                                            AgIOService.ntripStatus === 4 ? "Listening NTRIP":
                                                                                                                                                                                            AgIOService.ntripStatus === 5 ? "Wait GPS":
                                                                                                                                                                                                                            "Unknown")
                }

                Comp.SpinBoxCustomized { //ntrip caster port number
                    id: ntripPort
                    Component.onCompleted: value = SettingsManager.ntrip_port  // Initialize once, no binding
                    from: 0
                    to: 65535
                    text: qsTr("Caster Port:")
                    Layout.alignment: Qt.AlignHCenter
                    editable: true
                    onValueModified: notSaved = true
                }
                Comp.Text {
                    text: qsTr("Default: 2101")
                    color: "red"
                    visible: SettingsManager.ntrip_port !== 2101
                    Layout.alignment: Qt.AlignHCenter
                    //font.bold: true
                    font.pixelSize: 18
                }
            }
            ColumnLayout {
                id: rightColumn
                anchors.top: parent.top
                anchors.right: parent.right
                width: parent.width/2
                anchors.bottom: parent.bottom
                spacing: 5
                anchors.topMargin: 15 * theme.scaleHeight
                anchors.bottomMargin: 15 * theme.scaleHeight

                TextField {
                    id: ntripURL
                    Component.onCompleted: text = SettingsManager.ntrip_url  // Initialize once, no binding
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    height: 49
                    selectByMouse: true
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    Comp.Text {
                        text: qsTr("Enter Broadcaster URL or IP")
                        Layout.alignment: Qt.AlignHCenter
                        //font.bold: true
                        font.pixelSize: 18
                        anchors.bottom: parent.top
                        anchors.left: parent.left

                    }
                }

                TextField {
                    id: ntripUser
                    Component.onCompleted: text = SettingsManager.ntrip_userName  // Initialize once, no binding
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    selectByMouse: true
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    Comp.Text {
                        text: qsTr("Username")
                        Layout.alignment: Qt.AlignHCenter
                        //font.bold: true
                        font.pixelSize: 18
                        anchors.bottom: parent.top
                        anchors.left: parent.left
                    }
                }

                //Comp.Spacer{}


                TextField {
                    id: ntripPass
                    Component.onCompleted: text = SettingsManager.ntrip_password  // Initialize once, no binding
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    selectByMouse: true
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    Comp.Text {
                        text: qsTr("Password")
                        Layout.alignment: Qt.AlignHCenter
                        //font.bold: true
                        font.pixelSize: 18
                        anchors.bottom: parent.top
                        anchors.left: parent.left
                    }
                }
                //Comp.Spacer{}
                //          Comp.ButtonColor {
                // 	text: qsTr("Get Source Table")
                //              width: parent.width * .9
                // }

                TextField {
                    id: ntripMount
                    Component.onCompleted: text = SettingsManager.ntrip_mount  // Initialize once, no binding
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    selectByMouse: true
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Qt.AlignHCenter
                    Comp.Text {
                        text: qsTr("Mount:", "Ntrip Mountpoint")
                        Layout.alignment: Qt.AlignHCenter
                        //font.bold: true
                        font.pixelSize: 18
                        anchors.bottom: parent.top
                        anchors.left: parent.left

                    }
                }
            }
        }
        RowLayout {
            height: defaults.height  + 20 * theme.scaleHeight
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottomMargin: 5 * theme.scaleHeight
            spacing: 20 * theme.scaleWidth


            Comp.IconButtonTransparent{
                id: defaults
                Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                Layout.topMargin: 20 * theme.scaleHeight
                Layout.bottomMargin: 5 * theme.scaleHeight
                Layout.leftMargin: 10 * theme.scaleWidth
                icon.source: prefix + "/images/UpArrow64.png"
                onClicked: {
                    notSaved = true
                }
            }

            Comp.ButtonColor {
                id: ntripOn
                text: ntripEnabled ? qsTr("NTRIP On") : qsTr("NTRIP Off")
                Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                Layout.topMargin: 20 * theme.scaleHeight
                Layout.bottomMargin: 5 * theme.scaleHeight
                checkable: true
                Component.onCompleted: ntripEnabled = SettingsManager.ntrip_isTCP
                checked: ntripEnabled

                onClicked: {
                    ntripEnabled = !ntripEnabled
                    AgIOService.configureNTRIP()
                }
            }

            Item { Layout.fillWidth: true }

            Comp.IconButtonTransparent{
                id: btnSave
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                Layout.topMargin: 20 * theme.scaleHeight
                Layout.bottomMargin: 5 * theme.scaleHeight
                icon.source: notSaved?prefix + "/images/ToolAcceptNotSaved.png":prefix + "/images/ToolAcceptChange.png"
                Text{
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.left
                    anchors.rightMargin: 5
                    text: qsTr("Send + Save")
                }
                onClicked: {
                    notSaved = false
                    save_settings()
                    AgIOService.configureNTRIP()
                }
            }

            Comp.IconButtonTransparent{
                id: saveAndClose
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                Layout.topMargin: 20 * theme.scaleHeight
                Layout.bottomMargin: 5 * theme.scaleHeight
                Layout.rightMargin: 10 * theme.scaleWidth
                icon.source: prefix + "/images/OK64.png"
                onClicked: {
                    ntrip.close()
                    notSaved = false
                }
            }
        }
    }
}
