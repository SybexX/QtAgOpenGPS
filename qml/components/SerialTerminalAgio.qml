import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Controls.Fusion
import QtQuick.Layouts
//// 

import ".."
import "../components"
Drawer{

    width: 270 * theme.scaleWidth
    height: mainWindow.height
    modal: true
    id: serialTerminalAgio

    property string portBaud: ""
    property string portName: ""
    property bool saveConfig: false

    // Debug/Monitoring properties
    property bool enableMonitoring: false    // Enable real-time monitoring mode
    property string moduleType: "GPS"        // Module type (GPS, IMU, Steer, Machine, Blockage)

    onVisibleChanged: {
        if (visible) {
            load_settings()
        } else if (enableMonitoring) {
            // Reset monitoring data when closing window
            terminal.clear()
        }
    }

    // Error message dialog for port validation
    Rectangle {
        id: errorMessage
        visible: false
        anchors.centerIn: parent
        width: 300 * theme.scaleWidth
        height: 150 * theme.scaleHeight
        color: aogInterface.backgroundColor
        border.width: 2
        border.color: "#FF6B6B"
        radius: 8
        z: 1000

        property string message: ""

        function showError(msg) {
            message = msg;
            visible = true;
            errorTimer.restart();
        }

        Column {
            anchors.centerIn: parent
            spacing: 10
            width: parent.width - 20

            Text {
                text: "⚠️ Port Error"
                color: "#FF6B6B"
                font.bold: true
                font.pixelSize: 16
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: errorMessage.message
                color: aogInterface.textColor
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 10
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                text: "OK"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: errorMessage.visible = false
            }
        }

        Timer {
            id: errorTimer
            interval: 5000
            onTriggered: errorMessage.visible = false
        }
    }

    // CONNECTION MONITORING - Listen to AgIOService serial data signals
    Connections {
        target: AgIOService  // Connect to AgIOService in main thread
        enabled: enableMonitoring && visible

        // Monitor GPS data (NMEA sentences)
        function onGpsDataReceived(data) {
            if (moduleType === "GPS") {
                var timestamp = new Date().toLocaleTimeString();
                terminal.append("[" + timestamp + "] RX: " + data);
                blinkAnimation.start();

                // Real-time content validation: verify this is actually GPS data
                if (connectBtn.isConnected && enableMonitoring) {
                    // If we don't see GPS patterns in the data, warn the user
                    if (!data.includes("$GP") && !data.includes("$GN") &&
                        !data.includes("PANDA") && !data.includes("PAOGI")) {
                        console.warn("⚠️ GPS module not receiving expected NMEA data");
                    }
                }
            }
        }

        // Monitor IMU data (hex format)
        function onImuDataReceived(data) {
            if (moduleType === "IMU") {
                var timestamp = new Date().toLocaleTimeString();
                terminal.append("[" + timestamp + "] IMU: " + data);
                blinkAnimation.start();
            }
        }

        // Monitor Autosteer data (hex format)
        function onAutosteerDataReceived(data) {
            if (moduleType === "Steer") {
                var timestamp = new Date().toLocaleTimeString();
                terminal.append("[" + timestamp + "] RX: " + data);
                blinkAnimation.start();
            }
        }

        // Monitor Machine data (hex format)
        function onMachineDataReceived(data) {
            if (moduleType === "Machine") {
                var timestamp = new Date().toLocaleTimeString();
                terminal.append("[" + timestamp + "] RX: " + data);
                blinkAnimation.start();
            }
        }

        // Monitor Blockage data (if available in future)
        function onBlockageDataReceived(data) {
            if (moduleType === "Blockage") {
                var timestamp = new Date().toLocaleTimeString();
                terminal.append("[" + timestamp + "] RX: " + data);
                blinkAnimation.start();
            }
        }
    }

    // Error handling connections for port validation
    Connections {
        target: AgIOService

        function onPortAlreadyInUse(portName, ownerModule) {
            console.error("❌ Port conflict:", portName, "owned by", ownerModule);
            errorMessage.showError("Port " + portName + " is already in use by " + ownerModule + ". Please choose a different port.");
        }

        function onModuleValidationFailed(portName, expectedModule, detectedModule) {
            console.error("❌ Module validation failed:", portName, "expected:", expectedModule, "detected:", detectedModule);
            var msg = "⚠️ WRONG MODULE TYPE DETECTED!\n\n";
            msg += "Port " + portName + " does not appear to be a " + expectedModule + " module.";
            if (detectedModule.length > 0) {
                msg += "\n\nDetected: " + detectedModule + " data";
            }
            msg += "\n\n🔌 Auto-disconnected to prevent errors.";
            msg += "\n\nPlease check your connection and use the correct port.";
            errorMessage.showError(msg);
        }

        function onModuleTypeDetected(portName, detectedModuleType, dataPattern) {
            console.log("🔍 Module detected on", portName + ":", detectedModuleType, "pattern:", dataPattern);

            // If we're monitoring this port and detect wrong module type
            if (connectBtn.isConnected && serialPorts.currentText === portName) {
                if (detectedModuleType === "GPS" && moduleType !== "GPS") {
                    errorMessage.showError("⚠️ WARNING: Detected GPS data on " + portName + " but you connected it as " + moduleType + ". This port appears to be a GPS module.");
                }
                else if (detectedModuleType === "Module" && moduleType === "GPS") {
                    errorMessage.showError("⚠️ WARNING: Detected AgIO module data on " + portName + " but you connected it as GPS. This port appears to be an IMU/Steer/Machine module.");
                }
                else if (detectedModuleType === "Computer") {
                    errorMessage.showError("⚠️ WARNING: Detected computer/system data on " + portName + ". This doesn't appear to be a " + moduleType + " module - check if this is the correct port.");
                }
            }
        }
    }

    function load_settings() {

        if (portBaud === "9600") baudRate.currentIndex = 0
        else if (portBaud === "38400") baudRate.currentIndex = 1
        else if (portBaud === "115200") baudRate.currentIndex = 2
        else if (portBaud === "460800") baudRate.currentIndex = 3
        else if (portBaud === "921600") baudRate.currentIndex = 4
    }

    function save_settings() {
    portBaud = baudRate.currentText
    portName = serialPorts.currentText
    }

    Rectangle {
        id: gnssWindow
        visible: true
        color: aogInterface.backgroundColor
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        border.width: 2
        border.color: aogInterface.blackDayWhiteNight

        // Connections {
        //     target: serialTerminal
        //
        //     function onGetData(data) {
        //         terminal.append("<"+data);
        //     }
        // }

        // Row {
        //     spacing: 40

        //     Column {
        //         y: 40

        //         Flickable {
        //             clip: true
        //             width: Screen.width*0.8
        //             height: Screen.height*0.6

        //         TextArea.flickable: TextArea {

        //             id: terminal
        //             wrapMode: TextArea.Wrap
        //             placeholderText:  "Waiting..."
        //             width: Screen.width*0.8
        //             height: Screen.height*0.1
        //         }
        //         }

        //         TextField {
        //             id: dataToSend
        //             placeholderText:  "Data to send..."
        //             width: Screen.width*0.8
        //         }

        //         Button {
        //             id: sendBtn
        //             text: qsTr("Send")
        //             anchors.top: connectBtn.anchors.top
        //             onClicked: {
        //                 //terminal.append(">"+dataToSend.text)
        //                 //serialTerminal.writeToSerialPortSlot(dataToSend.text+"\r\n")
        //             }
        //         }
        //     }

        ColumnLayout {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10 * theme.scaleHeight
            spacing: 5

                    // Terminal with monitoring status indicator
                    Rectangle {
                        width: 260 * theme.scaleWidth
                        height: 170 * theme.scaleHeight
                        color: aogInterface.backgroundColor
                        border.width: 1
                        border.color: aogInterface.blackDayWhiteNight
                        radius: 4

                        // Data monitoring indicator
                        Rectangle {
                            id: dataIndicator
                            width: 8
                            height: 8
                            radius: 4
                            color: enableMonitoring && connectBtn.isConnected ? "#4CAF50" : "#757575"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 4

                            // Blinking animation when data arrives
                            SequentialAnimation {
                                id: blinkAnimation
                                ColorAnimation {
                                    target: dataIndicator
                                    property: "color"
                                    to: "#FFC107"
                                    duration: 100
                                }
                                ColorAnimation {
                                    target: dataIndicator
                                    property: "color"
                                    to: enableMonitoring && connectBtn.isConnected ? "#4CAF50" : "#757575"
                                    duration: 100
                                }
                            }
                        }

                        Flickable {
                            anchors.fill: parent
                            anchors.margins: 4
                            clip: true
                            contentWidth: terminal.contentWidth
                            contentHeight: terminal.contentHeight

                            TextArea.flickable: TextArea {
                                id: terminal
                                wrapMode: TextArea.Wrap
                                placeholderText: enableMonitoring ? "Monitoring " + moduleType + " data..." : "Monitoring disabled"
                                selectByMouse: true
                                readOnly: true
                                font.family: "Consolas, Monaco, monospace"
                                font.pixelSize: 10
                                background: Rectangle {
                                    color: "transparent"
                                }
                            }
                        }
                    }

                // Send Data section (Phase 6.1 - SerialMonitor functionality)
                TextField {
                    id: dataToSend
                    placeholderText: qsTr("Data to send...")
                    Layout.maximumWidth: 260 * theme.scaleWidth
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    enabled: connectBtn.isConnected
                }

                Button {
                    id: sendBtn
                    text: qsTr("Send")
                    Layout.alignment: Qt.AlignHCenter
                    enabled: connectBtn.isConnected && dataToSend.text.length > 0
                    onClicked: {
                        if (dataToSend.text.length > 0) {
                            var textToSend = dataToSend.text;
                            var timestamp = new Date().toLocaleTimeString();
                            SettingsManager.writeToSerialPort(moduleType, textToSend);
                            terminal.append("[" + timestamp + "] TX: " + textToSend);
                            console.log("📤 Data sent to", moduleType + ":", textToSend);
                            dataToSend.clear();
                            blinkAnimation.start();
                        }
                    }
                }

                Text {
                    text: qsTr("Serial port: " ) + portName
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 18
                }

                ComboBox {
                    id: serialPorts
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    Layout.alignment: Qt.AlignHCenter

                    property var availablePorts: []

                    // Initialize model with filtered ports for this module type
                    Component.onCompleted: {
                        refreshPortList();
                    }

                    // Refresh port list (excluding ports in use by other modules)
                    function refreshPortList() {
                        availablePorts = AgIOService.getAvailableSerialPortsForModule(moduleType);
                        model = availablePorts;
                        console.log("🔍 Available ports for", moduleType + ":", availablePorts);
                    }

                    // Refresh when opening the drawer
                    Connections {
                        target: serialTerminalAgio
                        function onVisibleChanged() {
                            if (serialTerminalAgio.visible) {
                                serialPorts.refreshPortList();
                            }
                        }
                    }
                }

                Text {
                    text: qsTr("Baud: ")
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 18
                }

                ComboBox {
                    id: baudRate
                    Layout.maximumWidth: 200 * theme.scaleWidth
                    Layout.fillWidth : true
                    Layout.alignment: Qt.AlignHCenter
                    //model: baudsModel
                    model: ["9600", "38400", "115200", "460800", "921600"]
                }

                Button {
                    id: connectBtn

                    // ✅ Phase 6.0.21.4: readonly property to prevent binding loop
                    readonly property bool isConnected: {
                        if (moduleType === "GPS") return AgIOService.gpsPortConnected
                        else if (moduleType === "IMU") return AgIOService.imuConnected
                        else if (moduleType === "Steer") return AgIOService.steerConnected
                        else if (moduleType === "Machine") return AgIOService.machineConnected
                        else if (moduleType === "Blockage") return AgIOService.blockageConnected
                        return false
                    }

                    text: connectBtn.isConnected ? qsTr("Disconnect") : qsTr("Connect")
                    Layout.alignment: Qt.AlignHCenter

                    // Dynamic colors based on connection state
                    background: Rectangle {
                        color: connectBtn.isConnected ? "#4CAF50" : "#F44336"  // Green if connected, red if disconnected
                        radius: 4
                        border.width: 1
                        border.color: connectBtn.isConnected ? "#388E3C" : "#D32F2F"
                    }

                    // White text for contrast
                    contentItem: Text {
                        text: connectBtn.text
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.centerIn: parent
                    }

                    onClicked: {
                        save_settings();
                        saveConfig = true;

                        if (!connectBtn.isConnected) {
                            // Check if port is available and validate module type
                            var selectedPort = serialPorts.currentText;
                            var selectedBaud = parseInt(baudRate.currentText);

                            // Check if port is already in use
                            var portOwner = SettingsManager.getPortOwner(selectedPort);
                            if (portOwner.length > 0) {
                                console.error("❌ Port", selectedPort, "already in use by", portOwner);
                                errorMessage.showError("Port " + selectedPort + " is already in use by " + portOwner + ". Please choose a different port.");
                                return;
                            }

                            // Validate module type (basic check for now)
                            var validationPassed = SettingsManager.validateModuleType(selectedPort, moduleType, selectedBaud);
                            if (!validationPassed) {
                                console.error("❌ Module validation failed for", selectedPort);
                                errorMessage.showError("Module validation failed for " + selectedPort + ". This port may not be compatible with " + moduleType + " module.");
                                return;
                            }

                            // Connect serial port
                            var success = SettingsManager.openSerialPort(moduleType, selectedPort, selectedBaud);
                            if (success) {
                                console.log("🔗 SerialPort connected:", moduleType, selectedPort, selectedBaud);
                                // Refresh port list for other modules
                                serialPorts.refreshPortList();
                            } else {
                                console.error("❌ SerialPort connection failed:", moduleType);
                                errorMessage.showError("Failed to connect to " + selectedPort + ". Please check the port and try again.");
                            }
                        } else {
                            // Disconnect serial port
                            SettingsManager.closeSerialPort(moduleType);
                            console.log("🔌 SerialPort disconnected:", moduleType);
                            // Refresh port list to make this port available again
                            serialPorts.refreshPortList();
                        }
                    }
                }
            }
        }
    }

