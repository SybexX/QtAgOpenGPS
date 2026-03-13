// Test QML file to validate AgIOService singleton access
import QtQuick 2.15
import "agio"

Rectangle {
    id: agioTestWindow
    width: 400
    height: 300
    color: "#2E3440"

    visible: false // Hidden by default

    // Qt 6.8 QProperty + BINDABLE: Use AgIOInterface instead of direct AgIOService access
    AgIOInterface {
        id: agioInterface
    }
    
    Text {
        id: titleText
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        text: "AgIO Service Test"
        color: "white"
        font.pixelSize: 18
        font.bold: true
    }
    
    Column {
        anchors.centerIn: parent
        spacing: 10
        
        Text {
            text: "Service Available: " + (typeof AgIOService !== 'undefined' ? "YES" : "NO")
            color: typeof AgIOService !== 'undefined' ? "#A3BE8C" : "#BF616A"
            font.pixelSize: 14
        }
        
        Text {
            text: "GPS Connected: " + (typeof AgIOService !== 'undefined' ? agioInterface.gpsConnected : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "Latitude: " + (typeof AgIOService !== 'undefined' ? agioInterface.latitude.toFixed(6) : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "Longitude: " + (typeof AgIOService !== 'undefined' ? agioInterface.longitude.toFixed(6) : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "Heading: " + (typeof AgIOService !== 'undefined' ? agioInterface.heading.toFixed(1) + "°" : "N/A")  // Rectangle Pattern: direct property access
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "IMU Roll: " + (typeof AgIOService !== 'undefined' ? agioInterface.imuRoll.toFixed(1) + "°" : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "Vehicle XY: " + (typeof AgIOService !== 'undefined' ? "N/A - Use AOGInterface" : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Text {
            text: "NTRIP Status: " + (typeof AgIOService !== 'undefined' ? agioInterface.ntripStatusText : "N/A")
            color: "white"
            font.pixelSize: 14
        }
        
        Rectangle {
            width: 200
            height: 40
            color: "#5E81AC"
            radius: 5
            
            Text {
                anchors.centerIn: parent
                text: "Test Thread Communication"
                color: "white"
                font.pixelSize: 12
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (typeof AgIOService !== 'undefined') {
                        AgIOService.testThreadCommunication();
                        console.log("Thread communication test triggered");
                    }
                }
            }
        }
        
        Rectangle {
            width: 200
            height: 40
            color: "#88C0D0"
            radius: 5
            
            Text {
                anchors.centerIn: parent
                text: "Start Communication"
                color: "white"
                font.pixelSize: 12
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (typeof AgIOService !== 'undefined') {
                        AgIOService.startCommunication();
                        console.log("Communication started");
                    }
                }
            }
        }
    }
    
    // Real-time updates
    Connections {
        target: typeof AgIOService !== 'undefined' ? AgIOService : null
        function onGpsDataChanged() {
            console.log("GPS data updated:", agioInterface.latitude, agioInterface.longitude)
        }
        function onGpsStatusChanged() {
            console.log("GPS status changed:", agioInterface.gpsConnected)
        }
        function onImuDataChanged() {
            console.log("IMU data updated:", agioInterface.imuRoll)
        }
    }
    
    Component.onCompleted: {
        console.log("=== AGIO TEST COMPONENT LOADED ===")
        console.log("AgIOService available:", typeof AgIOService !== 'undefined')
        
        if (typeof AgIOService !== 'undefined') {
            console.log("AgIOService properties:")
            console.log("  GPS Connected:", agioInterface.gpsConnected)
            console.log("  Current Thread Test:")
            AgIOService.testThreadCommunication()
        } else {
            console.log("❌ AgIOService NOT available in QML")
        }
        console.log("=== END AGIO TEST ===")
    }
}