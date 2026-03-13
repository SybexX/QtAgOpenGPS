#ifndef AGIOSERVICE_H
#define AGIOSERVICE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>  // Phase 6.0.24: Event-driven UDP in main thread
#include <QPointF>
#include <QQmlEngine>
#include <QtQml>
#include <QProperty> // Qt 6.8 QProperty + BINDABLE support
#include <QApplication>
#include <QWidget>
#include <QLoggingCategory>  // Phase 6.0.24: Logging categories for debug control
//#include "qmlutil.h"

// Phase 6.0.21: GPSWorker removed (useless thread - no I/O)
// Phase 6.0.24: UDPWorker removed (event-driven QUdpSocket in main thread)
#include "ctraffic.h"   // Phase 6.0.24: CTraffic extracted from udpworker.h
#include "ntripworker.h"
#include "serialworker.h"
#include "pgnparser.h"  // Phase 6.0.21: Centralized NMEA + PGN parser

// Phase 6.0.24: Logging category for AgIOService debug control
Q_DECLARE_LOGGING_CATEGORY(agioservice)

/**
 * @brief Real-time AgIO service for GPS, NTRIP, and hardware communication
 *
 * This class runs in the main/UI thREADto provide ZERO-latency access
 * to position data for OpenGL rendering and AutoSteer functionality.
 * I/O operations are handled by worker threads.
 *
 * ARCHITECTURE COMPLIANCE:
 * - Phase 1: Uses SettingsManager singleton (thread-safe)
 * - Phase 6.2.1: CTraffic monitoring integration
 * - Phase 6.3: NTRIP Advanced features ready
 * - Simulator Mode: Auto-disables all network communication
 */
class AgIOService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // PHASE 6.0.22.2: Module status tracking structure
    struct ModuleStatus {
        QString transport;         // "UDP" or "Serial"
        QString sourceID;          // IP address or COM port
        qint64 lastSeenMs = 0;     // Last data reception timestamp
        qint64 packetCount = 0;    // Total packets received
        double frequency = 0.0;    // Calculated Hz
        bool isActive = false;     // Active if data received within 2 seconds

        // Frequency calculation window (1 second)
        qint64 freqStartTime = 0;
        qint64 freqPacketCount = 0;

        // PHASE 6.0.22.12: Protocol ID is now the map key (no longer stored in structure)
        // Map key format: "$PANDA", "$GGA", "PGN211", "PGN214", etc.
    };

    // Phase 6.0.21: GPS/IMU data properties removed - moved to FormGPS (aog)
    // AgIOService only handles I/O and broadcasts via parsedDataReady() signal
    
    // GPS status - Qt 6.8 QProperty + BINDABLE
    Q_PROPERTY(bool gpsConnected READ gpsConnected WRITE setGpsConnected
               NOTIFY gpsConnectedChanged BINDABLE bindableGpsConnected)
    Q_PROPERTY(bool gpsPortConnected READ gpsPortConnected WRITE setGpsPortConnected
               NOTIFY gpsPortConnectedChanged BINDABLE bindableGpsPortConnected)
    Q_PROPERTY(int gpsQuality READ gpsQuality WRITE setGpsQuality
               NOTIFY gpsQualityChanged BINDABLE bindableGpsQuality)
    Q_PROPERTY(int satellites READ satellites WRITE setSatellites
               NOTIFY satellitesChanged BINDABLE bindableSatellites)

    // Connection status - Qt 6.8 QProperty + BINDABLE
    Q_PROPERTY(bool bluetoothConnected READ bluetoothConnected WRITE setBluetoothConnected
               NOTIFY bluetoothConnectedChanged BINDABLE bindableBluetoothConnected)
    Q_PROPERTY(QString bluetoothDevice READ bluetoothDevice WRITE setBluetoothDevice
               NOTIFY bluetoothDeviceChanged BINDABLE bindableBluetoothDevice)
    Q_PROPERTY(bool ethernetConnected READ ethernetConnected WRITE setEthernetConnected
               NOTIFY ethernetConnectedChanged BINDABLE bindableEthernetConnected)
    Q_PROPERTY(bool ntripConnected READ ntripConnected WRITE setNtripConnected
               NOTIFY ntripConnectedChanged BINDABLE bindableNtripConnected)

    // NTRIP status - Qt 6.8 QProperty + BINDABLE
    Q_PROPERTY(int ntripStatus READ ntripStatus WRITE setNtripStatus
               NOTIFY ntripStatusChanged BINDABLE bindableNtripStatus)
    Q_PROPERTY(QString ntripStatusText READ ntripStatusText WRITE setNtripStatusText
               NOTIFY ntripStatusTextChanged BINDABLE bindableNtripStatusText)
    Q_PROPERTY(int rawTripCount READ rawTripCount WRITE setRawTripCount
               NOTIFY rawTripCountChanged BINDABLE bindableRawTripCount)

    // Additional connection status for QML compatibility - Qt 6.8 QProperty + BINDABLE
    Q_PROPERTY(bool imuConnected READ imuConnected WRITE setImuConnected
               NOTIFY imuConnectedChanged BINDABLE bindableImuConnected)
    Q_PROPERTY(bool steerConnected READ steerConnected WRITE setSteerConnected
               NOTIFY steerConnectedChanged BINDABLE bindableSteerConnected)
    Q_PROPERTY(bool machineConnected READ machineConnected WRITE setMachineConnected
               NOTIFY machineConnectedChanged BINDABLE bindableMachineConnected)
    Q_PROPERTY(bool blockageConnected READ blockageConnected WRITE setBlockageConnected
               NOTIFY blockageConnectedChanged BINDABLE bindableBlockageConnected)
    Q_PROPERTY(bool rateControlConnected READ rateControlConnected WRITE setRateControlConnected
                   NOTIFY rateControlConnectedChanged BINDABLE bindableRateControlConnected)

    // PHASE 6.0.22.3: Module source tracking and frequency monitoring
    Q_PROPERTY(QString gpsSource READ gpsSource WRITE setGpsSource
               NOTIFY gpsSourceChanged BINDABLE bindableGpsSource)
    Q_PROPERTY(QString imuSource READ imuSource WRITE setImuSource
               NOTIFY imuSourceChanged BINDABLE bindableImuSource)
    Q_PROPERTY(QString steerSource READ steerSource WRITE setSteerSource
               NOTIFY steerSourceChanged BINDABLE bindableSteerSource)
    Q_PROPERTY(QString machineSource READ machineSource WRITE setMachineSource
               NOTIFY machineSourceChanged BINDABLE bindableMachineSource)
    Q_PROPERTY(double gpsFrequency READ gpsFrequency WRITE setGpsFrequency
               NOTIFY gpsFrequencyChanged BINDABLE bindableGpsFrequency)
    Q_PROPERTY(double imuFrequency READ imuFrequency WRITE setImuFrequency
               NOTIFY imuFrequencyChanged BINDABLE bindableImuFrequency)
    Q_PROPERTY(double steerFrequency READ steerFrequency WRITE setSteerFrequency
               NOTIFY steerFrequencyChanged BINDABLE bindableSteerFrequency)
    Q_PROPERTY(double machineFrequency READ machineFrequency WRITE setMachineFrequency
               NOTIFY machineFrequencyChanged BINDABLE bindableMachineFrequency)

    // PHASE 6.0.22.12: Dynamic protocol list with descriptions
    Q_PROPERTY(QVariantList activeProtocols READ activeProtocols NOTIFY activeProtocolsChanged)

    // Note: SerialWorker cannot be directly exposed to QML due to threading
    // QML access is provided through Q_INVOKABLE methods and signals

    // Configuration properties removed - use SettingsManager singleton directly
    // QML access: settingsManager.setNTRIP_url instead of agioService.setNTRIP_url

    // ✅ Phase 6.0.21.3: Initialization guard property for QML thread-safety
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

    // CTraffic is owned by AgIOService - read-only access for QML
    Q_PROPERTY(CTraffic* traffic READ traffic CONSTANT)
    // Configuration properties removed - use SettingsManager singleton directly
    // QML access: settingsManager.setNTRIP_ipAddress, settingsManager.setMenu_isMetric, etc.

    // ======== Phase 5.1 - Missing AgIO Parameters Extension (18 parameters) ========

    // Configuration properties removed - use SettingsManager singleton directly
    // Phase 5.1 parameters moved to SettingsManager for CDC compliance
    // QML access: settingsManager.setNTRIP_packetSize, settingsManager.setPort_portNameGPS2, etc.

    // Phase 6.0.21: GPS data properties (dualHeading, gpsHz, nowHz, yawrate, vehicleX, vehicleY)
    // removed - moved to FormGPS (aog). AgIOService only handles I/O status.

    // Missing Core Properties
    Q_PROPERTY(QVariantList bluetoothDevices READ bluetoothDevices WRITE setBluetoothDevices
               NOTIFY bluetoothDevicesChanged BINDABLE bindableBluetoothDevices)
    Q_PROPERTY(QByteArray serialData READ serialData WRITE setSerialData
               NOTIFY serialDataChanged BINDABLE bindableSerialData)
    Q_PROPERTY(QString serialPort READ serialPort WRITE setSerialPort
               NOTIFY serialPortChanged BINDABLE bindableSerialPort)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate
               NOTIFY baudRateChanged BINDABLE bindableBaudRate)
    Q_PROPERTY(bool isOpen READ isOpen WRITE setIsOpen
               NOTIFY isOpenChanged BINDABLE bindableIsOpen)

    Q_PROPERTY(QString unknownSentence READ unknownSentence WRITE setUnknownSentence
               NOTIFY unknownSentenceChanged BINDABLE bindableUnknownSentence)

    // NMEA Sentence Properties (for QML SerialMonitor)
    Q_PROPERTY(QString ggaSentence READ ggaSentence WRITE setGgaSentence
               NOTIFY ggaSentenceChanged BINDABLE bindableGgaSentence)
    Q_PROPERTY(QString vtgSentence READ vtgSentence WRITE setVtgSentence
               NOTIFY vtgSentenceChanged BINDABLE bindableVtgSentence)
    Q_PROPERTY(QString rmcSentence READ rmcSentence WRITE setRmcSentence
               NOTIFY rmcSentenceChanged BINDABLE bindableRmcSentence)
    Q_PROPERTY(QString pandaSentence READ pandaSentence WRITE setPandaSentence
               NOTIFY pandaSentenceChanged BINDABLE bindablePandaSentence)
    Q_PROPERTY(QString paogiSentence READ paogiSentence WRITE setPaogiSentence
               NOTIFY paogiSentenceChanged BINDABLE bindablePaogiSentence)
    Q_PROPERTY(QString hdtSentence READ hdtSentence WRITE setHdtSentence
               NOTIFY hdtSentenceChanged BINDABLE bindableHdtSentence)
    Q_PROPERTY(QString avrSentence READ avrSentence WRITE setAvrSentence
               NOTIFY avrSentenceChanged BINDABLE bindableAvrSentence)
    Q_PROPERTY(QString hpdSentence READ hpdSentence WRITE setHpdSentence
               NOTIFY hpdSentenceChanged BINDABLE bindableHpdSentence)
    Q_PROPERTY(QString sxtSentence READ sxtSentence WRITE setSxtSentence
               NOTIFY sxtSentenceChanged BINDABLE bindableSxtSentence)

    // NMEA Sentence Aliases (short names for QML - Phase 6.0.21 - READ-ONLY)
    Q_PROPERTY(QString gga READ gga NOTIFY ggaSentenceChanged BINDABLE bindableGga)
    Q_PROPERTY(QString vtg READ vtg NOTIFY vtgSentenceChanged BINDABLE bindableVtg)
    Q_PROPERTY(QString rmc READ rmc NOTIFY rmcSentenceChanged BINDABLE bindableRmc)
    Q_PROPERTY(QString panda READ panda NOTIFY pandaSentenceChanged BINDABLE bindablePanda)
    Q_PROPERTY(QString paogi READ paogi NOTIFY paogiSentenceChanged BINDABLE bindablePaogi)
    Q_PROPERTY(QString hdt READ hdt NOTIFY hdtSentenceChanged BINDABLE bindableHdt)
    Q_PROPERTY(QString avr READ avr NOTIFY avrSentenceChanged BINDABLE bindableAvr)
    Q_PROPERTY(QString hpd READ hpd NOTIFY hpdSentenceChanged BINDABLE bindableHpd)
    Q_PROPERTY(QString sxt READ sxt NOTIFY sxtSentenceChanged BINDABLE bindableSxt)

    // Enhanced UI Integration (Phase 4.5.4)
    Q_PROPERTY(QString gpsStatusText READ gpsStatusText WRITE setGpsStatusText
               NOTIFY gpsStatusTextChanged BINDABLE bindableGpsStatusText)
    Q_PROPERTY(QString moduleStatusText READ moduleStatusText WRITE setModuleStatusText
               NOTIFY moduleStatusTextChanged BINDABLE bindableModuleStatusText)
    Q_PROPERTY(QString serialStatusText READ serialStatusText WRITE setSerialStatusText
               NOTIFY serialStatusTextChanged BINDABLE bindableSerialStatusText)
    Q_PROPERTY(bool showErrorDialog READ showErrorDialog WRITE setShowErrorDialog
               NOTIFY showErrorDialogChanged BINDABLE bindableShowErrorDialog)
    Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage WRITE setLastErrorMessage
               NOTIFY lastErrorMessageChanged BINDABLE bindableLastErrorMessage)
    Q_PROPERTY(QVariantList discoveredModules READ discoveredModules WRITE setDiscoveredModules
               NOTIFY discoveredModulesChanged BINDABLE bindableDiscoveredModules)

    // Network Interface Discovery Properties
    Q_PROPERTY(QVariantList networkInterfaces READ networkInterfaces WRITE setNetworkInterfaces
               NOTIFY networkInterfacesChanged BINDABLE bindableNetworkInterfaces)
    Q_PROPERTY(QString discoveredModuleSubnet READ discoveredModuleSubnet WRITE setDiscoveredModuleSubnet
               NOTIFY discoveredModuleSubnetChanged BINDABLE bindableDiscoveredModuleSubnet)
    Q_PROPERTY(QString discoveredModuleIP READ discoveredModuleIP WRITE setDiscoveredModuleIP
               NOTIFY discoveredModuleIPChanged BINDABLE bindableDiscoveredModuleIP)

    // === Modernized Legacy Methods → Properties (Rectangle Pattern) ===
    Q_PROPERTY(bool udpListenOnly READ udpListenOnly WRITE setUdpListenOnly
               NOTIFY udpListenOnlyChanged BINDABLE bindableUdpListenOnly)
    Q_PROPERTY(bool ntripDebugEnabled READ ntripDebugEnabled WRITE setNtripDebugEnabled
               NOTIFY ntripDebugEnabledChanged BINDABLE bindableNtripDebugEnabled)
    Q_PROPERTY(bool bluetoothDebugEnabled READ bluetoothDebugEnabled WRITE setBluetoothDebugEnabled
               NOTIFY bluetoothDebugEnabledChanged BINDABLE bindableBluetoothDebugEnabled)
    Q_PROPERTY(QString ntripUrlIP READ ntripUrlIP WRITE setNtripUrlIP
               NOTIFY ntripUrlIPChanged BINDABLE bindableNtripUrlIP)

public:
    // C++ singleton access (strict singleton pattern - same as CTrack/CVehicle)
    static AgIOService* instance();
    static AgIOService *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    
    // ============================================================================
    // RECTANGLE PATTERN: Manual Method Declarations for all 54 Properties
    // ============================================================================
    // Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY requires manual declaration and implementation of:
    // - Getters: return m_property.value()
    // - Setters: m_property.setValue(value)
    // - Bindables: return &m_property

    // Phase 6.0.21: GPS/IMU data getters/setters removed - moved to FormGPS

    // Connection Status Properties
    bool gpsConnected() const;
    void setGpsConnected(bool gpsConnected);
    QBindable<bool> bindableGpsConnected();

    bool gpsPortConnected() const;
    void setGpsPortConnected(bool gpsPortConnected);
    QBindable<bool> bindableGpsPortConnected();

    bool bluetoothConnected() const;
    void setBluetoothConnected(bool bluetoothConnected);
    QBindable<bool> bindableBluetoothConnected();

    bool ethernetConnected() const;
    void setEthernetConnected(bool ethernetConnected);
    QBindable<bool> bindableEthernetConnected();

    bool ntripConnected() const;
    void setNtripConnected(bool ntripConnected);
    QBindable<bool> bindableNtripConnected();

    // GPS Quality Properties
    int gpsQuality() const;
    void setGpsQuality(int gpsQuality);
    QBindable<int> bindableGpsQuality();

    int satellites() const;
    void setSatellites(int satellites);
    QBindable<int> bindableSatellites();

    // Bluetooth Properties
    QString bluetoothDevice() const;
    void setBluetoothDevice(const QString& bluetoothDevice);
    QBindable<QString> bindableBluetoothDevice();

    // NTRIP Properties
    int ntripStatus() const;
    void setNtripStatus(int ntripStatus);
    QBindable<int> bindableNtripStatus();

    int rawTripCount() const;
    void setRawTripCount(int rawTripCount);
    QBindable<int> bindableRawTripCount();

    // Configuration Properties
    bool udpListenOnly() const;
    void setUdpListenOnly(bool udpListenOnly);
    QBindable<bool> bindableUdpListenOnly();

    bool ntripDebugEnabled() const;
    void setNtripDebugEnabled(bool ntripDebugEnabled);
    QBindable<bool> bindableNtripDebugEnabled();

    bool bluetoothDebugEnabled() const;
    void setBluetoothDebugEnabled(bool bluetoothDebugEnabled);
    QBindable<bool> bindableBluetoothDebugEnabled();

    QString ntripUrlIP() const;
    void setNtripUrlIP(const QString& ntripUrlIP);
    QBindable<QString> bindableNtripUrlIP();

    // Module Connection Properties
    bool imuConnected() const;
    void setImuConnected(bool imuConnected);
    QBindable<bool> bindableImuConnected();

    bool steerConnected() const;
    void setSteerConnected(bool steerConnected);
    QBindable<bool> bindableSteerConnected();

    bool machineConnected() const;
    void setMachineConnected(bool machineConnected);
    QBindable<bool> bindableMachineConnected();

    bool blockageConnected() const;
    void setBlockageConnected(bool blockageConnected);
    QBindable<bool> bindableBlockageConnected();

    bool rateControlConnected() const;
    void setRateControlConnected(bool rateControlConnected);
    QBindable<bool> bindableRateControlConnected();

    // PHASE 6.0.22.3: Module source and frequency properties
    QString gpsSource() const;
    void setGpsSource(const QString& gpsSource);
    QBindable<QString> bindableGpsSource();

    QString imuSource() const;
    void setImuSource(const QString& imuSource);
    QBindable<QString> bindableImuSource();

    QString steerSource() const;
    void setSteerSource(const QString& steerSource);
    QBindable<QString> bindableSteerSource();

    QString machineSource() const;
    void setMachineSource(const QString& machineSource);
    QBindable<QString> bindableMachineSource();

    double gpsFrequency() const;
    void setGpsFrequency(double gpsFrequency);
    QBindable<double> bindableGpsFrequency();

    double imuFrequency() const;
    void setImuFrequency(double imuFrequency);
    QBindable<double> bindableImuFrequency();

    double steerFrequency() const;
    void setSteerFrequency(double steerFrequency);
    QBindable<double> bindableSteerFrequency();

    double machineFrequency() const;
    void setMachineFrequency(double machineFrequency);
    QBindable<double> bindableMachineFrequency();

    // PHASE 6.0.22.12: Dynamic protocol list methods
    QVariantList activeProtocols() const;
    QString getProtocolDescription(const QString& protocolId) const;

    // NMEA Sentence Properties
    QString gga() const;
    void setGga(const QString& gga);
    QBindable<QString> bindableGga();

    QString vtg() const;
    void setVtg(const QString& vtg);
    QBindable<QString> bindableVtg();

    QString rmc() const;
    void setRmc(const QString& rmc);
    QBindable<QString> bindableRmc();

    QString panda() const;
    void setPanda(const QString& panda);
    QBindable<QString> bindablePanda();

    QString paogi() const;
    void setPaogi(const QString& paogi);
    QBindable<QString> bindablePaogi();

    QString hdt() const;
    void setHdt(const QString& hdt);
    QBindable<QString> bindableHdt();

    QString avr() const;
    void setAvr(const QString& avr);
    QBindable<QString> bindableAvr();

    QString hpd() const;
    void setHpd(const QString& hpd);
    QBindable<QString> bindableHpd();

    QString sxt() const;
    void setSxt(const QString& sxt);
    QBindable<QString> bindableSxt();

    QString unknownSentence() const;
    void setUnknownSentence(const QString& unknownSentence);
    QBindable<QString> bindableUnknownSentence();

    // ✅ Phase 6.0.21.3: Initialization guard getter
    bool ready() const { return m_ready; }

    // Status Text Properties
    QString ntripStatusText() const;
    void setNtripStatusText(const QString& ntripStatusText);
    QBindable<QString> bindableNtripStatusText();

    CTraffic* traffic() const;  // Read-only proxy to AgIOService->m_traffic

    QString gpsStatusText() const;
    void setGpsStatusText(const QString& gpsStatusText);
    QBindable<QString> bindableGpsStatusText();

    QString moduleStatusText() const;
    void setModuleStatusText(const QString& moduleStatusText);
    QBindable<QString> bindableModuleStatusText();

    QString serialStatusText() const;
    void setSerialStatusText(const QString& serialStatusText);
    QBindable<QString> bindableSerialStatusText();

    bool showErrorDialog() const;
    void setShowErrorDialog(bool showErrorDialog);
    QBindable<bool> bindableShowErrorDialog();

    QString lastErrorMessage() const;
    void setLastErrorMessage(const QString& lastErrorMessage);
    QBindable<QString> bindableLastErrorMessage();

    QVariantList discoveredModules() const;
    void setDiscoveredModules(const QVariantList& discoveredModules);
    QBindable<QVariantList> bindableDiscoveredModules();

    QVariantList networkInterfaces() const;
    void setNetworkInterfaces(const QVariantList& networkInterfaces);
    QBindable<QVariantList> bindableNetworkInterfaces();

    QString discoveredModuleSubnet() const;
    void setDiscoveredModuleSubnet(const QString& discoveredModuleSubnet);
    QBindable<QString> bindableDiscoveredModuleSubnet();

    QString discoveredModuleIP() const;
    void setDiscoveredModuleIP(const QString& discoveredModuleIP);
    QBindable<QString> bindableDiscoveredModuleIP();

    // ============================================================================
    // END RECTANGLE PATTERN MANUAL METHOD DECLARATIONS
    // ============================================================================


    // GPS quality interpretation (like FormLoop original)
    Q_INVOKABLE QString fixQuality() const;

    // Configuration bindable methods removed - use SettingsManager singleton directly

    // QML invokable methods
    Q_INVOKABLE void configureNTRIP();
    Q_INVOKABLE void startBluetoothDiscovery();
    Q_INVOKABLE void connectBluetooth(const QString& deviceName);
    Q_INVOKABLE void disconnectBluetooth();
    Q_INVOKABLE void startCommunication();
    Q_INVOKABLE void stopCommunication();
    // CDC Architecture: saveSettings() and loadSettings() removed
    // All settings now handled directly by SettingsManager
    
    // Test methods
    Q_INVOKABLE void testThreadCommunication();

    // UDP control methods (Phase 6.2.1)
    Q_INVOKABLE void enableUDP(bool enable);

    // UI Integration methods (Phase 6.0.21)
    Q_INVOKABLE void clearErrorDialog();
    Q_INVOKABLE void updateModuleStatus();
    Q_INVOKABLE void updateGPSStatus();
    Q_INVOKABLE void updateNTRIPStatus();
    Q_INVOKABLE void updateSerialStatus();

    // Additional QML methods for compatibility
    // ❌ LEGACY REPLACED: btnUDPListenOnly_clicked → udpListenOnly property
    // ❌ LEGACY REPLACED: ntripDebug → ntripDebugEnabled property
    // ❌ LEGACY REPLACED: bluetoothDebug → bluetoothDebugEnabled property
    // ❌ LEGACY REPLACED: setIPFromUrl → ntripUrlIP property
    Q_INVOKABLE void bt_search(const QString& deviceName);  // ✅ MODERNIZED: Renamed for clarity
    Q_INVOKABLE void bt_remove_device(const QString& deviceName);
    Q_INVOKABLE void btnSendSubnet_clicked();
    
    // Phase 4.6: PGN transmission (replaces FormGPS SendPgnToLoop)
    Q_INVOKABLE void sendPgn(const QByteArray& pgnData);
    
    // Phase 4.5: Direct worker communication (FormLoop eliminated)

public slots:
    // Application lifecycle
    void initialize();
    void shutdown();

    // AgIOService ON/OFF control slot
    void onAgIOServiceToggled();

    // ✅ Phase 6.0.21.3: Network Interface Management (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void refreshNetworkInterfaces();
    Q_INVOKABLE void autoConfigureFromDetection();
    Q_INVOKABLE bool canReachModuleSubnet(const QString& subnet);
    Q_INVOKABLE void scanAllSubnets();

    // ✅ Phase 6.0.21.3: Network configuration (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void configureSubnet();
    Q_INVOKABLE void sendPGN201ToDetectedModule();  // Optimized PGN 201 without 202 scan

    // Phase 6.0.24: UDP socket management (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void startUDP(const QString& address, int port);
    Q_INVOKABLE void stopUDP();
    Q_INVOKABLE void sendToTractor(const QByteArray& data);
    Q_INVOKABLE void sendRawMessage(const QByteArray& data, const QString& address, int port);

    // Phase 6.0.24: Module discovery (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void startModuleDiscovery();
    Q_INVOKABLE void stopModuleDiscovery();
    Q_INVOKABLE void scanSubnet(const QString& baseIP);
    Q_INVOKABLE void pingModule(const QString& moduleIP);
    Q_INVOKABLE void wakeUpModules(const QString& broadcastIP = "192.168.1.255");
    Q_INVOKABLE void sendHelloMessage(const QString& moduleIP = "");
    Q_INVOKABLE void sendScanRequest();

    // Phase 6.0.24: Multi-subnet discovery (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void sendPgnToDiscoveredSubnet(const QByteArray& pgnData);
    Q_INVOKABLE void sendToAllInterfaces(const QByteArray& data, const QString& targetIP, int port);

    // Phase 6.0.24: Network interface discovery (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE QVariantList getNetworkInterfaces();
    Q_INVOKABLE QString getLocalSubnet();
    Q_INVOKABLE bool hasInterfaceOnSubnet(const QString& subnet);
    Q_INVOKABLE QString getDiscoveredModuleSubnet() const { return m_discoveredModuleSubnet; }
    Q_INVOKABLE QString getDiscoveredModuleIP() const { return m_discoveredModuleIP; }
    Q_INVOKABLE void testPortAccessibility(int port, const QString& description);

    // Phase 6.0.24: Advanced network configuration (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void configureLocalAOGIP(const QString& localIP);
    Q_INVOKABLE void enableNMEAToUDPRelay(bool enable);
    Q_INVOKABLE void startModuleHeartbeatMonitoring();
    Q_INVOKABLE void stopModuleHeartbeatMonitoring();
    Q_INVOKABLE void updateModuleStatusSlot(const QString& moduleType, bool connected);

    // Phase 6.0.24: UDP traffic management (Q_INVOKABLE for QML thread-safety)
    Q_INVOKABLE void incrementUdpCounters(int bytesIn = 0, bool isOut = false);
    Q_INVOKABLE void resetUdpTrafficCounters();
    Q_INVOKABLE void pauseAllUDPTraffic();
    Q_INVOKABLE void resumeAllUDPTraffic();
    Q_INVOKABLE bool isModuleConnected(const QString& moduleType) const;
    Q_INVOKABLE quint32 getHelloCount(const QString& moduleType) const;
    Q_INVOKABLE void resetTrafficCounters();

    // Serial port management for QML (Phase 6.1 - SerialMonitor functionality)
    // ✅ CORRECTED: Removed Q_INVOKABLE (slots are already QML-accessible)
    bool openSerialPort(const QString& portType, const QString& portName, int baudRate);
    // ✅ CDC COMPLIANT: Non-blocking async version
    void openSerialPortAsync(const QString& portType, const QString& portName, int baudRate);
    void closeSerialPort(const QString& portType);
    QStringList getAvailableSerialPorts();
    QStringList getAvailableSerialPortsForModule(const QString& moduleType);
    bool isSerialPortConnected(const QString& portType);
    void writeToSerialPort(const QString& portType, const QString& data);
    QString getPortOwner(const QString& portName);
    QString getConnectedPortName(const QString& moduleType);
    bool validateModuleType(const QString& portName, const QString& expectedModuleType, int baudRate);

private slots:
    // Worker thread data reception (Qt::QueuedConnection for thread-safe processing)
    // Note: onGpsDataReceived/onImuDataReceived removed - Rectangle Pattern uses direct setters
    void onGpsStatusChanged(bool connected, int quality, int satellites, double age);
    // Phase 6.0.21.7: onNmeaSentenceReceived() REMOVED - dead code, replaced by onDataReceived()

    void onBluetoothStatusChanged(bool connected, const QString& device);
    void onNtripStatusChanged(int status, const QString& statusText);
    void onUdpStatusChanged(bool connected);

    void onNtripDataReceived(const QByteArray& rtcmData);

    // Phase 6.0.24: UDP event-driven handlers
    void onUdpDataReady();
    void onUdpError(QAbstractSocket::SocketError error);

    // Phase 6.0.24: UDP internal timers
    void checkConnectionStatus();
    void sendHeartbeat();
    void doTraffic();
    void broadcastHello();
    void checkModuleTimeouts();
    void checkModuleHeartbeats();
    
    
    // Serial Worker slots - Legacy 3 ports
    void onSerialIMUDataReceived(const QByteArray& imuData);
    void onSerialAutosteerResponse(const QByteArray& response);
    void onSerialGPSConnected(bool connected);
    void onSerialIMUConnected(bool connected);
    void onSerialAutosteerConnected(bool connected);
    void onSerialError(const QString& portName, const QString& error);

    // ✅ PHASE 5.2 - Extended Serial Worker slots for 5 new ports
    void onSerialGPS2Connected(bool connected);
    void onSerialMachineConnected(bool connected);
    void onSerialRadioConnected(bool connected);
    void onSerialRTCMConnected(bool connected);
    void onSerialToolConnected(bool connected);
    void onSerialPortConnected(const QString& portType, bool connected);

    // Extended data reception slots
    void onSerialMachineDataReceived(const QByteArray& machineData);
    void onSerialRadioDataReceived(const QByteArray& radioData);
    void onSerialRTCMDataReceived(const QByteArray& rtcmData);
    void onSerialToolDataReceived(const QByteArray& toolData);
    void onSerialPortDataReceived(const QString& portType, const QByteArray& data);

    // ✅ CDC Compliance - Async operation result handlers (replaces BlockingQueuedConnection)
    void onPortOpenResult(const QString& portType, bool success, const QString& errorMessage);
    void onPortCloseResult(const QString& portType, bool success);
    void onConnectionStatusResult(const QString& portType, bool isConnected);
    void onPortNameResult(const QString& portType, const QString& portName);
    void onPortBaudRateResult(const QString& portType, int baudRate);
    void onAvailablePortsResult(const QStringList& ports);

    // Port configuration slots
    void configureGPS2Port(const QString& portName, int baudRate);
    void configureMachinePort(const QString& portName, int baudRate);
    void configureRadioPort(const QString& portName, int baudRate);
    void configureRTCMPort(const QString& portName, int baudRate);
    void configureToolPort(const QString& portName, int baudRate);
    
    // Enhanced UDP Worker slots (Phase 4.5.4)
    void onModuleDiscovered(const QString& moduleIP, const QString& moduleType);
    void onModuleTimeout(const QString& moduleIP);
    void onNetworkScanCompleted(const QStringList& discoveredModules);
    void onPgnDataReceived(const QByteArray& pgnData);

    // Multi-subnet discovery slots
    void onModuleSubnetDiscovered(const QString& moduleIP, const QString& currentSubnet);
    void onSubnetScanCompleted(const QString& activeSubnet);

    // Phase 6.0.21.7: onNmeaReceivedFromSerial() REMOVED - dead code, replaced by onDataReceived()

    // ✅ PHASE 5.3 - Extended UDP Worker slots for module monitoring
    void onUDPModuleIMUStatusChanged(bool connected);
    void onUDPModuleSteerStatusChanged(bool connected);
    void onUDPModuleMachineStatusChanged(bool connected);
    void onUDPModuleHistoryUpdated(const QString& moduleType, bool wasConnected);
    void onUDPNMEARelayed(const QString& nmeaSentence);
    void onUDPLocalAOGIPConfigured(const QString& localIP);
    void onUDPModuleHeartbeatUpdate(const QString& moduleType, int intervalMs);
    void onUDPNMEABroadcastSent(const QString& nmeaSentence, const QString& targetAddress);

    // ✅ PHASE 5.3 - Extended NTRIP Worker slots for RTCM routing
    void onNTRIPRouteRTCMToSerial(const QByteArray& rtcmData);
    void onNTRIPBroadcastRTCMToUDP(const QByteArray& rtcmData);
    void onNTRIPRTCMPacketProcessed(int size, const QString& destination);

    // ✅ PHASE 5.3 - Extended GPS Worker slots for GPS2 support
    void onGPS2DataReceived(double latitude, double longitude, double heading, double speed);
    void onGPS2StatusChanged(bool connected, int quality, int satellites, double age);

    // ===== NMEA Sentence Properties Method Declarations =====
    QString ggaSentence() const;
    void setGgaSentence(const QString& ggaSentence);
    QBindable<QString> bindableGgaSentence();

    QString vtgSentence() const;
    void setVtgSentence(const QString& vtgSentence);
    QBindable<QString> bindableVtgSentence();

    QString rmcSentence() const;
    void setRmcSentence(const QString& rmcSentence);
    QBindable<QString> bindableRmcSentence();

    QString pandaSentence() const;
    void setPandaSentence(const QString& pandaSentence);
    QBindable<QString> bindablePandaSentence();

    QString paogiSentence() const;
    void setPaogiSentence(const QString& paogiSentence);
    QBindable<QString> bindablePaogiSentence();

    QString hdtSentence() const;
    void setHdtSentence(const QString& hdtSentence);
    QBindable<QString> bindableHdtSentence();

    QString avrSentence() const;
    void setAvrSentence(const QString& avrSentence);
    QBindable<QString> bindableAvrSentence();

    QString hpdSentence() const;
    void setHpdSentence(const QString& hpdSentence);
    QBindable<QString> bindableHpdSentence();

    QString sxtSentence() const;
    void setSxtSentence(const QString& sxtSentence);
    QBindable<QString> bindableSxtSentence();

    // ===== Missing Core Properties Method Declarations =====
    QVariantList bluetoothDevices() const;
    void setBluetoothDevices(const QVariantList& bluetoothDevices);
    QBindable<QVariantList> bindableBluetoothDevices();

    QByteArray serialData() const;
    void setSerialData(const QByteArray& serialData);
    QBindable<QByteArray> bindableSerialData();

    QString serialPort() const;
    void setSerialPort(const QString& serialPort);
    QBindable<QString> bindableSerialPort();

    int baudRate() const;
    void setBaudRate(int baudRate);
    QBindable<int> bindableBaudRate();

    bool isOpen() const;
    void setIsOpen(bool isOpen);
    QBindable<bool> bindableIsOpen();

signals:
    // Phase 6.0.21: Broadcast parsed data to all consumers
    void parsedDataReady(const PGNParser::ParsedData& data);

    // Phase 6.0.25: Separated data streams for optimal routing
    void nmeaDataReady(const PGNParser::ParsedData& data);     // GPS position data (NMEA only)
    void imuDataReady(const PGNParser::ParsedData& data);      // External IMU data (PGN 211 only)
    void steerDataReady(const PGNParser::ParsedData& data);    // AutoSteer feedback (PGN 253/250)
    void machineDataReady(const PGNParser::ParsedData& data);    // Machine
    void blockageDataReady(const PGNParser::ParsedData& data);    // Blockage data (PGN 244)
    void rateControlDataReady(const PGNParser::ParsedData& data);    // Blockage data (PGN 240)

    // Rectangle Pattern NOTIFY signals for STATUS properties only
    // GPS/IMU data properties removed - moved to FormGPS

    // GPS status (4 properties)
    void gpsConnectedChanged();
    void gpsPortConnectedChanged();
    void gpsQualityChanged();
    void satellitesChanged();

    // Connection status (4 properties)
    void bluetoothConnectedChanged();
    void bluetoothDeviceChanged();
    void ethernetConnectedChanged();
    void ntripConnectedChanged();

    // NTRIP status (3 properties)
    void ntripStatusChanged();
    void ntripStatusTextChanged();
    void rawTripCountChanged();

    // Additional connection status (4 properties)
    void imuConnectedChanged();
    void steerConnectedChanged();
    void machineConnectedChanged();
    void blockageConnectedChanged();
    void rateControlConnectedChanged();

    // PHASE 6.0.22.3: Module source and frequency signals (8 properties)
    void gpsSourceChanged();
    void imuSourceChanged();
    void steerSourceChanged();
    void machineSourceChanged();
    void gpsFrequencyChanged();
    void imuFrequencyChanged();
    void steerFrequencyChanged();
    void machineFrequencyChanged();

    // PHASE 6.0.22.12: Dynamic protocol list change signal
    void activeProtocolsChanged();

    // trafficChanged removed - CTraffic QProperty handles binding automatically

    // Additional GPS properties (7 properties - some are aliases using existing signals)
    // satellitesChanged(); - already declared above
    // gpsQualityChanged(); - already declared above
    // headingChanged(); - already declared above
    void dualHeadingChanged();
    void gpsHzChanged();
    void nowHzChanged();
    void yawrateChanged();

    // NMEA sentence strings (9 properties)
    void ggaSentenceChanged();
    void vtgSentenceChanged();
    void rmcSentenceChanged();
    void pandaSentenceChanged();
    void paogiSentenceChanged();
    void hdtSentenceChanged();
    void avrSentenceChanged();
    void hpdSentenceChanged();
    void sxtSentenceChanged();
    void unknownSentenceChanged();

    // Enhanced UI Integration (6 properties)
    void gpsStatusTextChanged();
    void moduleStatusTextChanged();
    void serialStatusTextChanged();
    void showErrorDialogChanged();
    void lastErrorMessageChanged();
    void discoveredModulesChanged();

    // Network Interface Discovery (3 properties)
    void networkInterfacesChanged();
    void discoveredModuleSubnetChanged();
    void discoveredModuleIPChanged();

    // === Modernized Legacy Methods → Properties NOTIFY signals ===
    void udpListenOnlyChanged();
    void ntripDebugEnabledChanged();
    void bluetoothDebugEnabledChanged();
    void ntripUrlIPChanged();

    // ✅ CDC COMPLIANT: Async serial port operation results
    void serialPortOpened(const QString& portType, bool success, const QString& errorMessage);
    void serialPortClosed(const QString& portType, bool success);
    void serialPortListChanged();
    void availablePortsChanged(const QStringList& ports);
    void portNameChanged(const QString& portType, const QString& portName);
    void portBaudRateChanged(const QString& portType, int baudRate);

    // Additional status change signals for QML compatibility
    void imuStatusChanged();
    void steerStatusChanged();
    void machineStatusChanged();
    void blockageStatusChanged();
    void rateControlStatusChanged();

    // Phase 6.0.24: UDP communication signals
    void udpStatusChanged(bool connected);
    void errorOccurred(const QString& error);

    // Phase 6.0.24: Module discovery signals
    void moduleDiscovered(const QString& moduleIP, const QString& moduleType);
    void moduleTimeout(const QString& moduleIP);
    void networkScanCompleted(const QStringList& discoveredModules);
    void pgnDataReceived(const QByteArray& pgnData);

    // Phase 6.0.24: Multi-subnet discovery signals
    void moduleSubnetDiscovered(const QString& moduleIP, const QString& currentSubnet);
    void subnetScanCompleted(const QString& activeSubnet);

    // Phase 6.0.24: Module status real-time signals
    void moduleIMUStatusChanged(bool connected);
    void moduleSteerStatusChanged(bool connected);
    void moduleMachineStatusChanged(bool connected);
    void moduleHistoryUpdated(const QString& moduleType, bool wasConnected);

    // Phase 6.0.24: Advanced network signals
    void localAOGIPConfigured(const QString& localIP);
    void subnetMismatchDetected(const QString& pcSubnet, const QString& moduleSubnet, const QString& moduleIP);
    void moduleDiscoveryChanged();

    // ✅ CDC Compliance - Connection status signals for async operations
    void gpsConnectionChanged(bool connected);
    void imuConnectionChanged(bool connected);
    void steerConnectionChanged(bool connected);
    void machineConnectionChanged(bool connected);
    void blockageConnectionChanged(bool connected);
    void rateControlConnectionChanged(bool connected);

    // Phase 5.1 - Module Status Real-Time signals
    void moduleStatusChanged();  // For 6 module status properties (setMod_*, setPort_was*)

    // Phase 6.1 - SerialMonitor data streams for QML
    void gpsDataReceived(const QString& data);
    void imuDataReceived(const QString& data);
    void autosteerDataReceived(const QString& data);
    void machineDataReceived(const QString& data);
    void blockageDataReceived(const QString& data);
    void rateControlDataReceived(const QString& data);

    // Serial port validation signals
    void portAlreadyInUse(const QString& portName, const QString& ownerModule);
    void moduleValidationFailed(const QString& portName, const QString& expectedModule, const QString& detectedModule);
    void moduleTypeDetected(const QString& portName, const QString& detectedModuleType, const QString& dataPattern);

    // Enhanced UI Integration signals (Phase 4.5.4)
    void statusChanged();
    void statusMessageChanged(const QString& message);

    // ✅ Phase 6.0.21.3: Initialization guard signal
    void readyChanged();

    
    // Commands to workers (Qt::QueuedConnection)
    void requestStartGPS(const QString& serialPort, int baudRate);
    void requestStopGPS();
    void requestStartNTRIP(const QString& url, const QString& user, 
                          const QString& password, const QString& mount, int port);
    void requestStopNTRIP();
    void requestStartUDP(const QString& address, int port);
    void requestStopUDP();
    void requestBluetoothScan();
    
    // Serial Worker commands
    void requestSerialWorkerStart();
    void requestSerialWorkerStop();

    // ===== Missing NOTIFY signals for added properties =====
    void vehicleXChanged();
    void vehicleYChanged();
    void bluetoothDevicesChanged();
    void serialDataChanged();
    void serialPortChanged();
    void baudRateChanged();
    void isOpenChanged();

private:
    // Private constructor for strict singleton pattern
    explicit AgIOService(QObject* parent = nullptr);
    ~AgIOService();

    //prevent copying
    AgIOService(const AgIOService &) = delete;
    AgIOService &operator=(const AgIOService &) = delete;

    static AgIOService *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    // ✅ CDC COMPLIANT: Private async methods for non-blocking subnet configuration
    void sendPGN201ToAllInterfaces();
    void sendPGN201ToNextSubnet(const QByteArray& message, const QStringList& subnets, int index);
    void handleModuleRestart();

    // ✅ CDC COMPLIANT: Main thread coordination for periodic operations
    void startPeriodicModuleScanning();

    // Module detection helper methods
    bool performQuickModuleDetection(const QString& portName, const QString& expectedModuleType, int baudRate);
    QString analyzeDataPattern(const QByteArray& data);
    QString analyzeDataPattern(const QString& data);
    bool isNMEAGPSData(const QString& data);
    bool isAgIOBinaryData(const QByteArray& data);
    bool isRandomComputerData(const QByteArray& data);

    // Phase 6.0.21: GPS/IMU data members removed - moved to FormGPS

    // Status data - Qt 6.8 Rectangle Pattern
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_gpsConnected, &AgIOService::gpsConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_gpsPortConnected, &AgIOService::gpsPortConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_bluetoothConnected, &AgIOService::bluetoothConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_ethernetConnected, &AgIOService::ethernetConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_ntripConnected, &AgIOService::ntripConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, int, m_gpsQuality, &AgIOService::gpsQualityChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, int, m_satellites, &AgIOService::satellitesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_bluetoothDevice, &AgIOService::bluetoothDeviceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, int, m_ntripStatus, &AgIOService::ntripStatusChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, int, m_rawTripCount, &AgIOService::rawTripCountChanged)

    // ✅ MODERNIZED PROPERTIES - Converted from legacy Q_INVOKABLE methods
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_udpListenOnly, &AgIOService::udpListenOnlyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_ntripDebugEnabled, &AgIOService::ntripDebugEnabledChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_bluetoothDebugEnabled, &AgIOService::bluetoothDebugEnabledChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_ntripUrlIP, &AgIOService::ntripUrlIPChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_imuConnected, &AgIOService::imuConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_steerConnected, &AgIOService::steerConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_machineConnected, &AgIOService::machineConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_blockageConnected, &AgIOService::blockageConnectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_rateControlConnected, &AgIOService::rateControlConnectedChanged)

    // PHASE 6.0.22.3: Module source and frequency tracking
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_gpsSource, &AgIOService::gpsSourceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_imuSource, &AgIOService::imuSourceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_steerSource, &AgIOService::steerSourceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_machineSource, &AgIOService::machineSourceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, double, m_gpsFrequency, &AgIOService::gpsFrequencyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, double, m_imuFrequency, &AgIOService::imuFrequencyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, double, m_steerFrequency, &AgIOService::steerFrequencyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, double, m_machineFrequency, &AgIOService::machineFrequencyChanged)

    // NTRIP status (Qt 6.8 QProperty m_ntripStatus, m_rawTripCount already declared above)
    bool m_ntripEnabled;
    
    // Configuration variables removed - use SettingsManager singleton directly
    // These were duplicated data that violated Single Source of Truth principle

    // ======== Phase 5.1 - Missing AgIO Parameters Member Variables (25 total) - Qt 6.8 QProperty + BINDABLE ========

    // Configuration variables removed - use SettingsManager singleton directly
    // NTRIP Advanced and Multi-Serial Port configuration moved to SettingsManager

    // Module Status Real-Time members - CLEANED UP: Removed duplicates
    // Duplicates removed: m_isIMUConnected (→ use m_imuConnected), m_isSteerConnected (→ use m_steerConnected), m_isMachineConnected (→ use m_machineConnected)
    // Unused pattern removed: m_wasSteerConnected (wasConnected pattern deprecated)

    // Network configuration variables removed - use SettingsManager singleton directly

    // UDP settings
    QString m_udpBroadcastAddress;  // e.g., "192.168.1.255"
    int m_udpListenPort;  // 9999 for AgIO
    int m_udpSendPort;    // 8888 to modules
    // m_udpIsOn removed - use settingsManager.setUDP_isOn instead

    // NMEA sentence storage for debugging - Qt 6.8 Rectangle Pattern (9 parameters)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_ggaSentence, &AgIOService::ggaSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_vtgSentence, &AgIOService::vtgSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_rmcSentence, &AgIOService::rmcSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_pandaSentence, &AgIOService::pandaSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_paogiSentence, &AgIOService::paogiSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_hdtSentence, &AgIOService::hdtSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_avrSentence, &AgIOService::avrSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_hpdSentence, &AgIOService::hpdSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_sxtSentence, &AgIOService::sxtSentenceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_unknownSentence, &AgIOService::unknownSentenceChanged)

    // Enhanced UI state (Phase 4.5.4) - Qt 6.8 Rectangle Pattern (11 properties + 1 proxy)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_ntripStatusText, &AgIOService::ntripStatusTextChanged)
    // CTraffic proxy pointer - points to UDPWorker->m_traffic (not owned by AgIOService)
    CTraffic* m_traffic;
    // m_ntripIPAddress Q_OBJECT_BINDABLE_PROPERTY removed - internal C++ only, no QML exposure needed
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_gpsStatusText, &AgIOService::gpsStatusTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_moduleStatusText, &AgIOService::moduleStatusTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_serialStatusText, &AgIOService::serialStatusTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_showErrorDialog, &AgIOService::showErrorDialogChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_lastErrorMessage, &AgIOService::lastErrorMessageChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QVariantList, m_discoveredModules, &AgIOService::discoveredModulesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QVariantList, m_networkInterfaces, &AgIOService::networkInterfacesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_discoveredModuleSubnet, &AgIOService::discoveredModuleSubnetChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_discoveredModuleIP, &AgIOService::discoveredModuleIPChanged)

    // Missing Core Properties
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QVariantList, m_bluetoothDevices, &AgIOService::bluetoothDevicesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QByteArray, m_serialData, &AgIOService::serialDataChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, QString, m_serialPort, &AgIOService::serialPortChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, int, m_baudRate, &AgIOService::baudRateChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AgIOService, bool, m_isOpen, &AgIOService::isOpenChanged)

    // Configuration QProperty variables removed - use SettingsManager singleton directly
    // These duplicate QProperty instances violated Single Source of Truth principle

    // Worker threads
    // Phase 6.0.21: m_gpsThread removed (GPSWorker deleted - useless thread)
    // Phase 6.0.24: m_udpThread removed (event-driven socket in main thread)
    QThread* m_ntripThread;
    QThread* m_serialThread;

    // Workers
    // Phase 6.0.21: m_gpsWorker removed (deleted - useless thread with no I/O)
    // Phase 6.0.24: m_udpWorker removed (event-driven socket in main thread)
    NTRIPWorker* m_ntripWorker;
    SerialWorker* m_serialWorker;

    // Phase 6.0.24: Event-driven UDP socket in main thread
    QUdpSocket* m_udpSocket;

    // Phase 6.0.24: UDP timers (main thread)
    QTimer* m_statusTimer;
    QTimer* m_udpHeartbeatTimer;
    QTimer* m_trafficTimer;
    QTimer* m_discoveryTimer;
    QTimer* m_timeoutTimer;
    QTimer* m_subnetScanTimer;
    QTimer* m_heartbeatMonitorTimer;
    QTimer* m_nmeaRelayTimer;

    // Phase 6.0.24: UDP connection state
    QHostAddress m_tractorAddress;
    int m_tractorPort;       // Port 8888: Module communication (PGN, autosteer, IMU)
    int m_rtcmPort;          // Port 2233: NTRIP/RTCM corrections (AgOpenGPS standard)
    int m_listenPort;
    bool m_isRunning;
    bool m_isConnected;
    bool m_isUdpEnabled;
    qint64 m_lastDataTime;
    int m_dataTimeoutMs;

    // Phase 6.0.24: UDP statistics
    qint64 m_bytesReceived;
    qint64 m_bytesSent;
    int m_packetsReceived;
    int m_packetsSent;
    double m_receiveRate;
    double m_sendRate;
    QByteArray m_receiveBuffer;
    QString m_nmeaBuffer;

    // Phase 6.0.24: UDP traffic counters
    qint64 m_udpOutBytes;
    qint64 m_udpInBytes;
    qint64 m_udpOutStartTime;
    qint64 m_udpInStartTime;
    qint64 m_udpOutLastTime;
    qint64 m_udpInLastTime;
    quint32 m_udpOutPackets;
    quint32 m_udpInPackets;

    // Phase 6.0.24: Delta tracking for instantaneous rate calculation (Problem 9 fix)
    qint64 m_udpInBytesLast;
    qint64 m_udpInPacketsLast;
    qint64 m_udpOutBytesLast;
    qint64 m_udpOutPacketsLast;
    qint64 m_lastTrafficTime;

    // Phase 6.0.24: Module discovery
    QMap<QString, QDateTime> m_moduleLastSeen;
    bool m_discoveryActive;
    QString m_currentSubnet;

    // Phase 6.0.24: Multi-subnet discovery
    QStringList m_commonSubnets;
    int m_currentSubnetIndex;
    bool m_subnetScanActive;
    QByteArray m_pendingPgnData;

    // Phase 6.0.24: Module status monitoring
    QMap<QString, bool> m_moduleStatus;
    QMap<QString, QDateTime> m_moduleHistory;
    bool m_heartbeatMonitoringActive;

    // Phase 6.0.24: Advanced network configuration
    QString m_localAOGIP;
    bool m_nmeaToUDPRelayEnabled;
    bool m_udpTrafficPaused;

    // Phase 6.0.24: Local traffic counters (main thread)
    quint32 m_localCntrMachine;
    quint32 m_localCntrBlockage;
    quint32 m_localCntrRateControl;
    quint32 m_localCntrSteer;
    quint32 m_localCntrIMU;
    quint32 m_localCntrUDPOut;
    quint32 m_localCntrUDPIn;
    quint32 m_localCntrUDPInBytes;
    double m_localUdpOutRate;
    double m_localUdpInRate;
    double m_localUdpOutFreq;
    double m_localUdpInFreq;

    // Phase 6.0.21: Centralized parser for NMEA text and PGN binary
    PGNParser* m_pgnParser;

    // Core components
    QTimer* m_heartbeatTimer;

    // Initialization flags
    bool m_initialized;
    bool m_ready;  // ✅ Phase 6.0.21.3: Guard for QML thread-safe access

    // ✅ CDC Compliance - Cache for async operation results (avoids blocking calls)
    QStringList m_cachedAvailablePorts;
    QMap<QString, bool> m_cachedConnectionStatus;
    QMap<QString, QString> m_cachedPortNames;
    QMap<QString, int> m_cachedBaudRates;

    // PHASE 6.0.22.12: Protocol status tracking (protocol-centric)
    QMap<QString, ModuleStatus> m_protocolStatusMap;  // Key: "$PANDA", "$GGA", "PGN211", "PGN214", etc.
    QTimer* m_moduleStatusUpdateTimer;  // Throttle QML property updates
    bool m_pendingModuleUpdates;        // Flag for batching updates
    QTimer* m_modulePingTimer;          // Sends PGN 200 hello every second (ModSim compatibility)

    // PHASE 6.0.22.12: Protocol status tracking (protocol-centric)
    void updateModuleStatus(const QString& protocolId, const QString& transport,
                           const QString& sourceID, qint64 timestampMs);
    void applyPendingModuleUpdates();   // Apply batched updates to QML properties
    void sendModuleHello();             // Sends PGN 200 hello to modules (ModSim compatibility)
    QString detectModuleType(const PGNParser::ParsedData& data);  // PHASE 6.0.22.12: No longer used, kept for compatibility
    bool shouldAcceptData(const QString& protocolId, const QString& transport);

    // Setup methods
    void setupWorkerThreads();
    void connectWorkerSignals();
    void loadDefaultSettings();
    void initializeUdpSocket();  // Phase 6.0.24: Event-driven UDP initialization

    // Phase 6.0.24: UDP utility methods
    void sendModuleData(const QByteArray& moduleData);
    QByteArray buildHeartbeatPacket() const;
    bool bindSocket();
    void cleanupConnection();
    void updateReceiveRate();
    void updateSendRate();
    void resetStatistics();
    void processModuleResponse(const QByteArray& data);
    void handlePGNProtocol(const QByteArray& data);
    void validateModuleConnection();
    void manageModuleTimeouts();
    QString getSubnetFromIP(const QString& ip);
    void updateSubnetList();
    void initializeModuleStatus();
    void processModuleHeartbeat(const QByteArray& data);
    void updateModuleConnectionStatus(const QString& moduleType, bool connected);
    void recordModuleHistory(const QString& moduleType, bool connected);
    void processHelloMessage(const QByteArray& data);
    void checkSubnetMismatch(const QString& moduleSubnet, const QString& moduleIP);
    QString getCurrentPCSubnet();
    void emitTrafficChangedThrottled(bool force = false);

    // Utility methods
    // Phase 6.0.21: updateVehiclePosition() removed - GPS/position data now in FormGPS
    void logDebugInfo() const;
};

#endif // AGIOSERVICE_H
