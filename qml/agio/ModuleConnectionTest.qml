// Interface de test pour connexions modules via AgIOService
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

import "../components" as Comp

ApplicationWindow {
    id: moduleTestWindow
    objectName: "moduleConnectionTest"
    width: 800
    height: 900
    title: "AgIOService Module Connection Test"
    
    property bool serviceAvailable: typeof AgIOService !== 'undefined'
    property int testCounter: 0
    property string lastModuleResponse: ""

    // Qt 6.8 QProperty + BINDABLE: Interface property for setProperty() compatibility
    property var agioDiscoveredModules: serviceAvailable ? AgIOService.discoveredModules : []
    property var discoveredModules: agioDiscoveredModules
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 10
        
        ColumnLayout {
            width: parent.width - 20
            spacing: 10
            
            // Header Status
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: serviceAvailable ? "lightgreen" : "lightcoral"
                
                ColumnLayout {
                    anchors.centerIn: parent
                    
                    Text {
                        text: serviceAvailable ? "🔗 AgIOService Module Tester" : "❌ AgIOService Not Available"
                        font.bold: true
                        font.pointSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: serviceAvailable ? "Communication: " + (AgIOService.ethernetConnected ? "✅ Active" : "⏳ Standby") : ""
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
            
            // Quick Test Panel
            GroupBox {
                title: "🚀 Quick Module Test"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        TextField {
                            id: testModuleIP
                            text: "192.168.1.126"
                            placeholderText: "Module IP (e.g., 192.168.1.126)"
                            Layout.fillWidth: true
                        }
                        
                        Button {
                            text: "🔍 Test Connection"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable && testModuleIP.text) {
                                    testCounter++
                                    console.log("Testing connection to:", testModuleIP.text)
                                    
                                    testResultText.text = `Test ${testCounter}: Waking up modules and scanning for ${testModuleIP.text}...`

                                    // 1. Wake up dormant modules first (IMPORTANT!)
                                    AgIOService.wakeUpModules("192.168.1.255")

                                    // 2. Wait a bit then start discovery
                                    wakeupTimer.start()
                                }
                            }
                        }
                        
                        Button {
                            text: "🔊 Wake Up Modules"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable) {
                                    AgIOService.wakeUpModules("192.168.1.255")
                                    testResultText.text = "📡 Broadcasting wakeup signals to dormant modules..."
                                }
                            }
                        }
                    }
                    
                    Text {
                        id: testResultText
                        text: "Ready to test module connections"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }
                }
            }
            
            // Module Discovery Panel
            GroupBox {
                title: "🔍 Module Discovery & Status"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    RowLayout {
                        Button {
                            text: "Start Discovery"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable) {
                                    AgIOService.startModuleDiscovery()
                                    discoveryStatus.text = "🔄 Discovering modules..."
                                }
                            }
                        }

                        Button {
                            text: "Stop Discovery"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable) {
                                    AgIOService.stopModuleDiscovery()
                                    discoveryStatus.text = "⏹️ Discovery stopped"
                                }
                            }
                        }

                        Button {
                            text: "Refresh Status"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable) {
                                    AgIOService.updateModuleStatus()
                                    // Forcer mise à jour de la liste via interface property
                                    agioDiscoveredModules = AgIOService.discoveredModules
                                }
                            }
                        }
                    }
                    
                    Text {
                        id: discoveryStatus
                        text: serviceAvailable ? "Ready for module discovery" : "Service unavailable"
                        Layout.fillWidth: true
                    }
                    
                    // Liste des modules découverts
                    Text {
                        text: `Discovered Modules (${discoveredModules.length}):`
                        font.bold: true
                    }
                    
                    Repeater {
                        model: discoveredModules
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 60
                            color: index % 2 ? "lightgray" : "white"
                            border.color: "gray"
                            border.width: 1
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 5
                                
                                Column {
                                    Text {
                                        text: `📍 IP: ${modelData.ip || 'Unknown'}`
                                        font.bold: true
                                    }
                                    Text {
                                        text: `Type: ${modelData.type || 'Unknown'}`
                                        color: "darkblue"
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Column {
                                    Text {
                                        text: `Status: ${modelData.status || 'Unknown'}`
                                        color: modelData.status === 'Connected' ? "green" : "red"
                                        font.bold: true
                                    }
                                    Text {
                                        text: `Last Seen: ${modelData.lastSeen || 'Never'}`
                                        font.pointSize: 8
                                    }
                                }
                            }
                        }
                    }
                    
                    // Module status text from AgIOService
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        color: "lightyellow"
                        border.color: "orange"

                        Text {
                            anchors.centerIn: parent
                            text: serviceAvailable ? AgIOService.moduleStatusText : "Service unavailable"
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
            
            // Communication Test Panel
            GroupBox {
                title: "💬 Communication Test"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    Text {
                        text: "Test communication with specific module types:"
                        font.bold: true
                    }
                    
                    GridLayout {
                        columns: 2
                        Layout.fillWidth: true
                        
                        Button {
                            text: "🎛️ Test AutoSteer (126)"
                            enabled: serviceAvailable
                            onClicked: {
                                console.log("Testing AutoSteer module communication")
                                // Test spécifique pour module AutoSteer
                                if (serviceAvailable) {
                                    // Simuler test AutoSteer
                                    commTestResults.text += "Testing AutoSteer module (PGN 126)...\n"
                                }
                            }
                        }
                        
                        Button {
                            text: "🚜 Test Machine (123)"
                            enabled: serviceAvailable
                            onClicked: {
                                console.log("Testing Machine module communication")
                                if (serviceAvailable) {
                                    commTestResults.text += "Testing Machine module (PGN 123)...\n"
                                }
                            }
                        }
                        
                        Button {
                            text: "🧭 Test IMU (121)"
                            enabled: serviceAvailable
                            onClicked: {
                                console.log("Testing IMU module communication")
                                if (serviceAvailable) {
                                    commTestResults.text += "Testing IMU module (PGN 121)...\n"
                                }
                            }
                        }
                        
                        Button {
                            text: "📡 Test GPS Module"
                            enabled: serviceAvailable
                            onClicked: {
                                console.log("Testing GPS module communication")
                                if (serviceAvailable) {
                                    commTestResults.text += "Testing GPS module communication...\n"
                                }
                            }
                        }
                    }
                    
                    ScrollView {
                        Layout.fillWidth: true
                        height: 100
                        
                        TextArea {
                            id: commTestResults
                            text: "Communication test results will appear here...\n"
                            readOnly: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
            
            // Network Configuration Panel
            GroupBox {
                title: "🌐 Network Configuration"
                Layout.fillWidth: true
                
                GridLayout {
                    columns: 2
                    width: parent.width
                    
                    Text { text: "Network Settings:" }
                    Text { text: "" }
                    
                    Text { text: "UDP IP Range:" }
                    Text { 
                        text: serviceAvailable ? 
                              `${SettingsManager.ethernet_ipOne}.${SettingsManager.ethernet_ipTwo}.${SettingsManager.ethernet_ipThree}.xxx` : 
                              "N/A"
                    }
                    
                    Text { text: "Listen Port:" }
                    Text { text: "9999 (AgIOService)" }
                    
                    Text { text: "Send Port:" }
                    Text { text: "8888 (Modules)" }
                    
                    Text { text: "Ethernet Status:" }
                    Text {
                        text: serviceAvailable ?
                              (AgIOService.ethernetConnected ? "✅ Connected" : "❌ Disconnected") :
                              "N/A"
                        color: serviceAvailable ?
                               (AgIOService.ethernetConnected ? "green" : "red") :
                               "gray"
                    }
                }
            }
            
            // Real-time Monitoring Panel
            GroupBox {
                title: "📊 Real-time Monitoring"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    Text {
                        text: `Monitoring Active - Updates: ${monitorCounter}`
                        font.bold: true
                    }
                    
                    GridLayout {
                        columns: 2
                        Layout.fillWidth: true
                        
                        Text { text: "GPS Status:" }
                        Text {
                            text: serviceAvailable ? AgIOService.gpsStatusText : "N/A"
                        }
                        
                        Text { text: "Serial Status:" }
                        Text {
                            text: serviceAvailable ? AgIOService.serialStatusText : "N/A"
                        }
                        
                        Text { text: "Last Error:" }
                        Text {
                            text: serviceAvailable ? AgIOService.lastErrorMessage : "N/A"
                            color: serviceAvailable && AgIOService.lastErrorMessage !== "" ? "red" : "green"
                        }

                        Text { text: "Error Dialog:" }
                        Text {
                            text: serviceAvailable ?
                                  (AgIOService.showErrorDialog ? "⚠️ Active" : "✅ None") :
                                  "N/A"
                            color: serviceAvailable ?
                                   (AgIOService.showErrorDialog ? "red" : "green") :
                                   "gray"
                        }
                    }
                }
            }
            
            // Control Actions Panel
            GroupBox {
                title: "🎛️ Control Actions"
                Layout.fillWidth: true
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    Button {
                        text: "▶️ Start Communication"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                AgIOService.startCommunication()
                                console.log("AgIOService communication started")
                            }
                        }
                    }
                    
                    Button {
                        text: "⏹️ Stop Communication"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                AgIOService.stopCommunication()
                                console.log("AgIOService communication stopped")
                            }
                        }
                    }
                    
                    Button {
                        text: "🔄 Update Status"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                // TODO: These methods don't exist in AgIOService
                                // AgIOService.updateGPSStatus()
                                // AgIOService.updateModuleStatus()
                                // AgIOService.updateSerialStatus()
                            }
                        }
                    }
                    
                    Button {
                        text: "🧹 Clear Errors"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                AgIOService.clearErrorDialog()
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Real-time monitoring timer
    property int monitorCounter: 0
    
    Timer {
        interval: 2000 // Every 2 seconds
        running: serviceAvailable
        repeat: true
        onTriggered: {
            monitorCounter++
            
            // Update discovered modules list via interface property
            if (serviceAvailable) {
                agioDiscoveredModules = AgIOService.discoveredModules
            }
        }
    }
    
    // Timer pour séquence réveil → discovery
    Timer {
        id: wakeupTimer
        interval: 3000 // 3 secondes après réveil
        running: false
        repeat: false
        onTriggered: {
            if (serviceAvailable) {
                AgIOService.startModuleDiscovery()
                testResultText.text += "\n🔍 Starting module discovery after wakeup..."
            }
        }
    }
}