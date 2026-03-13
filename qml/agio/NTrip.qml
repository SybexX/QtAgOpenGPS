import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import "../components" as Comp
Drawer{
    objectName: "ntrip"
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    modal: true
    id: ntrip

    // Qt 6.8 QProperty + BINDABLE: AgIOService is a singleton, use it directly

    //anchors.centerIn: parent
    visible: false
    // function show(){
    //     parent.visible = true
    // }

    // Comp.TopLine{
    //     id: topLine
    //     titleText: qsTr("NTRIP RTK settings")
    //     onBtnCloseClicked:  ntrip.close()
    // }

    Rectangle {
        id: source
        visible: true
        color: aogInterface.backgroundColor
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomButtons.top
        border.width: 2
        border.color: aogInterface.blackDayWhiteNight

        /*Comp.TextLine {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 100
            anchors.leftMargin: 100
            text: "Ntrip Stat: " +
                     ( SettingsManager.ntripStatus === 0 ? "Invalid" :
                      SettingsManager.ntripStatus === 1 ? "Authorizing" :
                      SettingsManager.ntripStatus === 2 ? "Waiting" :
                      SettingsManager.ntripStatus === 3 ? "Send GGA" :
                      SettingsManager.ntripStatus === 4 ? "Listening NTRIP":
                      SettingsManager.ntripStatus === 5 ? "Wait GPS":
                     "Unknown")
        }*/

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
            width: parent.width
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

            Comp.Text {
				text: qsTr("Enter Broadcaster URL or IP")
                Layout.alignment: Qt.AlignHCenter
                //font.bold: true
                font.pixelSize: 18
                //anchors.bottom: ntripURL.top

			}
			TextField {
				id: ntripURL
                Component.onCompleted: text = SettingsManager.ntrip_url  // Initialize once, no binding
                Layout.maximumWidth: 200 * theme.scaleWidth
                Layout.fillWidth : true
                height: 49
				selectByMouse: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Qt.AlignHCenter
			}
            Comp.ButtonColor {
				text: qsTr("Verify")
                onClicked:  SettingsManager.iPFromUrl = ntripURL.text
                width: parent.width * .9
                Layout.alignment: Qt.AlignHCenter
            }

            Comp.Text {
                text: "IP: " + SettingsManager.ntrip_casterIP
                Layout.alignment: Qt.AlignHCenter
                //font.bold: true
                font.pixelSize: 18
			}


            Comp.Text {
				text: qsTr("Username")
                Layout.alignment: Qt.AlignHCenter
                //font.bold: true
                font.pixelSize: 18
                //anchors.bottom: ntripUser.top
			}
			TextField {
				id: ntripUser
                Component.onCompleted: text = SettingsManager.ntrip_userName  // Initialize once, no binding
                Layout.maximumWidth: 200 * theme.scaleWidth
                Layout.fillWidth : true
				selectByMouse: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Qt.AlignHCenter
			}

            //Comp.Spacer{}

            Comp.Text {
				text: qsTr("Password")
                Layout.alignment: Qt.AlignHCenter
                //font.bold: true
                font.pixelSize: 18
                //anchors.bottom: ntripPass.top
			}
			TextField {
				id: ntripPass
                Component.onCompleted: text = SettingsManager.ntrip_password  // Initialize once, no binding
                Layout.maximumWidth: 200 * theme.scaleWidth
                Layout.fillWidth : true
                selectByMouse: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Qt.AlignHCenter
			}
            //Comp.Spacer{}
   //          Comp.ButtonColor {
            // 	text: qsTr("Get Source Table")
   //              width: parent.width * .9
            // }
            Comp.Text {
				text: qsTr("Mount:", "Ntrip Mountpoint")
                Layout.alignment: Qt.AlignHCenter
                //font.bold: true
                font.pixelSize: 18
                //anchors.bottom: ntripMount.top

			}
			TextField {
				id: ntripMount
                Component.onCompleted: text = SettingsManager.ntrip_mount  // Initialize once, no binding
                Layout.maximumWidth: 200 * theme.scaleWidth
                Layout.fillWidth : true
				selectByMouse: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Qt.AlignHCenter
			}
            Comp.Spacer{}
            Comp.SpinBoxCustomized { //ntrip caster port number
                id: ntripPort
                Component.onCompleted: value = SettingsManager.ntrip_port  // Initialize once, no binding
				from: 0
				to: 65535
                text: qsTr("Caster Port:")
                Layout.alignment: Qt.AlignHCenter
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
    }
	Row {
		height: 50* theme.scaleHeight
		id: bottomButtons
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		anchors.bottomMargin: 5 * theme.scaleHeight
		anchors.rightMargin: 5 * theme.scaleWidth
        spacing: 25 * theme.scaleWidth
        Comp.ButtonColor {
			id: ntripOn
			text: ntripEnabled ? qsTr("NTRIP On") : qsTr("NTRIP Off")
			height: parent.height
            width: height * 3.5
            checkable: true
            property bool ntripEnabled: false
            Component.onCompleted: ntripEnabled = SettingsManager.ntrip_isTCP
            checked: ntripEnabled

            onClicked: {
                // Toggle state
                ntripEnabled = !ntripEnabled

                // Save ALL form values to SettingsManager (batch update)
                SettingsManager.ntrip_url = ntripURL.text
                SettingsManager.ntrip_userName = ntripUser.text
                SettingsManager.ntrip_password = ntripPass.text
                SettingsManager.ntrip_mount = ntripMount.text
                SettingsManager.ntrip_port = ntripPort.value
                SettingsManager.ntrip_isTCP = ntripEnabled

                // Apply configuration (starts or stops NTRIP based on ntrip_isTCP)
                AgIOService.configureNTRIP()
            }
		}
        Comp.IconButtonTransparent {
			id: cancel
            visible: false //not sure if we even want/need this
			height: parent.height
			width: height
            icon.source: "../images/Cancel64.png"
            onClicked: ntrip.visible = false
		}
	}
}

