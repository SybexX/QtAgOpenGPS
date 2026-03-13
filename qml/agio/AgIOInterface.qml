import QtQuick
import QtQuick.Controls.Fusion
// 
/* This type contains properties, signals, and functions to interface
   the C++ backend with the QML gui, while abstracting and limiting
   how much the C++ needs to know about the QML structure of the GUI,
   and how much the QML needs to know about the backends.

   This type also exposes the QSettings object for QML to use. However
   there are not change signals in the AgIOService object, so we'll provide
   a signal here to let the backend know QML touched a setting.  Also
   a javascript function here that C++ can call to let QML objects know
   something in AgIOService changed.

   MIGRATION NOTE: Phase 4.1 - Modified to use AgIOService direct properties
   for real-time data instead of static values.
*/




Item {
    id: agioInterfaceType

	// Connection status - direct AgIOService properties for real-time updates
	property bool ethernetConnected: false
	property bool ntripConnected: false
	property bool aogConnected: false // TODO: Add to AgIOService if needed
	property bool steerConnected: false // TODO: Map to SettingsManager.moduleConnected
	property bool gpsConnected: false
	property bool imuConnected: false // TODO: Add IMU connection status to AgIOService
	property bool machineConnected: false // TODO: Add machine connection status to AgIOService
    property bool blockageConnected: false // TODO: Add blockage connection status to AgIOService
    property bool bluetoothConnected: false

    // NTRIP status - direct AgIOService properties for real-time monitoring
    property int ntripStatus: 0
    property string ntripStatusText: ""
    property int tripBytes: 0 // TODO: Add totalNTRIPBytes to AgIOService
    property int ntripCounter: 0 // TODO: Add NTRIP message counter to AgIOService
    property int rawTripCount: 0 // TODO: Add raw NTRIP count to AgIOService


	// GPS/NMEA data - direct AgIOService properties for real-time updates
	property double latitude: 0.0
	property double longitude: 0.0
	property double altitude: 0 // TODO: Add altitude to AgIOService if available
	property double speed: 0.0
    property double gpsHeading: 0.0
    property double dualHeading: 0 // TODO: Add dual heading to AgIOService if needed
    property double imuHeading: 0.0
    property double imuRoll: 0.0
    property double imuPitch: 0.0
    property double age: 0.0
    property int hdop: 0 // TODO: Add HDOP to AgIOService if available
	property int quality: 0
	property int sats: 0
	property double yawrate: 0 // TODO: Add yaw rate to AgIOService if needed
    property double gpsHz: 0 // TODO: Add GPS frequency to AgIOService if needed
    property double nowHz: 0 // TODO: Add current Hz to AgIOService if needed

	property string gga: ""
	property string vtg: ""
	property string panda: ""
	property string paogi: ""
	property string hdt: ""
	property string avr: ""
	property string hpd: ""
	property string sxt: ""
    property string unknownSentence: ""

    property int nmeaError: 0 // triggers if altitude changes drastically--a sign of 2 separate nmea strings

    //these are signals that get sent to the backend
    // ✅ PHASE 6.0.20: AGIO SIGNALS MODERNIZED TO AgIOService DIRECT CALLS
    // REMOVED 10 LEGACY SIGNALS - All replaced by AgIOService methods:
    // - btnSendSubnet_clicked() → AgIOService.sendSubnet()
    // - ntripDebug(bool) → AgIOService.setNTRIPDebug(bool)
    // - setIPFromUrl(string) → AgIOService.setIPFromUrl(string)
    // - configureNTRIP() → AgIOService.configureNTRIP()
    // - btnUDPListenOnly_clicked(bool) → AgIOService.setUDPListenOnly(bool)
    // - bt_search(string) → AgIOService.bluetoothSearch(string)
    // - bt_kill() → AgIOService.bluetoothKill()
    // - bt_remove_device(string) → AgIOService.bluetoothRemoveDevice(string)
    // - bluetoothDebug(bool) → AgIOService.setBluetoothDebug(bool)
    // - startBluetoothDiscovery() → AgIOService.startBluetoothDiscovery()

    property string connectedBTDevices: ""


}

