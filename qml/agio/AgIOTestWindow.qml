// Test Window pour valider les connexions AgIOService
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG

ApplicationWindow {
    id: testWindow
    width: 600
    height: 800
    title: "AgIOService Test & Validation"
    visible: true
    
    property bool serviceAvailable: typeof AgIOService !== 'undefined'
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 10
        
        ColumnLayout {
            width: parent.width - 20
            spacing: 15
            
            // Header
            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: serviceAvailable ? "lightgreen" : "lightcoral"
                
                Text {
                    anchors.centerIn: parent
                    text: serviceAvailable ? "✅ AgIOService Available" : "❌ AgIOService Not Available"
                    font.bold: true
                    font.pointSize: 16
                }
            }
            
            // Singleton Status Section
            GroupBox {
                title: "🔧 Singleton Status"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    Text { text: serviceAvailable ? "AgIOService singleton loaded successfully" : "AgIOService singleton failed to load" }
                    Text { text: "Service type: " + (serviceAvailable ? typeof AgIOService : "undefined") }
                    
                    Button {
                        text: "Test Service Methods"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                console.log("Testing AgIOService methods...")
                                AgIOService.testThreadCommunication()
                                AgIOService.updateGPSStatus()
                            }
                        }
                    }
                }
            }
            
            // GPS Data Section
            GroupBox {
                title: "📍 GPS Data (Real-time)"
                Layout.fillWidth: true
                
                GridLayout {
                    columns: 2
                    width: parent.width
                    
                    Text { text: "Latitude:" }
                    Text { text: serviceAvailable ? Backend.fixFrame.latitude.toFixed(6) : "N/A" }

                    Text { text: "Longitude:" }
                    Text { text: serviceAvailable ? Backend.fixFrame.longitude.toFixed(6) : "N/A" }

                    Text { text: "Heading:" }
                    Text { text: serviceAvailable ? Backend.fixFrame.heading.toFixed(2) + "°" : "N/A" }

                    Text { text: "Speed:" }
                    Text { text: serviceAvailable ? VehicleInterface.avgSpeed.toFixed(1) + " km/h" : "N/A" }
                    
                    Text { text: "GPS Connected:" }
                    Text { 
                        text: serviceAvailable ? (AgIOService.gpsConnected ? "✅ Connected" : "❌ Disconnected") : "N/A"
                        color: serviceAvailable ? (AgIOService.gpsConnected ? "green" : "red") : "gray"
                    }
                }
            }
            
            // Connection Status Section
            GroupBox {
                title: "🌐 Connection Status"
                Layout.fillWidth: true
                
                GridLayout {
                    columns: 2
                    width: parent.width
                    
                    Text { text: "NTRIP Connected:" }
                    Text { 
                        text: serviceAvailable ? (AgIOService.ntripConnected ? "✅ Connected" : "❌ Disconnected") : "N/A"
                        color: serviceAvailable ? (AgIOService.ntripConnected ? "green" : "red") : "gray"
                    }
                    
                    Text { text: "NTRIP Status:" }
                    Text { text: serviceAvailable ? AgIOService.ntripStatus : "N/A" }
                    
                    Text { text: "Ethernet Connected:" }
                    Text { 
                        text: serviceAvailable ? (AgIOService.ethernetConnected ? "✅ Connected" : "❌ Disconnected") : "N/A"
                        color: serviceAvailable ? (AgIOService.ethernetConnected ? "green" : "red") : "gray"
                    }
                    
                    Text { text: "Bluetooth Connected:" }
                    Text { 
                        text: serviceAvailable ? (AgIOService.bluetoothConnected ? "✅ Connected" : "❌ Disconnected") : "N/A"
                        color: serviceAvailable ? (AgIOService.bluetoothConnected ? "green" : "red") : "gray"
                    }
                }
            }
            
            // Settings Test Section
            GroupBox {
                title: "⚙️ Settings Test"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    Text { text: "NTRIP URL: " + (serviceAvailable ? SettingsManager.setNTRIP_url : "N/A") }
                    Text { text: "NTRIP Mount: " + (serviceAvailable ? SettingsManager.setNTRIP_mount : "N/A") }
                    Text { text: "GPS Serial Port: " + (serviceAvailable ? SettingsManager.setGnss_SerialPort : "N/A") }
                    Text { text: "GPS Baud Rate: " + (serviceAvailable ? SettingsManager.setGnss_BaudRate : "N/A") }
                    Text { text: "UDP IP: " + (serviceAvailable ? SettingsManager.ethernet_ipOne + "." + SettingsManager.ethernet_ipTwo + "." + SettingsManager.ethernet_ipThree + ".xxx" : "N/A") }
                }
            }
            
            // Enhanced UI Properties (Phase 4.5.4)
            GroupBox {
                title: "📊 Enhanced UI Status (Phase 4.5.4)"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    Text { text: "GPS Status Text: " + (serviceAvailable ? SettingsManager.gpsStatusText : "N/A") }
                    Text { text: "Module Status Text: " + (serviceAvailable ? SettingsManager.moduleStatusText : "N/A") }
                    Text { text: "Serial Status Text: " + (serviceAvailable ? SettingsManager.serialStatusText : "N/A") }
                    Text { text: "NTRIP Status Text: " + (serviceAvailable ? SettingsManager.ntripStatusText : "N/A") }
                    
                    Text { 
                        text: "Error Dialog: " + (serviceAvailable ? (SettingsManager.showErrorDialog ? "⚠️ Yes" : "✅ No") : "N/A")
                        color: serviceAvailable ? (SettingsManager.showErrorDialog ? "orange" : "green") : "gray"
                    }
                    
                    Text { text: "Last Error: " + (serviceAvailable ? SettingsManager.lastErrorMessage : "N/A") }
                    
                    Text { text: "Discovered Modules: " + (serviceAvailable ? AgIOService.discoveredModules.length : "N/A") }
                }
            }
            
            // Control Buttons Section
            GroupBox {
                title: "🎛️ Control Actions"
                Layout.fillWidth: true
                
                RowLayout {
                    width: parent.width
                    
                    Button {
                        text: "Start Communication"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                AgIOService.startCommunication()
                                console.log("Started AgIOService communication")
                            }
                        }
                    }
                    
                    Button {
                        text: "Stop Communication"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                AgIOService.stopCommunication()
                                console.log("Stopped AgIOService communication")
                            }
                        }
                    }
                    
                    Button {
                        text: "Scan Modules"
                        enabled: serviceAvailable
                        onClicked: {
                            if (serviceAvailable) {
                                SettingsManager.startModuleDiscovery()
                                console.log("Started module discovery")
                            }
                        }
                    }
                }
            }
            
            // Manual Module Test
            GroupBox {
                title: "🔌 Manual Module Test"
                Layout.fillWidth: true
                
                ColumnLayout {
                    width: parent.width
                    
                    RowLayout {
                        TextField {
                            id: moduleIPField
                            text: "192.168.1.126"
                            placeholderText: "Module IP address"
                        }
                        
                        Button {
                            text: "Ping Module"
                            enabled: serviceAvailable
                            onClicked: {
                                if (serviceAvailable && moduleIPField.text) {
                                    // Si la méthode existe dans UDPWorker
                                    console.log("Pinging module:", moduleIPField.text)
                                    // SettingsManager.pingModule(moduleIPField.text) // Pourrait être ajouté
                                }
                            }
                        }
                    }
                }
            }
            
            // Real-time Update Counter
            Rectangle {
                Layout.fillWidth: true
                height: 30
                color: "lightblue"
                
                Text {
                    anchors.centerIn: parent
                    text: "Updates: " + updateCounter + " (every second)"
                    
                    property int updateCounter: 0
                    
                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        onTriggered: parent.updateCounter++
                    }
                }
            }
        }
    }
}
