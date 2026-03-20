import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import "../components" as Comp

Dialog {
    id:ethernetConfig
    //color: "ghostwhite"
    visible: false
    height: 500  * theme.scaleHeight
    width:700  * theme.scaleWidth
    anchors.centerIn: parent
    modal: false
    function show(){
        parent.visible = true
    }

    Comp.TopLine{
        id: topLine
        titleText: qsTr("Ethernet Configuration")
        onBtnCloseClicked:  ethernetConfig.close()
    }

    onVisibleChanged: {
        // Only load settings and refresh if components are ready
        if (visible && spIP1 && spIP2 && spIP3) {
            load_settings()
            // Refresh network interfaces when dialog opens if on Configuration tab
            if (tabBar.currentIndex === 0) {
                AgIOService.refreshNetworkInterfaces()
            }
        }
    }

    Component.onCompleted: {
        // Load settings once all components are created
        load_settings()
    }

    function load_settings(){
        if (spIP1 && spIP2 && spIP3) {
            spIP1.text = SettingsManager.ethernet_ipOne.toString()
            spIP2.text = SettingsManager.ethernet_ipTwo.toString()
            spIP3.text = SettingsManager.ethernet_ipThree.toString()
        }
    }

    Rectangle {
        id: ethIP
        visible: true
        color: aogInterface.backgroundColor
        anchors.top: topLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        border.width: 2
        border.color: aogInterface.blackDayWhiteNight

        // Phase 6.2.2 - Qt 6 TabBar + StackLayout Architecture
        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10 * theme.scaleHeight
            anchors.leftMargin: 5 * theme.scaleWidth
            anchors.rightMargin: 5 * theme.scaleWidth
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true

                onCurrentIndexChanged: {
                    // Refresh network interfaces when entering Configuration tab
                    if (currentIndex === 0) {
                        AgIOService.refreshNetworkInterfaces()
                    }
                }

                TabButton {
                    text: qsTr("Configuration")
                }
                TabButton {
                    text: qsTr("Monitor")
                }
                TabButton {
                    text: qsTr("Active Protocols")
                }
            }

            StackLayout {
                id: stackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // Configuration Tab Content
                Item {

                    ColumnLayout {
                        id: configColumn
                        width: parent.width - 20 * theme.scaleWidth
                        anchors.margins: 10 * theme.scaleWidth

                        // Phase 6.2.2 - UDP Enable/Disable Section

                        RowLayout {
                            id: udpRow
                            Layout.fillWidth: true

                            GroupBox {
                                title: " "
                                Layout.fillWidth: true
                                ColumnLayout {
                                    width: parent.width
                                    Text {
                                        color: "black"
                                        Layout.alignment: Qt.AlignCenter
                                        text: qsTr("Enable UDP")
                                        font.pixelSize: 11 * theme.scaleWidth
                                    }
                                Switch {
                                    id: udpEnableSwitch
                                    //text: qsTr("Enable UDP")
                                    checked: SettingsManager.ethernet_isOn
                                    onCheckedChanged: AgIOService.enableUDP(checked)
                                }
                                }
                            }

                            // Existing IP Configuration Section
                            GroupBox {
                                title: "Subnet Configuration"
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                ColumnLayout {
                                    width: parent.width
                                    Text {
                                        color: "black"
                                        Layout.alignment: Qt.AlignCenter
                                        text: qsTr("IP Address")
                                        font.pixelSize: 11 * theme.scaleWidth
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 5 * theme.scaleWidth

                                        TextField {
                                            id: spIP1
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 40 * theme.scaleWidth
                                            horizontalAlignment: TextInput.AlignHCenter
                                            validator: IntValidator { bottom: 0; top: 255 }
                                            inputMethodHints: Qt.ImhDigitsOnly
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }
                                        Text {
                                            color: "black"
                                            text: "."
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }
                                        TextField {
                                            id: spIP2
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 40 * theme.scaleWidth
                                            horizontalAlignment: TextInput.AlignHCenter
                                            validator: IntValidator { bottom: 0; top: 255 }
                                            inputMethodHints: Qt.ImhDigitsOnly
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }
                                        Text {
                                            color: "black"
                                            text: "."
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }
                                        TextField {
                                            id: spIP3
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 40 * theme.scaleWidth
                                            horizontalAlignment: TextInput.AlignHCenter
                                            validator: IntValidator { bottom: 0; top: 255 }
                                            inputMethodHints: Qt.ImhDigitsOnly
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }
                                        Text {
                                            color: "black"
                                            text: ".x"
                                            font.pixelSize: 11 * theme.scaleWidth
                                        }

                                        Button {
                                            id: ethIPSet
                                            text: qsTr("IP Set")
                                            Layout.preferredWidth: 80 * theme.scaleWidth
                                            font.pixelSize: 11 * theme.scaleWidth
                                            onClicked: {
                                                SettingsManager.ethernet_ipOne = parseInt(spIP1.text)
                                                SettingsManager.ethernet_ipTwo = parseInt(spIP2.text)
                                                SettingsManager.ethernet_ipThree = parseInt(spIP3.text)

                                                console.log("🌐 Configuring subnet:", SettingsManager.ethernet_ipOne + "." + SettingsManager.ethernet_ipTwo + "." + SettingsManager.ethernet_ipThree + ".x")

                                                // Direct call to AgIOService (replaces old SettingsManager.btnSendSubnet_clicked)
                                                AgIOService.configureSubnet()

                                                timedMessage.addMessage(2000, "IP Address Change", ("IP address changed to " +
                                                                                                    SettingsManager.ethernet_ipOne + "." + SettingsManager.ethernet_ipTwo + "." + SettingsManager.ethernet_ipThree + "!"))
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Comp.Spacer { height: 10 }

                        RowLayout {
                            id: availRow
                            Layout.fillWidth: true

                            // Network Interfaces Section
                            GroupBox {
                                id: avNetwork
                                title: "Network Interfaces\nAvailable"
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignTop


                                Column {
                                    width: parent.width
                                    spacing: 5 * theme.scaleHeight

                                    ScrollView {
                                        width: parent.width
                                        height: 100 * theme.scaleHeight

                                        ListView {
                                            id: interfacesList
                                            model: AgIOService.networkInterfaces

                                            delegate: Rectangle {
                                                width: parent.width
                                                height: 30 * theme.scaleHeight
                                                color: modelData.canBroadcast ? "#e8f5e8" : "#fff8dc"
                                                border.width: 1
                                                border.color: aogInterface.borderColor
                                                radius: 3

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 6

                                                    Text { color: "black";
                                                        text: "🌐 " + modelData.name
                                                        font.bold: true
                                                        font.pixelSize: 11 * theme.scaleWidth
                                                    }

                                                    Text { color: "black";
                                                        text: modelData.ip + " (subnet: " + modelData.subnet + ")"
                                                        Layout.fillWidth: true
                                                        font.pixelSize: 11 * theme.scaleWidth
                                                    }

                                                    Text {
                                                        text: modelData.canBroadcast ? "✅" : "❌"
                                                        color: modelData.canBroadcast ? "green" : "orange"
                                                        font.pixelSize: 15 * theme.scaleWidth
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Button {
                                        text: qsTr("Refresh Interfaces")
                                        onClicked: AgIOService.refreshNetworkInterfaces()
                                    }
                                }
                            }

                            // Module Detection & Auto-Configuration Section
                            GroupBox {
                                title: "Module Detection &\n Auto-Configuration"
                                Layout.alignment: Qt.AlignTop
                                Layout.fillHeight: true
                                width: parent.width*0.4
                                //height: avNetwork.height

                                Column {
                                    width: parent.width
                                    spacing: 10 * theme.scaleHeight

                                    ScrollView {
                                        width: parent.width
                                        height: 100 * theme.scaleHeight

                                        GridLayout {
                                            Layout.fillWidth: true
                                            columns: 2
                                            columnSpacing: 10 * theme.scaleWidth
                                            rowSpacing: 5 * theme.scaleHeight

                                            Text {
                                                color: "black";
                                                text: "Detected Module:"
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }
                                            Text {
                                                text: AgIOService.discoveredModuleIP || "Not detected"
                                                color: AgIOService.discoveredModuleIP ? "green" : "black"
                                                font.bold: AgIOService.discoveredModuleIP
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }

                                            Text {
                                                color: "black";
                                                text: "Module Subnet:"
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }
                                            Text {
                                                text: AgIOService.discoveredModuleSubnet || "Unknown"
                                                color: AgIOService.discoveredModuleSubnet ? "green" : "black"
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }

                                            Text {
                                                color: "black";
                                                text: "PC Can Reach:"
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }
                                            Text {
                                                text: AgIOService.discoveredModuleSubnet ?
                                                          (AgIOService.canReachModuleSubnet(AgIOService.discoveredModuleSubnet) ? "✅ Yes" : "❌ No") :
                                                          "⚠️ Unknown"
                                                color: AgIOService.discoveredModuleSubnet ?
                                                           (AgIOService.canReachModuleSubnet(AgIOService.discoveredModuleSubnet) ? "green" : "red") :
                                                           "orange"
                                                font.pixelSize: 11 * theme.scaleWidth
                                            }
                                        }
                                    }

                                    Item { Layout.fillHeight: true
                                        Layout.fillWidth: true}

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Button {
                                            text: qsTr("Scan Modules")
                                            onClicked: AgIOService.scanAllSubnets()
                                        }

                                        Button {
                                            id: autoConfigBtn
                                            text: qsTr("Auto-Configure")
                                            enabled: true  // Phase 6.0.38: Always enabled, will scan first

                                            // Phase 6.0.38: Flag to track if we should auto-configure after scan
                                            property bool shouldAutoConfigure: false

                                            // Phase 6.0.38: Listen for scan completion and auto-configure if requested
                                            Connections {
                                                target: AgIOService
                                                function onSubnetScanCompleted(subnet) {
                                                    if (autoConfigBtn.shouldAutoConfigure && subnet !== "") {
                                                        console.log("✅ Scan completed with subnet:", subnet, "- triggering auto-configuration")
                                                        autoConfigBtn.shouldAutoConfigure = false
                                                        AgIOService.autoConfigureFromDetection()
                                                    }
                                                }
                                            }

                                            onClicked: {
                                                // Phase 6.0.38: Set flag and scan - auto-config will trigger on completion
                                                console.log("🔄 Auto-Configure: Starting module scan first...")
                                                shouldAutoConfigure = true
                                                AgIOService.scanAllSubnets()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Monitor Tab Content
                Item {

                    ColumnLayout {
                        id: monitorColumn
                        anchors.fill: parent
                        anchors.margins: 10 * theme.scaleWidth

                        // Module Status Section
                        GroupBox {
                            title: "Module Status"
                            Layout.fillWidth: true

                            GridLayout {
                                columns: 2
                                columnSpacing: 10 * theme.scaleWidth
                                rowSpacing: 5 * theme.scaleHeight

                                Text { color: "black"; text: "Machine:" }
                                Text {
                                    property bool moduleOnline: (AgIOService.ready && AgIOService.traffic) ?
                                                                    (AgIOService.traffic.helloFromMachine < 3) : false
                                    text: (AgIOService.ready && AgIOService.traffic) ?
                                              (moduleOnline ? "✅ Online" : "❌ Offline (" + AgIOService.traffic.helloFromMachine + ")") :
                                              "⏳ Waiting..."
                                    color: (AgIOService.ready && AgIOService.traffic) ? (moduleOnline ? "green" : "red") : "gray"
                                    font.pixelSize: 15
                                }

                                Text { color: "black"; text: "AutoSteer:" }
                                Text {
                                    property bool moduleOnline: (AgIOService.ready && AgIOService.traffic) ?
                                                                    (AgIOService.traffic.helloFromAutoSteer < 3) : false
                                    text: (AgIOService.ready && AgIOService.traffic) ?
                                              (moduleOnline ? "✅ Online" : "❌ Offline (" + AgIOService.traffic.helloFromAutoSteer + ")") :
                                              "⏳ Waiting..."
                                    color: (AgIOService.ready && AgIOService.traffic) ? (moduleOnline ? "green" : "red") : "gray"
                                    font.pixelSize: 15
                                }

                                Text { color: "black"; text: "IMU:" }
                                Text {
                                    property bool moduleOnline: (AgIOService.ready && AgIOService.traffic) ?
                                                                    (AgIOService.traffic.helloFromIMU < 3) : false
                                    text: (AgIOService.ready && AgIOService.traffic) ?
                                              (moduleOnline ? "✅ Online" : "❌ Offline (" + AgIOService.traffic.helloFromIMU + ")") :
                                              "⏳ Waiting..."
                                    color: (AgIOService.ready && AgIOService.traffic) ? (moduleOnline ? "green" : "red") : "gray"
                                    font.pixelSize: 15
                                }
                            }
                        }

                        Comp.Spacer { height: 10 }

                        RowLayout {
                            id: gpRow
                            Layout.fillWidth: true

                            GroupBox {
                                title: "UDP Traffic"
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    columnSpacing: 10 * theme.scaleWidth
                                    rowSpacing: 5 * theme.scaleHeight

                                    Text { color: "black"; text: "UDP Out:" }
                                    Text {
                                        text: (AgIOService.ready && AgIOService.traffic) ?
                                                  (AgIOService.traffic.udpOutRate.toFixed(2) + " KB/s @ " + AgIOService.traffic.udpOutFreq.toFixed(1) + " Hz") :
                                                  "0.00 KB/s @ 0.0 Hz"
                                        color: (AgIOService.ready && AgIOService.traffic && AgIOService.traffic.udpOutRate > 0) ? "green" : "black"
                                        font.pixelSize: 15
                                    }

                                    Text { color: "black"; text: "UDP In:" }
                                    Text {
                                        text: (AgIOService.ready && AgIOService.traffic) ?
                                                  (AgIOService.traffic.udpInRate.toFixed(2) + " KB/s @ " + AgIOService.traffic.udpInFreq.toFixed(1) + " Hz") :
                                                  "0.00 KB/s @ 0.0 Hz"
                                        color: (AgIOService.ready && AgIOService.traffic && AgIOService.traffic.udpInRate > 0) ? "green" : "black"
                                        font.pixelSize: 15
                                    }
                                }
                            }

                            GroupBox {
                                title: "UDP Status"
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    columnSpacing: 10 * theme.scaleWidth
                                    rowSpacing: 5 * theme.scaleHeight

                                    Text { color: "black"; text: "UDP Active:" }
                                    Text {
                                        // Phase 6.0.21.7: Use ethernet_isOn (actual UDP state) instead of setUDP_isOn
                                        text: SettingsManager.ethernet_isOn ? "✅ Enabled" : "❌ Disabled"
                                        color: SettingsManager.ethernet_isOn ? "green" : "red"
                                        font.pixelSize: 15
                                    }

                                    Text { color: "black"; text: "Subnet:" }
                                    Text { color: "black"; text: SettingsManager.ethernet_ipOne + "." + SettingsManager.ethernet_ipTwo + "." + SettingsManager.ethernet_ipThree + ".x" }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10 * theme.scaleWidth

                        Comp.TitleFrame {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 4

                                Row {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: qsTr("Protocol")
                                        font.bold: true
                                        font.pixelSize: 15
                                        width: 100
                                    }
                                    Text {
                                        text: qsTr("Description")
                                        font.bold: true
                                        font.pixelSize: 15
                                        width: 200
                                    }
                                    Text {
                                        text: qsTr("Source")
                                        font.bold: true
                                        font.pixelSize: 15
                                        width: 150
                                    }
                                    Text {
                                        text: qsTr("Frequency")
                                        font.bold: true
                                        font.pixelSize: 15
                                        width: 80
                                    }
                                }

                                ListView {
                                    id: protocolList
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: AgIOService.activeProtocols

                                    delegate: Row {
                                        spacing: 8
                                        width: protocolList.width

                                        Text {
                                            text: modelData.id
                                            font.pixelSize: 15
                                            font.bold: modelData.id.startsWith("PGN")
                                            width: 100
                                        }
                                        Text {
                                            text: modelData.description
                                            font.pixelSize: 15
                                            color: "gray"
                                            width: 200
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.source
                                            font.pixelSize: 15
                                            color: {
                                                if (modelData.source.startsWith("UDP")) return "green"
                                                if (modelData.source.startsWith("Serial")) return "orange"
                                                return "red"
                                            }
                                            width: 150
                                        }
                                        Text {
                                            text: Math.round(modelData.frequency * 10) / 10 + " Hz"
                                            font.pixelSize: 15
                                            width: 80
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
