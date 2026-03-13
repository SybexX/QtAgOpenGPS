import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
// 
import "../components"
import "interfaces"
import AOG



Dialog {
    id:gpsInfo
    //color: "ghostwhite"
    visible: false
    height: 500  * theme.scaleHeight
    width:700  * theme.scaleWidth
    anchors.centerIn: parent
    modal: false
    function show(){
        parent.visible = true
    }

    TopLine{
        id: topLine
        titleText: qsTr("Module and GPS Info")
        onBtnCloseClicked:  gpsInfo.close()
    }
    NTripInterface {
        id: ntrip
    }

    AgIOInterface {
        id: agioInterface
    }

    GridLayout {
        id: layout
        anchors.margins: 4
        anchors.bottomMargin: 132
        anchors.top: topLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: sentences.top
        rows: 4
        flow: Grid.TopToBottom

        Text {
            font.pixelSize: 10
            text: qsTr("Lat: ") + (Number(Backend.fixFrame.latitude).toLocaleString(Qt.locale(), 'f', 7))
            Layout.alignment: Qt.AlignLeft
        }

        Text {
            font.pixelSize: 10
            text: qsTr("Lon: ") + (Number(Backend.fixFrame.longitude).toLocaleString(Qt.locale(), 'f', 7))
            Layout.alignment: Qt.AlignLeft
        }

        Text {
            font.pixelSize: 10
            text: qsTr("Speed KMH: ") + Math.round( aog.speedKph * 100) / 100
            Layout.alignment: Qt.AlignLeft
        }
        //quality
        Text {
            font.pixelSize: 10
            text: qsTr("GPS Quality: ") + AgIOService.gpsQuality
            Layout.alignment: Qt.AlignLeft
        }

        //HDOP
        Text {
            font.pixelSize: 10
            text: "HDOP: " +  Backend.fixFrame.hdop
            Layout.alignment: Qt.AlignLeft
        }
        //# Sats
        Text {
            font.pixelSize: 10
            text: qsTr("# Sats: ", "Number of satellites") +  AgIOService.satellites // Rectangle Pattern: direct property access
            Layout.alignment: Qt.AlignLeft
        }
        Text {
            font.pixelSize: 10
            text: qsTr("Altitude: ") + (Number( Backend.fixFrame.altitude).toLocaleString(Qt.locale(), 'f', 2))
            Layout.alignment: Qt.AlignLeft
        }
        //age
        Text {
            font.pixelSize: 10
            text: qsTr("Age: ") + (Number( Backend.fixFrame.age).toLocaleString(Qt.locale(), 'f', 1))
            Layout.alignment: Qt.AlignLeft
        }
        Text {
            font.pixelSize: 10
            text: qsTr("VTG: ") + "todo"
        }
        Text {
            font.pixelSize: 10
            text: qsTr("Dual: ") + "todo"
            Layout.alignment: Qt.AlignLeft
        }
        Column{
            Text{
                font.pixelSize: 10
                text: qsTr("Hz: ") + Math.round( Backend.fixFrame.hz * 100) / 100
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: qsTr("Raw Hz: ") + Math.round( Backend.fixFrame.rawHz * 100) / 100  // nowHz property removed
                Layout.alignment: Qt.AlignLeft
            }
        }

        Text {
            font.pixelSize: 10
            text: qsTr("Heading: ") + Number(Backend.fixFrame.imuHeading).toLocaleString(Qt.locale(), 'f', 1)
            Layout.alignment: Qt.AlignLeft
        }
        Text {
            font.pixelSize: 10
            text: qsTr("Roll: ") + (Number(Backend.fixFrame.imuRoll).toLocaleString(Qt.locale(), 'f', 1))
            Layout.alignment: Qt.AlignLeft
        }
        Text {
            font.pixelSize: 10
            text: qsTr("Pitch: ") + (Number(Backend.fixFrame.imuPitch).toLocaleString(Qt.locale(), 'f', 1))
            Layout.alignment: Qt.AlignLeft
        }
        Text {
            font.pixelSize: 10
            text: qsTr("Yaw Rate: ") + (Number(Backend.fixFrame.yawRate).toLocaleString(Qt.locale(), 'f', 1))
            Layout.alignment: Qt.AlignLeft
        }
    }

    // PHASE 6.0.22.12: Dynamic protocol list - shows ALL active protocols (NMEA + PGN)
    TitleFrame {
        id: moduleStatus
        title: qsTr("Active Protocols")
        height: Math.min(400, protocolList.contentHeight + 60)  // Dynamic height based on protocol count
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: layout.bottom
        anchors.bottom: sentences.top
        anchors.margins: 4

        // Column Headers
        Row {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4
            spacing: 8

            Text {
                text: qsTr("Protocol")
                font.bold: true
                font.pixelSize: 10
                width: 100
            }
            Text {
                text: qsTr("Description")
                font.bold: true
                font.pixelSize: 10
                width: 200
            }
            Text {
                text: qsTr("Source")
                font.bold: true
                font.pixelSize: 10
                width: 150
            }
            Text {
                text: qsTr("Frequency")
                font.bold: true
                font.pixelSize: 10
                width: 80
            }
        }

        // Dynamic Protocol List
        ListView {
            id: protocolList
            anchors.top: headerRow.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 4
            clip: true

            model: AgIOService.activeProtocols

            delegate: Row {
                spacing: 8

                // Protocol ID
                Text {
                    text: modelData.id  // "$PANDA" or "PGN211"
                    font.pixelSize: 10
                    font.bold: modelData.id.startsWith("PGN")  // Bold for PGN binary protocols
                    width: 100
                }

                // Protocol Description
                Text {
                    text: modelData.description  // "Single Antenna + IMU"
                    font.pixelSize: 10
                    color: "gray"
                    width: 200
                    elide: Text.ElideRight
                }

                // Source (Transport + ID)
                Text {
                    text: modelData.source  // "UDP:192.168.1.126" or "Serial:COM1"
                    font.pixelSize: 10
                    color: {
                        if (modelData.source.startsWith("UDP")) return "green"
                        if (modelData.source.startsWith("Serial")) return "orange"
                        return "red"
                    }
                    width: 150
                }

                // Frequency
                Text {
                    text: Math.round(modelData.frequency * 10) / 10 + " Hz"
                    font.pixelSize: 10
                    width: 80
                }
            }
        }
    }

    TitleFrame{
        id: sentences
        title: qsTr("NMEA Sentences")
        height: parent.height * .35 // Reduced to make room for module status
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        ScrollView {
            anchors.fill: parent
            clip: true


        ColumnLayout {
            anchors.fill: parent
            anchors.bottomMargin: 0
            id: strings
            Text{
                font.pixelSize: 10
                text: "GGA: " +  AgIOService.gga
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "VTG: " +  AgIOService.vtg
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "RMC: " +  AgIOService.rmc
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "NDA: " +  AgIOService.panda
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "OGI: " +  AgIOService.paogi
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "HDT: " +  AgIOService.hdt
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "AVR: " +  AgIOService.avr
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "HPD: " +  AgIOService.hpd
                Layout.alignment: Qt.AlignLeft
            }
            Text{
                font.pixelSize: 10
                text: "SXT: " +  AgIOService.sxt
                Layout.alignment: Qt.AlignLeft
            }
            //Unknown sentences. Ones AOG just ignores. Most likely a wrong reciever config.
            Text{
                color: "red"
                font.pixelSize: 10
                text: qsTr("Unknown: ") +  AgIOService.unknownSentence
            }
        }
    }
    }
}
