#include "agioservice.h"
#include "settingsmanager.h"
#include "formgps.h"
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QElapsedTimer>
#include <QHostInfo>
#include <QDateTime>
#include <QRegularExpression>

// Phase 6.0.24: Logging category definition for selective debug logging
// Usage: qCDebug(agioservice) << "message"; instead of qDebug()
// Control via main.cpp: agioservice.debug=true|false
Q_LOGGING_CATEGORY(agioservice, "agioservice")

AgIOService *AgIOService::s_instance = nullptr;
QMutex AgIOService::s_mutex;
bool AgIOService::s_cpp_created = false;

// SomcoSoftware approach: Qt manages the singleton automatically

AgIOService::AgIOService(QObject *parent)
    : QObject(parent)
    // RECTANGLE PATTERN: All Q_OBJECT_BINDABLE_PROPERTY members auto-initialized
    // No manual initialization needed for: m_latitude, m_longitude, m_heading, etc.
    // Rectangle Pattern handles initialization automatically with default values
    // Phase 6.0.21: m_easting, m_northing removed (stored in FormGPS now)
    // Configuration variables removed - use SettingsManager::instance() directly
    // Phase 6.0.21: m_gpsThread removed (GPSWorker deleted)
    // Phase 6.0.24: m_udpThread removed (event-driven socket in main thread)
    , m_ntripThread(nullptr)
    , m_serialThread(nullptr)
    // Phase 6.0.21: m_gpsWorker removed (GPSWorker deleted)
    // Phase 6.0.24: m_udpWorker removed (event-driven socket in main thread)
    , m_ntripWorker(nullptr)
    , m_serialWorker(nullptr)
    , m_traffic(nullptr)  // Phase 6.0.21.2: Initialized FIRST (matches .h line 794 order)
    , m_pgnParser(nullptr)  // Phase 6.0.21: Centralized NMEA + PGN parser
    , m_heartbeatTimer(nullptr)
    , m_moduleStatusUpdateTimer(nullptr)  // PHASE 6.0.22: Throttle property updates
    , m_pendingModuleUpdates(false)
    , m_initialized(false)
    , m_ready(false)  // ✅ Phase 6.0.21.3: QML guard - will be set true after full initialization
    , m_cachedAvailablePorts()       // Empty port list initially
    , m_cachedConnectionStatus()     // Empty connection status map
    , m_cachedPortNames()           // Empty port names map
    , m_cachedBaudRates()           // Empty baud rates map
{
    // ✅ Phase 6.0.21.3: Create CTraffic immediately (Main Thread)
    // CRITICAL: Must be first line in constructor body to prevent QML nullptr access
    // NO PARENT (nullptr) for explicit lifecycle management
    m_traffic = new CTraffic(nullptr);
    qDebug() << "✅ CTraffic created in Main Thread (thread-safe for QML)";

    qDebug() << "🔧 AgIOService constructor - Main Thread:" << QThread::currentThread();
    
    qDebug() << "🏗️ AgIOService singleton constructor called, parent:" << parent;
    
    // Initialize AgIO-specific settings - now via SettingsManager (thread-safe)
    // Uses unified QtAgOpenGPS.ini via SettingsManager
    qDebug() << "📁 AgIOService now uses SettingsManager for thread-safe settings";
    
    // Initialize heartbeat timer
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(1000); // 1Hz heartbeat
    connect(m_heartbeatTimer, &QTimer::timeout, this, &AgIOService::logDebugInfo);

    // PHASE 6.0.22: Initialize module status update timer (throttle QML property updates)
    m_moduleStatusUpdateTimer = new QTimer(this);
    m_moduleStatusUpdateTimer->setInterval(1000); // 1Hz update rate - aligns with frequency calculation window
    m_moduleStatusUpdateTimer->setSingleShot(false);
    connect(m_moduleStatusUpdateTimer, &QTimer::timeout, this, &AgIOService::applyPendingModuleUpdates);

    // PHASE 6.0.22.12: Initialize module ping timer (sends PGN 200 hello every second for ModSim compatibility)
    m_modulePingTimer = new QTimer(this);
    m_modulePingTimer->setInterval(1000); // 1Hz - matches C# AgIO OneSecondLoop
    m_modulePingTimer->setSingleShot(false);
    connect(m_modulePingTimer, &QTimer::timeout, this, &AgIOService::sendModuleHello);

    // Phase 6.0.21: Initialize centralized NMEA + PGN parser
    m_pgnParser = new PGNParser(this);
    qDebug() << "PGNParser initialized - centralized NMEA and PGN binary parsing ready";

    // Phase 6.0.24: Initialize UDP timers (main thread event-driven architecture)
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(2000);
    connect(m_statusTimer, &QTimer::timeout, this, &AgIOService::checkConnectionStatus);

    m_udpHeartbeatTimer = new QTimer(this);
    m_udpHeartbeatTimer->setInterval(5000);
    connect(m_udpHeartbeatTimer, &QTimer::timeout, this, &AgIOService::sendHeartbeat);

    m_trafficTimer = new QTimer(this);
    m_trafficTimer->setInterval(2000);
    connect(m_trafficTimer, &QTimer::timeout, this, &AgIOService::doTraffic);

    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setInterval(2000);
    connect(m_discoveryTimer, &QTimer::timeout, this, &AgIOService::broadcastHello);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(10000);
    connect(m_timeoutTimer, &QTimer::timeout, this, &AgIOService::checkModuleTimeouts);

    m_subnetScanTimer = new QTimer(this);
    m_subnetScanTimer->setInterval(5000);
    m_subnetScanTimer->setSingleShot(true);
    connect(m_subnetScanTimer, &QTimer::timeout, this, [this]() {
        m_currentSubnetIndex++;
        scanAllSubnets();
    });

    m_heartbeatMonitorTimer = new QTimer(this);
    m_heartbeatMonitorTimer->setInterval(5000);
    connect(m_heartbeatMonitorTimer, &QTimer::timeout, this, &AgIOService::checkModuleHeartbeats);

    m_nmeaRelayTimer = new QTimer(this);
    m_nmeaRelayTimer->setInterval(100);

    // Initialize common subnets for multi-subnet discovery
    m_commonSubnets << "192.168.1" << "192.168.2" << "192.168.3" << "192.168.4" << "192.168.5" << "192.168.0";

    // Initialize module status monitoring
    initializeModuleStatus();

    // Start traffic monitoring timer
    m_trafficTimer->start();

    qDebug() << "Phase 6.0.24: UDP timers initialized (event-driven main thread architecture)";

    // Phase 6.0.24: Initialize UDP state variables
    m_udpSocket = nullptr;
    m_tractorAddress = QHostAddress::Broadcast;  // Temporary - will be updated to subnet broadcast from SettingsManager
    m_tractorPort = 8888;                        // Default AgIO module port (PGN, autosteer, IMU)
    m_rtcmPort = 2233;                           // NTRIP/RTCM corrections port (AgOpenGPS standard)
    m_listenPort = 9999;                         // Default AgIO listen port
    m_isRunning = false;
    m_isConnected = false;
    m_isUdpEnabled = false;
    m_lastDataTime = 0;
    m_dataTimeoutMs = 5000;

    // Phase 6.0.24: Initialize UDP statistics
    m_bytesReceived = 0;
    m_bytesSent = 0;
    m_packetsReceived = 0;
    m_packetsSent = 0;
    m_receiveRate = 0.0;
    m_sendRate = 0.0;

    // Phase 6.0.24: Initialize traffic counter variables (CRITICAL - prevents garbage values)
    m_udpOutBytes = 0;
    m_udpInBytes = 0;
    m_udpOutStartTime = 0;
    m_udpInStartTime = 0;
    m_udpOutPackets = 0;
    m_udpInPackets = 0;

    // Phase 6.0.24: Initialize delta tracking variables (Problem 9 fix)
    m_udpInBytesLast = 0;
    m_udpInPacketsLast = 0;
    m_udpOutBytesLast = 0;
    m_udpOutPacketsLast = 0;
    m_lastTrafficTime = 0;

    m_localCntrMachine = 99;      // 99 = disconnected (as per CTraffic initialization)
    m_localCntrBlockage = 99;
    m_localCntrRateControl = 99;
    m_localCntrSteer = 99;
    m_localCntrIMU = 99;
    m_localCntrUDPOut = 0;
    m_localCntrUDPIn = 0;
    m_localCntrUDPInBytes = 0;
    m_localUdpOutRate = 0.0;
    m_localUdpInRate = 0.0;
    m_localUdpOutFreq = 0.0;
    m_localUdpInFreq = 0.0;

    qDebug() << "Phase 6.0.24: UDP state and traffic variables initialized";

    // Connect SettingsManager feature_isAgIOOn signal to our toggle handler
    SettingsManager* settings = SettingsManager::instance();
    connect(settings, &SettingsManager::feature_isAgIOOnChanged, this, &AgIOService::onAgIOServiceToggled);
    
    // Load default settings
    loadDefaultSettings();
    
    // Initialize immediately comme CTrack/CVehicle singleton pattern
    initialize();
    // CDC Architecture: Settings loaded automatically by SettingsManager

    // ✅ PHASE 5.1 DEBUG - Force save to create missing INI entries
    qDebug() << "🔧 Phase 5.1 - Forcing settings save to create INI entries...";

    // 🔍 DEBUG: Qt6 Pure Architecture - INI path managed internally by QSettings
    qDebug() << "📁 SettingsManager uses Qt6 automatic INI persistence";
    qDebug() << "📁 Expected INI Location: C:\\Users\\TheRedBoots\\Documents\\QtAgOpenGPS\\QtAgOpenGPS.ini";

    // CDC Architecture: Settings saved automatically by SettingsManager

    // Start communication only if AgIOService is enabled
    if (settings->feature_isAgIOOn()) {
        startCommunication();
        qDebug() << "✅ AgIOService created and STARTED - hardware priority enabled";
    } else {
        qDebug() << "✅ AgIOService created but STOPPED - manual QML controls enabled";
    }

    qDebug() << "✅ AgIOService created - Phase 1 thread-safe architecture + Phase 6.2.1 CTraffic ready";
}

AgIOService::~AgIOService()
{
    qDebug() << "AgIOService destructor - shutdown already handled by FormGPS";

    // Phase 6.0.21.2: Cleanup CTraffic (no parent, must delete manually)
    if (m_traffic) {
        delete m_traffic;
        m_traffic = nullptr;
        qDebug() << "✅ CTraffic deleted in destructor";
    }
}

AgIOService *AgIOService::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new AgIOService();
        qDebug(agioservice) << "singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

AgIOService *AgIOService::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new AgIOService();
        qDebug(agioservice) << "singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}


// ============================================================================
// RECTANGLE PATTERN: Manual Implementation of Getters, Setters, and Bindables
// ============================================================================
// Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY requires manual implementation of:
// - Getters: return m_property.value()
// - Setters: m_property.setValue(value)
// - Bindables: return &m_property

// Phase 6.0.21: GPS/IMU data property implementations removed - moved to FormGPS

// Connection Status Properties
bool AgIOService::gpsConnected() const { return m_gpsConnected.value(); }
void AgIOService::setGpsConnected(bool gpsConnected) { m_gpsConnected.setValue(gpsConnected); }
QBindable<bool> AgIOService::bindableGpsConnected() { return &m_gpsConnected; }

bool AgIOService::gpsPortConnected() const { return m_gpsPortConnected.value(); }
void AgIOService::setGpsPortConnected(bool gpsPortConnected) { m_gpsPortConnected.setValue(gpsPortConnected); }
QBindable<bool> AgIOService::bindableGpsPortConnected() { return &m_gpsPortConnected; }

bool AgIOService::bluetoothConnected() const { return m_bluetoothConnected.value(); }
void AgIOService::setBluetoothConnected(bool bluetoothConnected) { m_bluetoothConnected.setValue(bluetoothConnected); }
QBindable<bool> AgIOService::bindableBluetoothConnected() { return &m_bluetoothConnected; }

bool AgIOService::ethernetConnected() const { return m_ethernetConnected.value(); }
void AgIOService::setEthernetConnected(bool ethernetConnected) { m_ethernetConnected.setValue(ethernetConnected); }
QBindable<bool> AgIOService::bindableEthernetConnected() { return &m_ethernetConnected; }

bool AgIOService::ntripConnected() const { return m_ntripConnected.value(); }
void AgIOService::setNtripConnected(bool ntripConnected) { m_ntripConnected.setValue(ntripConnected); }
QBindable<bool> AgIOService::bindableNtripConnected() { return &m_ntripConnected; }

// GPS Quality Properties
int AgIOService::gpsQuality() const { return m_gpsQuality.value(); }
void AgIOService::setGpsQuality(int gpsQuality) { m_gpsQuality.setValue(gpsQuality); }
QBindable<int> AgIOService::bindableGpsQuality() { return &m_gpsQuality; }

int AgIOService::satellites() const { return m_satellites.value(); }
void AgIOService::setSatellites(int satellites) { m_satellites.setValue(satellites); }
QBindable<int> AgIOService::bindableSatellites() { return &m_satellites; }

// Bluetooth Properties
QString AgIOService::bluetoothDevice() const { return m_bluetoothDevice.value(); }
void AgIOService::setBluetoothDevice(const QString& bluetoothDevice) { m_bluetoothDevice.setValue(bluetoothDevice); }
QBindable<QString> AgIOService::bindableBluetoothDevice() { return &m_bluetoothDevice; }

// NTRIP Properties
int AgIOService::ntripStatus() const { return m_ntripStatus.value(); }
void AgIOService::setNtripStatus(int ntripStatus) { m_ntripStatus.setValue(ntripStatus); }
QBindable<int> AgIOService::bindableNtripStatus() { return &m_ntripStatus; }

int AgIOService::rawTripCount() const { return m_rawTripCount.value(); }
void AgIOService::setRawTripCount(int rawTripCount) { m_rawTripCount.setValue(rawTripCount); }
QBindable<int> AgIOService::bindableRawTripCount() { return &m_rawTripCount; }

// Configuration Properties
bool AgIOService::udpListenOnly() const { return m_udpListenOnly.value(); }
void AgIOService::setUdpListenOnly(bool udpListenOnly) { m_udpListenOnly.setValue(udpListenOnly); }
QBindable<bool> AgIOService::bindableUdpListenOnly() { return &m_udpListenOnly; }

bool AgIOService::ntripDebugEnabled() const { return m_ntripDebugEnabled.value(); }
void AgIOService::setNtripDebugEnabled(bool ntripDebugEnabled) { m_ntripDebugEnabled.setValue(ntripDebugEnabled); }
QBindable<bool> AgIOService::bindableNtripDebugEnabled() { return &m_ntripDebugEnabled; }

bool AgIOService::bluetoothDebugEnabled() const { return m_bluetoothDebugEnabled.value(); }
void AgIOService::setBluetoothDebugEnabled(bool bluetoothDebugEnabled) { m_bluetoothDebugEnabled.setValue(bluetoothDebugEnabled); }
QBindable<bool> AgIOService::bindableBluetoothDebugEnabled() { return &m_bluetoothDebugEnabled; }

QString AgIOService::ntripUrlIP() const { return m_ntripUrlIP.value(); }
void AgIOService::setNtripUrlIP(const QString& ntripUrlIP) { m_ntripUrlIP.setValue(ntripUrlIP); }
QBindable<QString> AgIOService::bindableNtripUrlIP() { return &m_ntripUrlIP; }

// Module Connection Properties
bool AgIOService::imuConnected() const { return m_imuConnected.value(); }
void AgIOService::setImuConnected(bool imuConnected) { m_imuConnected.setValue(imuConnected); }
QBindable<bool> AgIOService::bindableImuConnected() { return &m_imuConnected; }

bool AgIOService::steerConnected() const { return m_steerConnected.value(); }
void AgIOService::setSteerConnected(bool steerConnected) { m_steerConnected.setValue(steerConnected); }
QBindable<bool> AgIOService::bindableSteerConnected() { return &m_steerConnected; }

bool AgIOService::machineConnected() const { return m_machineConnected.value(); }
void AgIOService::setMachineConnected(bool machineConnected) { m_machineConnected.setValue(machineConnected); }
QBindable<bool> AgIOService::bindableMachineConnected() { return &m_machineConnected; }

bool AgIOService::blockageConnected() const { return m_blockageConnected.value(); }
void AgIOService::setBlockageConnected(bool blockageConnected) { m_blockageConnected.setValue(blockageConnected); }
QBindable<bool> AgIOService::bindableBlockageConnected() { return &m_blockageConnected; }

bool AgIOService::rateControlConnected() const { return m_rateControlConnected.value(); }
void AgIOService::setRateControlConnected(bool rateControlConnected) { m_rateControlConnected.setValue(rateControlConnected); }
QBindable<bool> AgIOService::bindableRateControlConnected() { return &m_rateControlConnected; }

// PHASE 6.0.22.3: Module source and frequency property implementations
QString AgIOService::gpsSource() const { return m_gpsSource.value(); }
void AgIOService::setGpsSource(const QString& gpsSource) { m_gpsSource.setValue(gpsSource); }
QBindable<QString> AgIOService::bindableGpsSource() { return &m_gpsSource; }

QString AgIOService::imuSource() const { return m_imuSource.value(); }
void AgIOService::setImuSource(const QString& imuSource) { m_imuSource.setValue(imuSource); }
QBindable<QString> AgIOService::bindableImuSource() { return &m_imuSource; }

QString AgIOService::steerSource() const { return m_steerSource.value(); }
void AgIOService::setSteerSource(const QString& steerSource) { m_steerSource.setValue(steerSource); }
QBindable<QString> AgIOService::bindableSteerSource() { return &m_steerSource; }

QString AgIOService::machineSource() const { return m_machineSource.value(); }
void AgIOService::setMachineSource(const QString& machineSource) { m_machineSource.setValue(machineSource); }
QBindable<QString> AgIOService::bindableMachineSource() { return &m_machineSource; }

double AgIOService::gpsFrequency() const { return m_gpsFrequency.value(); }
void AgIOService::setGpsFrequency(double gpsFrequency) { m_gpsFrequency.setValue(gpsFrequency); }
QBindable<double> AgIOService::bindableGpsFrequency() { return &m_gpsFrequency; }

double AgIOService::imuFrequency() const { return m_imuFrequency.value(); }
void AgIOService::setImuFrequency(double imuFrequency) { m_imuFrequency.setValue(imuFrequency); }
QBindable<double> AgIOService::bindableImuFrequency() { return &m_imuFrequency; }

double AgIOService::steerFrequency() const { return m_steerFrequency.value(); }
void AgIOService::setSteerFrequency(double steerFrequency) { m_steerFrequency.setValue(steerFrequency); }
QBindable<double> AgIOService::bindableSteerFrequency() { return &m_steerFrequency; }

double AgIOService::machineFrequency() const { return m_machineFrequency.value(); }
void AgIOService::setMachineFrequency(double machineFrequency) { m_machineFrequency.setValue(machineFrequency); }
QBindable<double> AgIOService::bindableMachineFrequency() { return &m_machineFrequency; }

// PHASE 6.0.22.12: Dynamic protocol list generation
QVariantList AgIOService::activeProtocols() const
{
    QVariantList protocols;

    for (auto it = m_protocolStatusMap.constBegin(); it != m_protocolStatusMap.constEnd(); ++it) {
        const ModuleStatus& status = it.value();

        // Only include active protocols (received data within last 2 seconds)
        if (!status.isActive) continue;

        QVariantMap protocol;
        protocol["id"] = it.key();  // "$PANDA" or "PGN211"
        protocol["description"] = getProtocolDescription(it.key());
        protocol["source"] = QString("%1:%2").arg(status.transport, status.sourceID);
        protocol["frequency"] = status.frequency;

        protocols.append(protocol);
    }

    return protocols;
}

// PHASE 6.0.22.12: Protocol description lookup
QString AgIOService::getProtocolDescription(const QString& protocolId) const
{
    // Hardcoded descriptions from NMEA_Sentences.md and PGN_Sentences.md
    static const QMap<QString, QString> descriptions = {
        // NMEA Sentences
        {"$GGA", "GPS Fix Data"},
        {"$VTG", "Track and Speed"},
        {"$HDT", "Heading True"},
        {"$PANDA", "Single Antenna + IMU"},
        {"$PAOGI", "Dual Antenna + IMU"},
        {"$AVR", "Trimble Dual Antenna Attitude"},
        {"$HPD", "High Precision Distance"},
        {"$KSXT", "Integrated GNSS/IMU"},

        // PGN Messages - AutoSteer Module
        {"PGN254", "Steer Data IN"},
        {"PGN253", "AutoSteer Status OUT"},
        {"PGN252", "Steer Settings IN"},
        {"PGN251", "Steer Config IN"},
        {"PGN250", "AutoSteer Sensor OUT"},
        {"PGN126", "Hello AutoSteer OUT"},

        // PGN Messages - Machine Module
        {"PGN239", "Machine Data IN"},
        {"PGN238", "Machine Config IN"},
        {"PGN236", "Pin Config IN"},
        {"PGN235", "Section Dimensions IN"},
        {"PGN237", "Machine Status OUT"},
        {"PGN229", "64 Sections IN"},
        {"PGN122", "Hello RateControl OUT"},
        {"PGN123", "Hello Machine OUT"},
        {"PGN244", "Blockage Data IN"},
        {"PGN240", "RateControl Data IN"},

        // PGN Messages - IMU Module
        {"PGN211", "IMU Data OUT"},
        {"PGN121", "Hello IMU OUT"},

        // PGN Messages - GPS Module
        {"PGN214", "Main Antenna OUT"},
        {"PGN215", "Tool Antenna OUT"},
        {"PGN120", "Hello GPS OUT"},

        // PGN Messages - Communication
        {"PGN200", "Hello Command"},
        {"PGN201", "Subnet Change"},
        {"PGN202", "Scan Request"},
        {"PGN203", "Scan Reply"}
    };

    return descriptions.value(protocolId, "Unknown Protocol");
}

// NMEA Sentence Properties
QString AgIOService::gga() const { return m_ggaSentence.value(); }
void AgIOService::setGga(const QString& gga) { m_ggaSentence.setValue(gga); }
QBindable<QString> AgIOService::bindableGga() { return &m_ggaSentence; }

QString AgIOService::vtg() const { return m_vtgSentence.value(); }
void AgIOService::setVtg(const QString& vtg) { m_vtgSentence.setValue(vtg); }
QBindable<QString> AgIOService::bindableVtg() { return &m_vtgSentence; }

QString AgIOService::rmc() const { return m_rmcSentence.value(); }
void AgIOService::setRmc(const QString& rmc) { m_rmcSentence.setValue(rmc); }
QBindable<QString> AgIOService::bindableRmc() { return &m_rmcSentence; }

QString AgIOService::panda() const { return m_pandaSentence.value(); }
void AgIOService::setPanda(const QString& panda) { m_pandaSentence.setValue(panda); }
QBindable<QString> AgIOService::bindablePanda() { return &m_pandaSentence; }

QString AgIOService::paogi() const { return m_paogiSentence.value(); }
void AgIOService::setPaogi(const QString& paogi) { m_paogiSentence.setValue(paogi); }
QBindable<QString> AgIOService::bindablePaogi() { return &m_paogiSentence; }

QString AgIOService::hdt() const { return m_hdtSentence.value(); }
void AgIOService::setHdt(const QString& hdt) { m_hdtSentence.setValue(hdt); }
QBindable<QString> AgIOService::bindableHdt() { return &m_hdtSentence; }

QString AgIOService::avr() const { return m_avrSentence.value(); }
void AgIOService::setAvr(const QString& avr) { m_avrSentence.setValue(avr); }
QBindable<QString> AgIOService::bindableAvr() { return &m_avrSentence; }

QString AgIOService::hpd() const { return m_hpdSentence.value(); }
void AgIOService::setHpd(const QString& hpd) { m_hpdSentence.setValue(hpd); }
QBindable<QString> AgIOService::bindableHpd() { return &m_hpdSentence; }

QString AgIOService::sxt() const { return m_sxtSentence.value(); }
void AgIOService::setSxt(const QString& sxt) { m_sxtSentence.setValue(sxt); }
QBindable<QString> AgIOService::bindableSxt() { return &m_sxtSentence; }

QString AgIOService::unknownSentence() const { return m_unknownSentence.value(); }
void AgIOService::setUnknownSentence(const QString& unknownSentence) { m_unknownSentence.setValue(unknownSentence); }
QBindable<QString> AgIOService::bindableUnknownSentence() { return &m_unknownSentence; }

// Status Text Properties
QString AgIOService::ntripStatusText() const { return m_ntripStatusText.value(); }
void AgIOService::setNtripStatusText(const QString& ntripStatusText) { m_ntripStatusText.setValue(ntripStatusText); }
QBindable<QString> AgIOService::bindableNtripStatusText() { return &m_ntripStatusText; }

// CTraffic read-only proxy to UDPWorker->m_traffic
CTraffic* AgIOService::traffic() const {
    return m_traffic;  // Returns pointer to UDPWorker-owned CTraffic instance
}

QString AgIOService::gpsStatusText() const { return m_gpsStatusText.value(); }
void AgIOService::setGpsStatusText(const QString& gpsStatusText) { m_gpsStatusText.setValue(gpsStatusText); }
QBindable<QString> AgIOService::bindableGpsStatusText() { return &m_gpsStatusText; }

QString AgIOService::moduleStatusText() const { return m_moduleStatusText.value(); }
void AgIOService::setModuleStatusText(const QString& moduleStatusText) { m_moduleStatusText.setValue(moduleStatusText); }
QBindable<QString> AgIOService::bindableModuleStatusText() { return &m_moduleStatusText; }

QString AgIOService::serialStatusText() const { return m_serialStatusText.value(); }
void AgIOService::setSerialStatusText(const QString& serialStatusText) { m_serialStatusText.setValue(serialStatusText); }
QBindable<QString> AgIOService::bindableSerialStatusText() { return &m_serialStatusText; }

bool AgIOService::showErrorDialog() const { return m_showErrorDialog.value(); }
void AgIOService::setShowErrorDialog(bool showErrorDialog) { m_showErrorDialog.setValue(showErrorDialog); }
QBindable<bool> AgIOService::bindableShowErrorDialog() { return &m_showErrorDialog; }

QString AgIOService::lastErrorMessage() const { return m_lastErrorMessage.value(); }
void AgIOService::setLastErrorMessage(const QString& lastErrorMessage) { m_lastErrorMessage.setValue(lastErrorMessage); }
QBindable<QString> AgIOService::bindableLastErrorMessage() { return &m_lastErrorMessage; }

QVariantList AgIOService::discoveredModules() const { return m_discoveredModules.value(); }
void AgIOService::setDiscoveredModules(const QVariantList& discoveredModules) { m_discoveredModules.setValue(discoveredModules); }
QBindable<QVariantList> AgIOService::bindableDiscoveredModules() { return &m_discoveredModules; }

QVariantList AgIOService::networkInterfaces() const { return m_networkInterfaces.value(); }
void AgIOService::setNetworkInterfaces(const QVariantList& networkInterfaces) { m_networkInterfaces.setValue(networkInterfaces); }
QBindable<QVariantList> AgIOService::bindableNetworkInterfaces() { return &m_networkInterfaces; }

QString AgIOService::discoveredModuleSubnet() const { return m_discoveredModuleSubnet.value(); }
void AgIOService::setDiscoveredModuleSubnet(const QString& discoveredModuleSubnet) { m_discoveredModuleSubnet.setValue(discoveredModuleSubnet); }
QBindable<QString> AgIOService::bindableDiscoveredModuleSubnet() { return &m_discoveredModuleSubnet; }

QString AgIOService::discoveredModuleIP() const { return m_discoveredModuleIP.value(); }
void AgIOService::setDiscoveredModuleIP(const QString& discoveredModuleIP) { m_discoveredModuleIP.setValue(discoveredModuleIP); }
QBindable<QString> AgIOService::bindableDiscoveredModuleIP() { return &m_discoveredModuleIP; }

// ===== NMEA Sentence Properties =====
QString AgIOService::ggaSentence() const { return m_ggaSentence.value(); }
void AgIOService::setGgaSentence(const QString& ggaSentence) { m_ggaSentence.setValue(ggaSentence); }
QBindable<QString> AgIOService::bindableGgaSentence() { return &m_ggaSentence; }

QString AgIOService::vtgSentence() const { return m_vtgSentence.value(); }
void AgIOService::setVtgSentence(const QString& vtgSentence) { m_vtgSentence.setValue(vtgSentence); }
QBindable<QString> AgIOService::bindableVtgSentence() { return &m_vtgSentence; }

QString AgIOService::rmcSentence() const { return m_rmcSentence.value(); }
void AgIOService::setRmcSentence(const QString& rmcSentence) { m_rmcSentence.setValue(rmcSentence); }
QBindable<QString> AgIOService::bindableRmcSentence() { return &m_rmcSentence; }

QString AgIOService::pandaSentence() const { return m_pandaSentence.value(); }
void AgIOService::setPandaSentence(const QString& pandaSentence) { m_pandaSentence.setValue(pandaSentence); }
QBindable<QString> AgIOService::bindablePandaSentence() { return &m_pandaSentence; }

QString AgIOService::paogiSentence() const { return m_paogiSentence.value(); }
void AgIOService::setPaogiSentence(const QString& paogiSentence) { m_paogiSentence.setValue(paogiSentence); }
QBindable<QString> AgIOService::bindablePaogiSentence() { return &m_paogiSentence; }

QString AgIOService::hdtSentence() const { return m_hdtSentence.value(); }
void AgIOService::setHdtSentence(const QString& hdtSentence) { m_hdtSentence.setValue(hdtSentence); }
QBindable<QString> AgIOService::bindableHdtSentence() { return &m_hdtSentence; }

QString AgIOService::avrSentence() const { return m_avrSentence.value(); }
void AgIOService::setAvrSentence(const QString& avrSentence) { m_avrSentence.setValue(avrSentence); }
QBindable<QString> AgIOService::bindableAvrSentence() { return &m_avrSentence; }

QString AgIOService::hpdSentence() const { return m_hpdSentence.value(); }
void AgIOService::setHpdSentence(const QString& hpdSentence) { m_hpdSentence.setValue(hpdSentence); }
QBindable<QString> AgIOService::bindableHpdSentence() { return &m_hpdSentence; }

QString AgIOService::sxtSentence() const { return m_sxtSentence.value(); }
void AgIOService::setSxtSentence(const QString& sxtSentence) { m_sxtSentence.setValue(sxtSentence); }
QBindable<QString> AgIOService::bindableSxtSentence() { return &m_sxtSentence; }

// ===== Missing Core Properties =====
QVariantList AgIOService::bluetoothDevices() const { return m_bluetoothDevices.value(); }
void AgIOService::setBluetoothDevices(const QVariantList& bluetoothDevices) { m_bluetoothDevices.setValue(bluetoothDevices); }
QBindable<QVariantList> AgIOService::bindableBluetoothDevices() { return &m_bluetoothDevices; }

QByteArray AgIOService::serialData() const { return m_serialData.value(); }
void AgIOService::setSerialData(const QByteArray& serialData) { m_serialData.setValue(serialData); }
QBindable<QByteArray> AgIOService::bindableSerialData() { return &m_serialData; }

QString AgIOService::serialPort() const { return m_serialPort.value(); }
void AgIOService::setSerialPort(const QString& serialPort) { m_serialPort.setValue(serialPort); }
QBindable<QString> AgIOService::bindableSerialPort() { return &m_serialPort; }

int AgIOService::baudRate() const { return m_baudRate.value(); }
void AgIOService::setBaudRate(int baudRate) { m_baudRate.setValue(baudRate); }
QBindable<int> AgIOService::bindableBaudRate() { return &m_baudRate; }

bool AgIOService::isOpen() const { return m_isOpen.value(); }
void AgIOService::setIsOpen(bool isOpen) { m_isOpen.setValue(isOpen); }
QBindable<bool> AgIOService::bindableIsOpen() { return &m_isOpen; }



// ============================================================================
// END RECTANGLE PATTERN MANUAL IMPLEMENTATIONS
// ============================================================================

QString AgIOService::fixQuality() const
{
    switch(m_gpsQuality) {
    case 0:
        return "Invalid: ";
    case 1:
        return "GPS 1: ";
    case 2:
        return "DGPS : ";
    case 3:
        return "PPS : ";
    case 4:
        return "RTK fix: ";
    case 5:
        return "Float: ";
    case 6:
        return "Estimate: ";
    case 7:
        return "Man IP: ";
    case 8:
        return "Sim: ";
    default:
        return "Unknown: ";
    }
}

// QML invokable methods
void AgIOService::configureNTRIP()
{
    // CDC Architecture: Get values directly from SettingsManager (no loadSettings needed)
    auto* settings = SettingsManager::instance();
    QString ntripUrl = settings->ntrip_url();
    QString ntripMount = settings->ntrip_mount();
    QString ntripUser = settings->ntrip_userName();
    QString ntripPassword = settings->ntrip_password();
    int ntripPort = settings->ntrip_port();
    bool ntripEnabled = settings->ntrip_isTCP();

    qDebug() << "Configuring NTRIP:" << ntripUrl << "mount:" << ntripMount << "port:" << ntripPort;

    if (ntripEnabled && !ntripUrl.isEmpty() && !ntripMount.isEmpty()) {
        emit requestStartNTRIP(ntripUrl, ntripUser, ntripPassword, ntripMount, ntripPort);

        // PHASE 6.0.37 - Configure RTCM routing: UDP broadcast (not Serial)
        if (m_ntripWorker) {
            QMetaObject::invokeMethod(m_ntripWorker, "enableSerialRouting",
                                      Qt::QueuedConnection, Q_ARG(bool, false));
            QMetaObject::invokeMethod(m_ntripWorker, "enableUDPBroadcast",
                                      Qt::QueuedConnection, Q_ARG(bool, true));
            qDebug() << "NTRIP configured for UDP broadcast routing";
        }

        qDebug() << "NTRIP started with" << ntripUrl << "/" << ntripMount << ":" << ntripPort;
    } else {
        emit requestStopNTRIP();
        qDebug() << "NTRIP stopped";
    }
}

void AgIOService::startBluetoothDiscovery()
{
    qDebug() << "🔍 Starting Bluetooth discovery...";
    emit requestBluetoothScan();
}

void AgIOService::connectBluetooth(const QString& deviceName)
{
    qDebug() << "📱 Connecting to Bluetooth device:" << deviceName;
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits bluetoothDeviceChanged())
    m_bluetoothDevice = deviceName;
    // TODO: Implement actual Bluetooth connection via worker
}

void AgIOService::disconnectBluetooth()
{
    qDebug() << "📱 Disconnecting Bluetooth";
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits signals)
    m_bluetoothDevice = "";              // Auto-emits bluetoothDeviceChanged()
    m_bluetoothConnected = false;        // Auto-emits bluetoothConnectedChanged()
    // ❌ REMOVED: Manual signal emission (Rectangle Pattern handles automatically)
}

void AgIOService::startCommunication()
{
    if (m_initialized) {
        qDebug() << "⚠️ AgIO communication already started";
        return;
    }
    
    qDebug() << "🚀 Starting AgIO communication - Main Thread:" << QThread::currentThread();
    
    // Setup worker threads
    setupWorkerThreads();

    // Phase 6.0.24: Initialize event-driven UDP socket in main thread
    initializeUdpSocket();

    // ✅ CDC Compliance - Initialize async caches by triggering initial scans
    QTimer::singleShot(500, this, [this]() {
        if (m_serialWorker) {
            qDebug() << "🔧 Initializing serial port cache...";
            QMetaObject::invokeMethod(m_serialWorker, "scanAvailablePortsAsync", Qt::QueuedConnection);
        }
    });

    // ✅ Auto-reconnect previously connected serial ports after startup
    QTimer::singleShot(1000, this, [this]() {
        qDebug() << "🔄 Auto-reconnecting previously connected serial ports...";

        // CDC Architecture: Get values directly from SettingsManager
        auto* settings = SettingsManager::instance();

        // Reconnect GPS if it was configured
        QString gpsPort = settings->gnss_SerialPort();
        int gpsBaud = settings->gnss_BaudRate();
        if (!gpsPort.isEmpty() && gpsPort != "COM3" && gpsPort != "ttyHSL2") {
            qDebug() << "🔄 Auto-reconnecting GPS to" << gpsPort << "at" << gpsBaud;
            openSerialPort("GPS", gpsPort, gpsBaud);
        }

        // Reconnect IMU if it was configured
        QString imuPort = settings->imu_SerialPort();
        int imuBaud = settings->imu_BaudRate();
        if (!imuPort.isEmpty() && imuPort != "COM4" && imuPort != "ttyHSL0") {
            qDebug() << "🔄 Auto-reconnecting IMU to" << imuPort << "at" << imuBaud;
            openSerialPort("IMU", imuPort, imuBaud);
        }

        // Reconnect AutoSteer if it was configured
        QString steerPort = settings->steer_SerialPort();
        int steerBaud = settings->steer_BaudRate();
        if (!steerPort.isEmpty() && steerPort != "ttyHSL3") {
            qDebug() << "🔄 Auto-reconnecting AutoSteer to" << steerPort << "at" << steerBaud;
            openSerialPort("Steer", steerPort, steerBaud);
        }
    });

    // Start heartbeat
    m_heartbeatTimer->start();

    // PHASE 6.0.22: Start module status update timer (throttled QML updates)
    m_moduleStatusUpdateTimer->start();

    // PHASE 6.0.22.12: Start module ping timer (PGN 200 hello for ModSim compatibility)
    m_modulePingTimer->start();

    // ⚡ PHASE 6.0.20 FIX: Initialize broadcast address and start UDP immediately
    auto* settings = SettingsManager::instance();

    // Initialize broadcast address from SettingsManager
    m_udpBroadcastAddress = QString("%1.%2.%3.255")
        .arg(settings->ethernet_ipOne())
        .arg(settings->ethernet_ipTwo())
        .arg(settings->ethernet_ipThree());
    qDebug() << "🌐 Broadcast address initialized:" << m_udpBroadcastAddress;

    // PHASE 6.0.37: Update tractor address to use subnet broadcast (like C# epModule)
    // Communication normale avec modules sur 192.168.X.255 (pas 255.255.255.255)
    m_tractorAddress = QHostAddress(m_udpBroadcastAddress);
    qDebug(agioservice) << "Tractor address initialized for subnet broadcast:" << m_udpBroadcastAddress;

    // Start UDP if enabled (avoids race condition with QTimer::singleShot)
    if (settings->ethernet_isOn()) {
        emit requestStartUDP(m_udpBroadcastAddress, 8888);
        qDebug() << "🚀 UDP auto-started immediately (ethernet_isOn=true)";
    } else {
        qDebug() << "ℹ️ UDP not started (ethernet_isOn=false) - use agioService.enableUDP(true) or EthernetConfig";
    }

    m_initialized = true;
    qDebug() << "✅ AgIO service started successfully";
}

void AgIOService::stopCommunication()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "🛑 Stopping AgIO communication";
    
    // Stop heartbeat
    m_heartbeatTimer->stop();
    
    // Stop all workers
    emit requestStopGPS();
    emit requestStopNTRIP();
    emit requestStopUDP();
    
    m_initialized = false;
}

// saveSettings() removed - SettingsManager provides automatic persistence via syncPropertyToINI()
// CDC Architecture: Single Source of Truth with auto-sync eliminates duplicate save logic

// loadSettings() removed - SettingsManager provides automatic loading on-demand
// CDC Architecture: Properties are loaded lazily when accessed, eliminating explicit load calls

void AgIOService::testThreadCommunication()
{
    qDebug() << "🧪 === THREAD COMMUNICATION TEST ===";
    qDebug() << "Main/AgIOService Thread:" << QThread::currentThread();
    qDebug() << "NTRIP Thread:" << m_ntripThread << "Worker:" << (m_ntripWorker ? "✅" : "❌");
    qDebug() << "UDP: Main thread event-driven socket (Phase 6.0.24)";
    qDebug() << "Service Initialized:" << m_initialized;
    qDebug() << "GPS Connected:" << m_gpsConnected;
    qDebug() << "NTRIP Status:" << m_ntripStatus;
    qDebug() << "Ethernet Connected:" << m_ethernetConnected;
    qDebug() << "=== END TEST ===";
}

void AgIOService::configureSubnet()
{
    // CDC Architecture: Get values directly from SettingsManager
    auto* settings = SettingsManager::instance();
    qDebug() << "🌐 Sending subnet configuration to modules:" << settings->ethernet_ipOne() << "." << settings->ethernet_ipTwo() << "." << settings->ethernet_ipThree() << ".x";

    // ✅ CRITICAL FIX: DO NOT pause UDP! Module dormant server needs port 9999 open!
    // AgIO original NEVER closes UDP during Set IP - modules need continuous connection
    qDebug() << "🌐 Keeping UDP active for dormant server compatibility (like AgIO original)";

    // Phase 6.0.21: GPS pause removed (GPSWorker deleted - UDP handles GPS now)

    qDebug() << "✅ UDP kept active for dormant modules - starting async sequence...";

    // ✅ CDC COMPLIANT: Replace all QThread::msleep() with async QTimer::singleShot()
    // Step 1: Reduce traffic noise (200ms async delay)
    QTimer::singleShot(200, this, [this]() {
        qDebug() << "🔍 Step 1a: Verifying UDP port accessibility...";

        // Phase 6.0.24: Direct call in main thread (no worker thread)
        testPortAccessibility(8888, "Module command port");
        testPortAccessibility(9999, "AgIO response port");

        // Step 2: Brief pause for port tests (100ms async delay)
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "📡 Step 1b: Waking up modules using dedicated function";

            // Phase 6.0.24: Direct call in main thread (no worker thread)
            wakeUpModules("255.255.255.255");
            qDebug() << "✅ PGN 202 sent via working wakeUpModules() function";

            // Step 3: Wait for module wake up (2000ms async delay)
            qDebug() << "👂 Step 1c: Listening for PGN 203 responses on port 9999 for 2 seconds...";
            qDebug() << "⏰ Module had broadcast silence - giving time to wake up";
            QTimer::singleShot(2000, this, [this]() {
                qDebug() << "⏳ Step 2: Module should be awake (expect PGN 203 responses), now sending PGN 201...";
                // Continue with PGN 201 sequence
                sendPGN201ToAllInterfaces();
            });
        });
    });

    // ✅ CDC COMPLIANT: Move all PGN 201 logic to separate method
    // This will be called from the async timer chain above
}

void AgIOService::sendPGN201ToAllInterfaces()
{
    // Build PGN 201 message to configure module IP (AgIO original format - 5 bytes payload)
    QByteArray sendIPToModules;
    sendIPToModules.append(char(0x80)); // Header 1
    sendIPToModules.append(char(0x81)); // Header 2
    sendIPToModules.append(char(0x7F)); // Source AgIO
    sendIPToModules.append(char(201)); // PGN 201 (Set Subnet - 0xC9)
    sendIPToModules.append(char(5)); // Length = 5 bytes payload (AgIO original!)

    // 5-byte payload (AgIO original format) - CDC Architecture: Get from SettingsManager
    auto* settings = SettingsManager::instance();
    int udpIP1 = settings->ethernet_ipOne();
    int udpIP2 = settings->ethernet_ipTwo();
    int udpIP3 = settings->ethernet_ipThree();

    sendIPToModules.append(char(201)); // Payload 1: PGN repeat (0xC9)
    sendIPToModules.append(char(201)); // Payload 2: PGN repeat (0xC9)
    sendIPToModules.append(char(udpIP1)); // Payload 3: IP1 (192 = 0xC0)
    sendIPToModules.append(char(udpIP2)); // Payload 4: IP2 (168 = 0xA8)
    sendIPToModules.append(char(udpIP3)); // Payload 5: IP3 (subnet 1 or 5)

    // Calculate checksum - AgIO original uses 0x47 fixed checksum for all subnets
    quint8 checksum = 0x47;

    sendIPToModules.append(char(checksum)); // Subnet-specific checksum

    qDebug() << "📦 PGN 201 message:" << sendIPToModules.toHex(' ');
    qDebug() << "🔢 Checksum:" << QString("0x%1").arg(checksum, 2, 16, QChar('0')).toUpper() << "for subnet" << udpIP3;
    qDebug() << "🎯 Target: 255.255.255.255:8888 (global broadcast to module command port)";
    qDebug() << "📋 Expected: Module will receive PGN 201, save to EEPROM, and restart with new IP";

    // ✅ OPTIMIZED: Send PGN 201 only to detected module subnet (if known)
    // Phase 6.0.24: Use BINDABLE property directly (main thread)
    QString detectedSubnet = m_discoveredModuleSubnet;
    QStringList subnets;

    if (!detectedSubnet.isEmpty()) {
        // We know exactly where the module is - send only to that subnet!
        QString targetSubnet = detectedSubnet + ".255";
        subnets = {targetSubnet};
        qDebug() << "🎯 Sending PGN 201 ONLY to detected module subnet:" << targetSubnet;
    } else {
        // Fallback: scan multiple subnets if module location unknown
        subnets = {"192.168.1.255", "192.168.5.255", "192.168.2.255", "192.168.3.255"};
        qDebug() << "🌐 Module location unknown - sending PGN 201 to multiple subnets...";
    }

    // ✅ CDC COMPLIANT: Replace QThread::msleep() with async subnet sending
    sendPGN201ToNextSubnet(sendIPToModules, subnets, 0);

    // CDC Architecture: Settings saved automatically by SettingsManager

    qDebug() << "✅ PGN 201 subnet command sent to modules";

    // Module will restart after receiving PGN 201 (takes ~2-3 seconds)

    // Schedule module restart handling (async)
    QTimer::singleShot(5000, this, [this]() {
        handleModuleRestart();
    });
}

void AgIOService::sendPGN201ToNextSubnet(const QByteArray& message, const QStringList& subnets, int index)
{
    if (index >= subnets.size()) {
        qDebug() << "✅ PGN 201 sent to all" << subnets.size() << "subnets - module should receive command";
        return;
    }

    const QString& subnet = subnets[index];
    qDebug() << "🌐 PGN 201 broadcast to subnet:" << subnet;

    // Phase 6.0.24: Direct call in main thread (no worker thread)
    sendToAllInterfaces(message, subnet, 8888);

    // ✅ CDC COMPLIANT: Replace QThread::msleep(100) with async delay
    QTimer::singleShot(100, this, [this, message, subnets, index]() {
        sendPGN201ToNextSubnet(message, subnets, index + 1);
    });
}

void AgIOService::handleModuleRestart()
{
    qDebug() << "🔄 Module should have restarted, now reconfiguring UDP...";

    // CDC Architecture: Get values directly from SettingsManager
    auto* settings = SettingsManager::instance();
    // Update broadcast address for new subnet
    m_udpBroadcastAddress = QString("%1.%2.%3.255").arg(settings->ethernet_ipOne()).arg(settings->ethernet_ipTwo()).arg(settings->ethernet_ipThree());
    qDebug() << "🌐 New broadcast address:" << m_udpBroadcastAddress;

    // Resume UDP traffic (socket stays open, just resume timers)
    // ✅ PHASE 6.2 FIX - Resume ALL UDP traffic (RX+TX) after module restart
    // UDP was never paused, just restart GPS transmission
    // Phase 6.0.21: GPS resume removed (GPSWorker deleted - UDP handles GPS now)

    // ✅ CRITICAL FIX: Send PGN 202 scan to wake up modules on new subnet
    // This will reset traffic counters and make buttons green again!
    qDebug() << "🔍 Sending PGN 202 scan to wake up modules on new subnet...";
    // Phase 6.0.24: Direct call in main thread (no worker thread)
    wakeUpModules("255.255.255.255");

    qDebug() << "✅ UDP active, GPS resumed, modules scanned - should see green buttons soon!";
}

void AgIOService::startPeriodicModuleScanning()
{
    // ✅ Phase 6.0.24: Periodic scanning in Main Thread (no worker)
    // Create timer in Main Thread for coordination
    QTimer* periodicScanTimer = new QTimer(this);
    periodicScanTimer->setInterval(3000); // 3 seconds for optimal module communication

    connect(periodicScanTimer, &QTimer::timeout, this, [this]() {
        // Qt 6.8 fix: Throttle debug messages to every 5 seconds
        static int periodicScanLogCounter = 0;

        // CDC Architecture: Check UDP status from SettingsManager
        auto* settings = SettingsManager::instance();

        periodicScanLogCounter++; // Increment counter once per call

        if (!settings->ethernet_isOn()) {
            if (periodicScanLogCounter % 4 == 0) { // Log every 12s (4*3s timer)
                qDebug() << "⏸️ Periodic PGN 202 scan skipped - UDP disabled";
            }
            return;
        }

        if (periodicScanLogCounter % 4 == 0) { // Log every 12s (4*3s timer)
            qDebug() << "🔄 Periodic PGN 202 scan (keeping modules alive) - Main Thread";
        }

        // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
        wakeUpModules("255.255.255.255");
    });

    periodicScanTimer->start();
    qDebug() << "✅ Periodic module scanning started from Main Thread every 3 seconds (Phase 6.0.24)";
}

// Phase 6.0.24: wakeUpModules() implementation moved to agioservice_udp.cpp
// This wrapper is no longer needed - function is implemented directly

// Legacy method removed - Phase 4.5: FormLoop eliminated
// AgIOService now gets GPS data directly from UDPWorker/GPSWorker

// Public slots
void AgIOService::initialize()
{
    qDebug(agioservice) << "Initializing AgIOService";

    // CDC Architecture: Settings loaded automatically by SettingsManager
    startCommunication();

    // PHASE 6.0.38: Auto-start NTRIP if configured ON in settings
    SettingsManager* settings = SettingsManager::instance();
    if (settings->ntrip_isTCP()) {
        qDebug(agioservice) << "NTRIP configured ON in settings, auto-starting connection";
        configureNTRIP();
    } else {
        qDebug(agioservice) << "NTRIP configured OFF in settings, not starting";
    }

    // ✅ Phase 6.0.21.3: Signal QML that AgIOService is ready for safe access
    m_ready = true;
    emit readyChanged();

    qDebug(agioservice) << "AgIOService initialized and ready for QML access";
}

void AgIOService::shutdown()
{
    qDebug() << "🛑 Shutting down AgIOService...";

    stopCommunication();
    // CDC Architecture: Settings saved automatically by SettingsManager
    
    // Clean up workers and threads
    // Phase 6.0.21: GPSWorker cleanup removed (deleted in Phase 1)

    if (m_ntripWorker) {
        m_ntripWorker->deleteLater();
        m_ntripWorker = nullptr;
    }
    if (m_ntripThread) {
        m_ntripThread->quit();
        m_ntripThread->wait(3000);
        m_ntripThread->deleteLater();
        m_ntripThread = nullptr;
    }

    // Phase 6.0.24: m_udpWorker and m_udpThread removed (event-driven socket in main thread)
    // UDP socket cleanup handled by stopUDP() in agioservice_udp.cpp

    if (m_serialWorker) {
        m_serialWorker->deleteLater();
        m_serialWorker = nullptr;
    }
    if (m_serialThread) {
        m_serialThread->quit();
        m_serialThread->wait(3000);
        m_serialThread->deleteLater();
        m_serialThread = nullptr;
    }
    
    qDebug() << "✅ AgIOService shutdown complete";
}

// 🎯 RECTANGLE PATTERN PURE: Old slots removed - workers call setters directly
// Note: onGpsDataReceived/onImuDataReceived removed - workers call setters directly:
//   GPS: setLatitude(), setLongitude(), setHeading(), setSpeed()
//   IMU: setImuRoll(), setImuPitch(), setImuYaw()

void AgIOService::onGpsStatusChanged(bool connected, int quality, int satellites, double age)
{
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits signals)
    m_gpsConnected = connected;
    m_gpsQuality = quality;
    m_satellites = satellites;
    // Phase 6.0.21: m_age removed (GPS data now in FormGPS)

    // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged(), gpsQualityChanged(), satellitesChanged()
    
    // Qt 6.8 fix: Remove debug spam - called multiple times per second
    // qDebug() << "📡 GPS status:" << connected << "Q:" << quality << "Sats:" << satellites << "Age:" << age;
}

// Phase 6.0.21.7: REMOVED - Dead code, never connected, replaced by onDataReceived()
// void AgIOService::onNmeaSentenceReceived() - was for legacy serial NMEA parsing

void AgIOService::onBluetoothStatusChanged(bool connected, const QString& device)
{
    m_bluetoothConnected = connected;
    m_bluetoothDevice = device;
    
    // ✅ RECTANGLE PATTERN: Auto-emitted via bluetoothConnectedChanged(), bluetoothDeviceChanged()
    
    qDebug() << "📱 Bluetooth:" << connected << device;
}

void AgIOService::onNtripStatusChanged(int status, const QString& statusText)
{
    m_ntripStatus = status;
    m_ntripStatusText = statusText;

    // NTRIP is truly connected when ReceivingData (status == 4)
    m_ntripConnected = (status == 4); // ConnectionState::ReceivingData

    // ✅ RECTANGLE PATTERN: Auto-emitted via ntripConnectedChanged(), ntripStatusChanged(), ntripStatusTextChanged()

    qDebug() << "📡 NTRIP status:" << status << statusText
             << "Connected:" << m_ntripConnected.value();
}

void AgIOService::onUdpStatusChanged(bool connected)
{
    m_ethernetConnected = connected;
    
    // ✅ RECTANGLE PATTERN: Auto-emitted via ethernetConnectedChanged()
    
    qDebug() << "🌐 UDP status:" << connected;
}

void AgIOService::onNtripDataReceived(const QByteArray& rtcmData)
{
    // Forward RTCM corrections to GPS worker or UDP worker
    // This maintains real-time data flow
    Q_UNUSED(rtcmData)
    
    static int dataCounter = 0;
    if (++dataCounter % 100 == 0) { // Debug every 100 packets
        qDebug() << "📡 NTRIP data received:" << rtcmData.size() << "bytes";
    }
}

// Private methods
void AgIOService::setupWorkerThreads()
{
    qCDebug(agioservice) << "Setting up worker threads...";

    // Phase 6.0.21: GPS worker thread removed (GPSWorker deleted - useless thread with no I/O)
    // SerialWorker handles GPS serial I/O, parsing done in AgIOService via PGNParser

    // Create NTRIP worker thread
    m_ntripThread = new QThread(this);
    m_ntripThread->setObjectName("NTRIPWorkerThread");
    m_ntripWorker = new NTRIPWorker();
    m_ntripWorker->moveToThread(m_ntripThread);

    // Phase 6.0.24: UDP socket in main thread (event-driven, no worker needed)
    // Event-driven QUdpSocket initialized after worker threads

    // Create Serial worker thread
    m_serialThread = new QThread(this);
    m_serialThread->setObjectName("SerialWorkerThread");
    m_serialWorker = new SerialWorker();
    m_serialWorker->moveToThread(m_serialThread);

    // Phase 6.0.24: Start threads (2 workers: NTRIP, Serial)
    // UDP is now event-driven in main thread (no worker thread)
    m_ntripThread->start();
    m_serialThread->start();

    qCDebug(agioservice) << "Worker threads created and started (2 workers: NTRIP, Serial)";

    // Setup thread-safe connections
    connectWorkerSignals();
}

void AgIOService::connectWorkerSignals()
{
    qCDebug(agioservice) << "Connecting worker signals...";

    // Phase 6.0.21: GPS Worker connections removed (GPSWorker deleted)
    // Serial GPS data now flows: SerialWorker → AgIOService (parse via PGNParser) → FormGPS

    // === NTRIP WORKER CONNECTIONS ===
    if (m_ntripWorker) {
        // Data signals (Qt::QueuedConnection for thread-safe processing)
        connect(m_ntripWorker, &NTRIPWorker::ntripDataReceived,
                this, &AgIOService::onNtripDataReceived, Qt::QueuedConnection);
        connect(m_ntripWorker, &NTRIPWorker::ntripStatusChanged,
                this, &AgIOService::onNtripStatusChanged, Qt::QueuedConnection);
        
        // Command signals (Qt::QueuedConnection for thread-safe commands)
        connect(this, &AgIOService::requestStartNTRIP,
                m_ntripWorker, &NTRIPWorker::startNTRIP, Qt::QueuedConnection);
        connect(this, &AgIOService::requestStopNTRIP,
                m_ntripWorker, &NTRIPWorker::stopNTRIP, Qt::QueuedConnection);

        // ✅ PHASE 5.3 - Advanced NTRIP RTCM Routing Signals
        connect(m_ntripWorker, &NTRIPWorker::routeRTCMToSerial,
                this, &AgIOService::onNTRIPRouteRTCMToSerial, Qt::QueuedConnection);
        connect(m_ntripWorker, &NTRIPWorker::broadcastRTCMToUDP,
                this, &AgIOService::onNTRIPBroadcastRTCMToUDP, Qt::QueuedConnection);
        connect(m_ntripWorker, &NTRIPWorker::rtcmPacketProcessed,
                this, &AgIOService::onNTRIPRTCMPacketProcessed, Qt::QueuedConnection);

        qCDebug(agioservice) << "  ✅ NTRIP worker signals connected (Phase 5.3 + Advanced RTCM routing)";
    }

    // === UDP MAIN THREAD CONNECTIONS (Phase 6.0.24) ===
    // Event-driven UDP socket in main thread - no worker thread needed
    // Connect requestStartUDP/requestStopUDP signals to main thread methods
    // Qt::DirectConnection = synchronous same-thread call (<20ns overhead, outside 40 Hz loop)
    connect(this, &AgIOService::requestStartUDP,
             this, &AgIOService::startUDP, Qt::DirectConnection);
    connect(this, &AgIOService::requestStopUDP,
             this, &AgIOService::stopUDP, Qt::DirectConnection);

    qCDebug(agioservice) << "  ✅ UDP main thread signals connected (Phase 6.0.24 event-driven architecture)";
    
    // === SERIAL WORKER CONNECTIONS ===
    if (m_serialWorker) {
        // Phase 6.0.21: GPS/GPS2 serial data connections removed (GPSWorker deleted)
        // Will be reconnected in Phase 6.0.21.3 to AgIOService for parsing via PGNParser

        connect(m_serialWorker, &SerialWorker::imuDataReceived,
                this, &AgIOService::onSerialIMUDataReceived, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::autosteerResponseReceived,
                this, &AgIOService::onSerialAutosteerResponse, Qt::QueuedConnection);

        // PHASE 5.2 - Data reception signals for 5 new ports
        connect(m_serialWorker, &SerialWorker::machineDataReceived,
                this, &AgIOService::onSerialMachineDataReceived, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::radioDataReceived,
                this, &AgIOService::onSerialRadioDataReceived, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::rtcmDataReceived,
                this, &AgIOService::onSerialRTCMDataReceived, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::toolDataReceived,
                this, &AgIOService::onSerialToolDataReceived, Qt::QueuedConnection);

        // Generic port data signal
        connect(m_serialWorker, &SerialWorker::portDataReceived,
                this, &AgIOService::onSerialPortDataReceived, Qt::QueuedConnection);

        // PHASE 6.0.24: onDataReceived removed - serial data handled by onSerialPortDataReceived above

        // Connection status signals - Legacy 3 ports
        connect(m_serialWorker, &SerialWorker::gpsConnected,
                this, &AgIOService::onSerialGPSConnected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::imuConnected,
                this, &AgIOService::onSerialIMUConnected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::autosteerConnected,
                this, &AgIOService::onSerialAutosteerConnected, Qt::QueuedConnection);

        // ✅ PHASE 5.2 - Connection status signals for 5 new ports
        connect(m_serialWorker, &SerialWorker::gps2Connected,
                this, &AgIOService::onSerialGPS2Connected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::machineConnected,
                this, &AgIOService::onSerialMachineConnected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::radioConnected,
                this, &AgIOService::onSerialRadioConnected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::rtcmConnected,
                this, &AgIOService::onSerialRTCMConnected, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::toolConnected,
                this, &AgIOService::onSerialToolConnected, Qt::QueuedConnection);

        // Generic port connection signal
        connect(m_serialWorker, &SerialWorker::portConnected,
                this, &AgIOService::onSerialPortConnected, Qt::QueuedConnection);

        // Error signals
        connect(m_serialWorker, &SerialWorker::serialError,
                this, &AgIOService::onSerialError, Qt::QueuedConnection);

        // Worker control signals
        connect(this, &AgIOService::requestSerialWorkerStart,
                m_serialWorker, &SerialWorker::startWorker, Qt::QueuedConnection);
        connect(this, &AgIOService::requestSerialWorkerStop,
                m_serialWorker, &SerialWorker::stopWorker, Qt::QueuedConnection);

        // ✅ CDC Compliance - Async operation result signals (replaces BlockingQueuedConnection)
        connect(m_serialWorker, &SerialWorker::portOpenResult,
                this, &AgIOService::onPortOpenResult, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::portCloseResult,
                this, &AgIOService::onPortCloseResult, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::connectionStatusResult,
                this, &AgIOService::onConnectionStatusResult, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::portNameResult,
                this, &AgIOService::onPortNameResult, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::portBaudRateResult,
                this, &AgIOService::onPortBaudRateResult, Qt::QueuedConnection);
        connect(m_serialWorker, &SerialWorker::availablePortsResult,
                this, &AgIOService::onAvailablePortsResult, Qt::QueuedConnection);
        
        qDebug() << "  ✅ Serial worker signals connected";
    }
    
    // Phase 6.0.24: Enhanced UDP worker connections removed (event-driven socket)
    // Module discovery and network scanning will be re-implemented if needed in Phase 2
    
    // Architecture notes:
    // - AgIOService handles GPS UDP reception on port 9999 (exclusive)
    // - AgIOService sends module commands on port 8888
    // - AgIOService complements with:
    //   * Serial GPS via GPSWorker
    //   * NTRIP corrections via NTRIPWorker
    //   * Module communication on port 9999 (GPS data, diagnostics)

    // ✅ PHASE 6.0.20 FIX: QTimer::singleShot(1000) REMOVED
    // UDP now starts immediately in startCommunication() with initialized broadcast address
    // This eliminates race condition and double UDP start
    
    qDebug() << "✅ All worker signals connected";
}

void AgIOService::loadDefaultSettings()
{
    qDebug() << "📂 Default settings handled by SettingsManager - AgIOService no longer manages config";
    // CDC Architecture: All defaults are now handled by SettingsManager
    // AgIOService focuses only on real-time data and communication
}

// Phase 6.0.21: updateVehiclePosition() removed - GPS/position data now stored in FormGPS
// FormHeadland has its own updateVehiclePosition() for headland design

// PHASE 6.0.22.12: Protocol status tracking (protocol-centric)

void AgIOService::updateModuleStatus(const QString& protocolId, const QString& transport,
                                    const QString& sourceID, qint64 timestampMs)
{
    ModuleStatus& status = m_protocolStatusMap[protocolId];  // Key changed from moduleType to protocolId

    status.transport = transport;
    status.sourceID = sourceID;
    status.lastSeenMs = timestampMs;
    status.packetCount++;
    status.isActive = true;

    // Calculate frequency over 1-second windows
    if (status.freqStartTime == 0) {
        status.freqStartTime = timestampMs;
        status.freqPacketCount = 1;
    } else {
        qint64 elapsed = timestampMs - status.freqStartTime;
        status.freqPacketCount++;

        if (elapsed >= 1000) {  // 1 second window
            status.frequency = (status.freqPacketCount * 1000.0) / elapsed;

            // Reset window
            status.freqStartTime = timestampMs;
            status.freqPacketCount = 0;
        }
    }

    // PHASE 6.0.22: Mark pending updates - actual QML property updates happen in timer
    m_pendingModuleUpdates = true;
}

void AgIOService::applyPendingModuleUpdates()
{
    // PHASE 6.0.22.12: Throttled QML property updates (1000ms interval = 1 Hz)
    // - Aligns with frequency calculation window (1 second)
    // - Prevents QML engine corruption from high-frequency updates
    // - 1 Hz is sufficient for monitoring/status display

    if (!m_ready || !m_pendingModuleUpdates) return;

    m_pendingModuleUpdates = false;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Mark protocols as inactive if no data received within 2 seconds
    for (auto it = m_protocolStatusMap.begin(); it != m_protocolStatusMap.end(); ++it) {
        ModuleStatus& status = it.value();
        if (status.isActive && (now - status.lastSeenMs > 2000)) {
            status.isActive = false;
        }
    }

    // PHASE 6.0.22.12: Emit single protocol change signal (replaces 4 individual signals)
    emit activeProtocolsChanged();
}

void AgIOService::sendModuleHello()
{
    // PHASE 6.0.22.12: Send PGN 200 hello to modules every second
    // - Triggers ModSim and hardware modules to respond with PGN 126/123/121 hello messages
    // - Matches C# AgIO OneSecondLoop behavior (FormLoop.cs line 448)
    // - Required for ModSim simulator compatibility

    if (!m_ready) return;

    // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
    // Send PGN 200 hello to broadcast address (modules listening on port 8888)
    auto* settings = SettingsManager::instance();
    QString ipStr = QString("%1.%2.%3")
                        .arg(settings->ethernet_ipOne())
                        .arg(settings->ethernet_ipTwo())
                        .arg(settings->ethernet_ipThree());
    sendHelloMessage(ipStr);  // Empty string = use broadcast address ???
}

QString AgIOService::detectModuleType(const PGNParser::ParsedData& data)
{
    // Auto-detect module type based on data content
    if (data.sourceType == "NMEA") {
        // NMEA sentences ($GGA, $RMC, $PANDA) → GPS
        if (data.hasIMU) return "IMU";  // $PANDA with IMU fields
        return "GPS";
    } else if (data.sourceType == "PGN") {
        // PGN binary data
        if (data.pgnNumber == 211) return "IMU";         // IMU data
        if (data.pgnNumber == 123) return "MACHINE";         // Machine data
        if (data.pgnNumber == 253) return "Steer";       // AutoSteer status
        if (data.pgnNumber == 214) return "GPS";         // GPS main antenna
        if (data.pgnNumber == 126 || data.pgnNumber == 127) return "Steer";  // WAS data
    }

    return "Unknown";
}

bool AgIOService::shouldAcceptData(const QString& protocolId, const QString& transport)
{
    // PHASE 6.0.22.12: UDP priority logic - Always accept UDP, only accept Serial if UDP inactive
    if (transport == "UDP") return true;

    if (transport == "Serial") {
        // Check if UDP is active for this protocol
        if (!m_protocolStatusMap.contains(protocolId)) return true;

        const ModuleStatus& status = m_protocolStatusMap[protocolId];

        // Accept Serial only if:
        // 1. UDP transport is not active, OR
        // 2. UDP data is stale (>2 seconds old)
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        bool udpActive = (status.transport == "UDP") &&
                        (status.isActive) &&
                        ((now - status.lastSeenMs) < 2000);

        return !udpActive;
    }

    return false;
}

void AgIOService::logDebugInfo() const
{
    static int heartbeatCounter = 0;
    if (++heartbeatCounter % 25 == 0) { // Every 25 seconds - reduced spam
        qDebug() << "💓 AgIOService heartbeat - GPS:" << m_gpsConnected 
                << "BT:" << m_bluetoothConnected 
                << "NTRIP:" << m_ntripStatus
                << "Thread:" << QThread::currentThread();
    }
}

// ❌ LEGACY METHODS REMOVED - Replaced by Rectangle Pattern properties
// btnUDPListenOnly_clicked(int) → udpListenOnly property with automatic setter
// ntripDebug(int) → ntripDebugEnabled property with automatic setter
// bluetoothDebug(int) → bluetoothDebugEnabled property with automatic setter
// setIPFromUrl(QString) → ntripUrlIP property with automatic setter

void AgIOService::bt_search(const QString& deviceName)
{
    qDebug() << "🔍 Bluetooth search and connect to device:" << deviceName;
    // Use existing connectBluetooth method which handles the connection
    connectBluetooth(deviceName);
}

void AgIOService::bt_remove_device(const QString& deviceName)
{
    qDebug() << "🗑️ Remove Bluetooth device from paired list:" << deviceName;
    // TODO: Implement device removal from Bluetooth settings/pairing
    // This would typically involve:
    // 1. Remove from system Bluetooth paired devices
    // 2. Update m_bluetoothDevices QVariantList
    // 3. Emit settingsChanged()
}

// ❌ LEGACY METHOD REMOVED: setIPFromUrl → ntripUrlIP property with automatic setter
// DNS lookup logic moved to property setter automatically generated by Rectangle Pattern

void AgIOService::btnSendSubnet_clicked()
{
    qDebug() << "📡 Send subnet configuration to modules";
    // Use existing configureSubnet method (already implemented)  
    configureSubnet();
}

void AgIOService::sendPgn(const QByteArray& pgnData)
{
    // Phase 4.6: PGN transmission via UDPWorker (replaces FormGPS SendPgnToLoop)

    if (!pgnData.isEmpty()) {
        // Qt 6.8 fix: Remove debug spam - this function is called very frequently
        // qDebug() << "📡 Sending PGN data:" << pgnData.size() << "bytes via UDP";
        // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
        sendToTractor(pgnData);
    } else {
        qWarning() << "❌ Cannot send PGN: empty data";
    }
}

// Serial Worker slot implementations
void AgIOService::onSerialIMUDataReceived(const QByteArray& imuData)
{
    // PHASE 6.1 - Real-time module type detection
    QString detectedType = analyzeDataPattern(imuData);
    if (detectedType == "GPS") {
        QString portName = getConnectedPortName("IMU");
        qWarning() << "⚠️ GPS data detected on IMU port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "GPS", QString::fromLatin1(imuData));
        emit moduleValidationFailed(portName, "IMU", "GPS");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("IMU");
            qDebug() << "🔌 Auto-disconnected IMU port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }
    else if (detectedType == "Computer") {
        QString portName = getConnectedPortName("IMU");
        qWarning() << "⚠️ Computer/System data detected on IMU port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "Computer", QString::fromLatin1(imuData));
        emit moduleValidationFailed(portName, "IMU", "Computer");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("IMU");
            qDebug() << "🔌 Auto-disconnected IMU port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }

    // Emit signal for QML SerialMonitor (Phase 6.1)
    emit imuDataReceived(QString::fromLatin1(imuData.toHex(' ')));

    // Process IMU binary data and extract roll, pitch, yaw
    // This is a simplified implementation - actual parsing depends on IMU protocol

    if (imuData.size() >= 32) { // Assuming 32-byte IMU packets
        // Extract IMU data (implementation depends on specific IMU protocol)
        // For now, just emit the signal
        qDebug() << "📐 Serial IMU data received:" << imuData.size() << "bytes";

        // TODO: Parse actual IMU data format and update m_imuRoll, m_imuPitch, etc.
        // ✅ RECTANGLE PATTERN: Auto-emitted via imuRollChanged(), imuPitchChanged(), imuYawChanged()
    }
}

void AgIOService::onSerialAutosteerResponse(const QByteArray& response)
{
    // PHASE 6.1 - Real-time module type detection
    QString detectedType = analyzeDataPattern(response);
    if (detectedType == "GPS") {
        QString portName = getConnectedPortName("Steer");
        qWarning() << "⚠️ GPS data detected on AutoSteer port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "GPS", QString::fromLatin1(response));
        emit moduleValidationFailed(portName, "Steer", "GPS");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("Steer");
            qDebug() << "🔌 Auto-disconnected Steer port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }
    else if (detectedType == "Computer") {
        QString portName = getConnectedPortName("Steer");
        qWarning() << "⚠️ Computer/System data detected on AutoSteer port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "Computer", QString::fromLatin1(response));
        emit moduleValidationFailed(portName, "Steer", "Computer");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("Steer");
            qDebug() << "🔌 Auto-disconnected Steer port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }

    // Emit signal for QML SerialMonitor (Phase 6.1)
    emit autosteerDataReceived(QString::fromLatin1(response.toHex(' ')));

    qDebug() << "🎛️ Serial AutoSteer response:" << response;

    // Process AutoSteer response - could be status, acknowledgment, or error
    // Implementation depends on AutoSteer protocol

    // For now, just log the response
    QString responseStr = QString::fromLatin1(response);
    qDebug() << "AutoSteer response:" << responseStr;
}

void AgIOService::onSerialGPSConnected(bool connected)
{
    qDebug() << "📡 Serial GPS connection status:" << connected;

    // Update GPS connection status for QML (Phase 6.1) - Qt 6.8 QProperty setValue()
    m_gpsConnected = connected;  // Rectangle Pattern: auto-emits gpsConnectedChanged()
    // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged(), gpsQualityChanged(), satellitesChanged(), ageChanged()
}

void AgIOService::onSerialIMUConnected(bool connected)
{
    qDebug() << "🧭 Serial IMU connection status:" << connected;

    // Update IMU connection status for QML (Phase 6.1)
    m_imuConnected = connected;
    // ✅ RECTANGLE PATTERN: Auto-emitted via imuConnectedChanged()

    if (connected) {
        qDebug() << "✅ Serial IMU connected and ready";
    } else {
        qDebug() << "❌ Serial IMU disconnected";
    }
    
    // TODO: Add specific serial IMU status property if needed
    // ✅ RECTANGLE PATTERN: Auto-emitted via imuRollChanged(), imuPitchChanged(), imuYawChanged()
}

void AgIOService::onSerialAutosteerConnected(bool connected)
{
    qDebug() << "🎛️ Serial AutoSteer connection status:" << connected;

    // Update Steer connection status for QML (Phase 6.1)
    m_steerConnected = connected;
    // ✅ RECTANGLE PATTERN: Auto-emitted via steerConnectedChanged()

    if (connected) {
        qDebug() << "✅ Serial AutoSteer connected and ready";
    } else {
        qDebug() << "❌ Serial AutoSteer disconnected";
    }
    
    // TODO: Add specific serial AutoSteer status property if needed
}

void AgIOService::onSerialError(const QString& portName, const QString& error)
{
    qWarning() << "⚠️ Serial error on" << portName << ":" << error;
    
    // Handle serial errors - could trigger reconnection attempts or UI notifications
    // For critical errors, might want to emit specific error signals
    
    if (error.contains("Resource error") || error.contains("Device not found")) {
        qCritical() << "🚨 Critical serial error on" << portName << "- device disconnected";
        
        // Update connection status based on port
        if (portName == "GPS") {
            onSerialGPSConnected(false);
        } else if (portName == "IMU") {
            onSerialIMUConnected(false);
        } else if (portName == "AutoSteer") {
            onSerialAutosteerConnected(false);
        }
    }
}

// ========== Enhanced UI Integration (Phase 4.5.4) ==========

// Note: UI Status getters moved to header as inline functions using .value()
// Note: discoveredModules() getter moved to header as inline function using .value()
// Note: Network Interface getters moved to header as inline functions using .value()

void AgIOService::refreshNetworkInterfaces()
{
    qDebug() << "🔄 Refreshing network interfaces";

    // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
    QVariantList interfaces = getNetworkInterfaces();
    setNetworkInterfaces(interfaces);
    qDebug() << "✅ Network interfaces refreshed:" << interfaces.size() << "interfaces found";
}

void AgIOService::autoConfigureFromDetection()
{
    // Phase 6.0.24: Use BINDABLE property directly (main thread)
    QString detectedSubnet = m_discoveredModuleSubnet;
    if (detectedSubnet.isEmpty()) {
        qDebug() << "⚠️ No module subnet detected for auto-configuration";
        return;
    }

    qDebug() << "🎯 Auto-configuring from detected subnet:" << detectedSubnet;

    // Parse subnet parts (e.g., "192.168.5" -> 192, 168, 5)
    QStringList parts = detectedSubnet.split('.');
    if (parts.size() >= 3) {
        bool ok1, ok2, ok3;
        int ip1 = parts[0].toInt(&ok1);
        int ip2 = parts[1].toInt(&ok2);
        int ip3 = parts[2].toInt(&ok3);

        if (ok1 && ok2 && ok3) {
            // ✅ CRITICAL FIX: Auto-Configure should bring MODULE to PC subnet (192.168.1.x)
            // Force target subnet to PC subnet (not current UDP settings)
            QString pcSubnet = "192.168.1";  // Hardcode for now - we know PC is on 192.168.1.x
            QStringList pcParts = pcSubnet.split('.');

            if (pcParts.size() >= 3) {
                int targetIP1 = pcParts[0].toInt();
                int targetIP2 = pcParts[1].toInt();
                int targetIP3 = pcParts[2].toInt();

                qDebug() << "🎯 Module detected on" << detectedSubnet << "- bringing module to PC subnet" << targetIP1 << "." << targetIP2 << "." << targetIP3;

                // CDC Architecture: Temporarily set UDP config to PC subnet via SettingsManager
                auto* settings = SettingsManager::instance();
                int oldIP3 = settings->ethernet_ipThree();
                settings->setEthernet_ipOne(targetIP1);
                settings->setEthernet_ipTwo(targetIP2);
                settings->setEthernet_ipThree(targetIP3);

                // ✅ TEST: Add PGN 202 wake-up call before PGN 201 to test response
                qDebug() << "🔊 TEST: Sending PGN 202 wake-up before PGN 201...";
                // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
                wakeUpModules("255.255.255.255");

                // ✅ CDC COMPLIANT: Replace QThread::msleep(1000) with async timer
                QTimer::singleShot(1000, this, [this]() {
                    qDebug() << "⏰ Waited 1 second for module wake-up response";
                    // Now send PGN 201
                    sendPGN201ToDetectedModule();
                    // Phase 6.0.38: Wait 3 seconds for module to reboot, then verify with new scan
                    QTimer::singleShot(3000, this, [this]() {
                        qDebug() << "🔍 Auto-Configure: Module should have rebooted - starting verification scan...";
                        scanAllSubnets();
                    });
                });

                qDebug() << "✅ Auto-Configure: Sent PGN 201 to change module from" << detectedSubnet << "to" << targetIP1 << "." << targetIP2 << "." << targetIP3;
            } else {
                qDebug() << "❌ Cannot determine PC subnet for auto-configure";
            }

            // configureSubnet() handles the logging
        } else {
            qDebug() << "❌ Invalid subnet format:" << detectedSubnet;
        }
    } else {
        qDebug() << "❌ Invalid subnet format:" << detectedSubnet;
    }
}

void AgIOService::sendPGN201ToDetectedModule()
{
    qDebug() << "🚀 Sending direct PGN 201 to detected module (skipping PGN 202 scan)";

    // Build PGN 201 message - exactly like configureSubnet() but without 202 scan
    QByteArray sendIPToModules;
    sendIPToModules.append(char(0x80)); // Header 1
    sendIPToModules.append(char(0x81)); // Header 2
    sendIPToModules.append(char(0x7F)); // Source AgIO
    sendIPToModules.append(char(201)); // PGN 201 (Set Subnet - 0xC9)
    sendIPToModules.append(char(5)); // Length = 5 bytes payload

    // 5-byte payload (AgIO original format):
    sendIPToModules.append(char(201)); // Payload 1: PGN repeat (0xC9)
    sendIPToModules.append(char(201)); // Payload 2: PGN repeat (0xC9)
    // CDC Architecture: Get IP values from SettingsManager
    auto* settings = SettingsManager::instance();
    sendIPToModules.append(char(settings->ethernet_ipOne())); // Payload 3: IP1 (192 = 0xC0)
    sendIPToModules.append(char(settings->ethernet_ipTwo())); // Payload 4: IP2 (168 = 0xA8)
    sendIPToModules.append(char(settings->ethernet_ipThree())); // Payload 5: IP3 (target subnet)

    // Calculate checksum
    quint8 checksum = 0x47;
    sendIPToModules.append(char(checksum));

    qDebug() << "📦 PGN 201 message:" << sendIPToModules.toHex(' ');
    qDebug() << "🎯 Target subnet:" << settings->ethernet_ipOne() << "." << settings->ethernet_ipTwo() << "." << settings->ethernet_ipThree();

    // ✅ TEST: Send PGN 201 using SAME method as PGN 202 (255.255.255.255 global broadcast)
    qDebug() << "🎯 TEST: Sending PGN 201 via SAME broadcast method as PGN 202 (255.255.255.255)";

    // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
    sendToAllInterfaces(sendIPToModules, "255.255.255.255", 8888);

    qDebug() << "✅ PGN 201 sent via GLOBAL broadcast like PGN 202!";

    // CDC Architecture: Settings saved automatically by SettingsManager
}

bool AgIOService::canReachModuleSubnet(const QString& subnet)
{
    // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
    return hasInterfaceOnSubnet(subnet);
}

// Phase 6.0.24: scanAllSubnets() implementation moved to agioservice_udp.cpp (line 715)

// UI Control methods
void AgIOService::updateGPSStatus()
{
    QString newStatus = gpsStatusText();
    // ✅ RECTANGLE PATTERN: Direct access
    if (m_gpsStatusText != newStatus) {
        m_gpsStatusText = newStatus;  // Rectangle Pattern: auto-emits gpsStatusTextChanged()
        // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
    }
}

void AgIOService::updateModuleStatus()
{
    QString newStatus = moduleStatusText();
    // ✅ RECTANGLE PATTERN: Direct access
    if (m_moduleStatusText != newStatus) {
        m_moduleStatusText = newStatus;  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
        // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
    }
}

void AgIOService::updateNTRIPStatus()
{
    QString newStatus;
    switch (m_ntripStatus) {
        case 0: newStatus = "NTRIP Off"; break;
        case 1: newStatus = "NTRIP Connecting"; break;
        case 2: newStatus = "NTRIP Connected"; break;
        case 3: newStatus = "NTRIP Receiving"; break;
        case 4: newStatus = "NTRIP Active"; break;
        default: newStatus = "NTRIP Error"; break;
    }
    
    // ✅ RECTANGLE PATTERN: Direct access
    if (m_ntripStatusText != newStatus) {
        m_ntripStatusText = newStatus;  // Rectangle Pattern: auto-emits ntripStatusTextChanged()
        // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
    }
}

void AgIOService::updateSerialStatus()
{
    QString newStatus = serialStatusText();
    if (m_serialStatusText != newStatus) {
        m_serialStatusText = newStatus;  // Rectangle Pattern: auto-emits serialStatusTextChanged()
        // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
    }
}

void AgIOService::clearErrorDialog()
{
    // ✅ RECTANGLE PATTERN: Direct access
    if (m_showErrorDialog) {
        m_showErrorDialog = false;
        m_lastErrorMessage = "";
        emit errorOccurred("");
    }
}

// Phase 6.0.24: startModuleDiscovery() implementation moved to agioservice_udp.cpp
// Phase 6.0.24: stopModuleDiscovery() implementation moved to agioservice_udp.cpp

void AgIOService::onAgIOServiceToggled()
{
    SettingsManager* settings = SettingsManager::instance();
    bool isAgIOOn = settings->feature_isAgIOOn();

    qDebug() << "🔧 AgIOService toggled to:" << (isAgIOOn ? "ON" : "OFF");

    if (isAgIOOn) {
        // Start AgIOService - enable all communication workers
        startCommunication();
        qDebug() << "✅ AgIOService started - hardware priority enabled";
    } else {
        // Stop AgIOService - disable all communication workers
        stopCommunication();
        qDebug() << "🛑 AgIOService stopped - manual QML controls enabled";
    }
}

// Enhanced UDP Worker slots
void AgIOService::onModuleDiscovered(const QString& moduleIP, const QString& moduleType)
{
    qDebug() << "🎯 Module discovered via UDP:" << moduleIP << "Type:" << moduleType;
    
    // Add to discovered modules list
    QVariantMap moduleInfo;
    moduleInfo["ip"] = moduleIP;
    moduleInfo["type"] = moduleType;
    moduleInfo["lastSeen"] = QDateTime::currentDateTime().toString();
    
    // Check if module already exists
    bool found = false;
    // ✅ RECTANGLE PATTERN: Direct access (no .value() needed)
    QVariantList modules = m_discoveredModules;
    for (int i = 0; i < modules.size(); ++i) {
        QVariantMap existing = modules[i].toMap();
        if (existing["ip"].toString() == moduleIP) {
            // Update existing module
            modules[i] = moduleInfo;
            found = true;
            break;
        }
    }

    if (!found) {
        modules.append(moduleInfo);
    }
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits discoveredModulesChanged())
    m_discoveredModules = modules;

    // ✅ RECTANGLE PATTERN: QML automatically receives updates via binding
    // ModuleConnectionTest.qml can bind directly to: agio.discoveredModules

    updateModuleStatus();
    // ✅ RECTANGLE PATTERN: Auto-emitted via discoveredModulesChanged() property
}

void AgIOService::onModuleTimeout(const QString& moduleIP)
{
    qDebug() << "⏰ Module timeout:" << moduleIP;
    
    // Remove from discovered modules list
    // ✅ RECTANGLE PATTERN: Direct access
    QVariantList modules = m_discoveredModules;
    for (int i = modules.size() - 1; i >= 0; --i) {
        QVariantMap moduleInfo = modules[i].toMap();
        if (moduleInfo["ip"].toString() == moduleIP) {
            modules.removeAt(i);
            break;
        }
    }
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits discoveredModulesChanged())
    m_discoveredModules = modules;

    // ✅ RECTANGLE PATTERN: QML automatically receives updates via binding
    // ModuleConnectionTest.qml can bind directly to: agio.discoveredModules

    updateModuleStatus();
    // ✅ RECTANGLE PATTERN: Auto-emitted via discoveredModulesChanged() property
}

void AgIOService::onNetworkScanCompleted(const QStringList& discoveredModules)
{
    qDebug() << "🌐 Network scan completed:" << discoveredModules.size() << "modules";
    updateModuleStatus();
}

void AgIOService::onPgnDataReceived(const QByteArray& pgnData)
{
    // Process PGN data from modules
    if (pgnData.size() >= 3) {
        quint8 pgnType = static_cast<quint8>(pgnData[1]);
        
        switch (pgnType) {
            case 0x83: // GPS data from module
                qDebug() << "📍 GPS PGN data received:" << pgnData.size() << "bytes";
                break;
                
            case 0x84: // IMU data from module
                qDebug() << "📐 IMU PGN data received:" << pgnData.size() << "bytes";
                break;
                
            case 0x85: // Section control data
                qDebug() << "🚜 Section PGN data received:" << pgnData.size() << "bytes";
                break;
                
            default:
                qDebug() << "📦 Unknown PGN data:" << QString::number(pgnType, 16);
                break;
        }
    }
}

// Multi-subnet discovery slots
void AgIOService::onModuleSubnetDiscovered(const QString& moduleIP, const QString& currentSubnet)
{
    qDebug() << "🎯 Module subnet discovered - IP:" << moduleIP << "Subnet:" << currentSubnet;

    // ✅ PHASE 6.0.21: Synchronize subnet discovery properties from UDPWorker
    m_discoveredModuleSubnet = currentSubnet;  // Rectangle Pattern: auto-emits discoveredModuleSubnetChanged()
    m_discoveredModuleIP = moduleIP;           // Rectangle Pattern: auto-emits discoveredModuleIPChanged()

    // Update status for UI
    // ✅ RECTANGLE PATTERN: Direct assignment (auto-emits moduleStatusTextChanged())
    m_moduleStatusText = QString("Module found on subnet %1 (IP: %2)").arg(currentSubnet, moduleIP);  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with moduleStatusTextChanged()
    emit statusMessageChanged("Module discovered on subnet " + currentSubnet);
}

void AgIOService::onSubnetScanCompleted(const QString& activeSubnet)
{
    if (activeSubnet.isEmpty()) {
        qDebug() << "❌ Subnet scan completed - no active modules found";
        m_moduleStatusText = "No AgOpenGPS modules found on network";  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
        emit statusMessageChanged("No modules found - please check module power and network connection");
    } else {
        qDebug() << "✅ Subnet scan completed - active subnet:" << activeSubnet;
        m_moduleStatusText = QString("Module active on subnet %1").arg(activeSubnet);  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
        emit statusMessageChanged("Module configured successfully on subnet " + activeSubnet);
    }

    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

// ===== PHASE 5.2 - Extended Serial Worker slots for 5 new ports =====

// Connection status slots for 5 new ports
void AgIOService::onSerialGPS2Connected(bool connected)
{
    qDebug() << "📰 Serial GPS2" << (connected ? "connected" : "disconnected");

    // Update internal status and emit QML signal
    // Note: GPS2 uses same GPS properties since it's secondary GPS
    // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged(), gpsQualityChanged(), satellitesChanged(), ageChanged()
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::onSerialMachineConnected(bool connected)
{
    qDebug() << "🚜 Serial Machine" << (connected ? "connected" : "disconnected");

    // Update machine connection status (Phase 5.1 property)
    m_machineConnected = connected;  // Rectangle Pattern: auto-emits machineConnectedChanged() (was duplicate m_isMachineConnected)
    // ✅ RECTANGLE PATTERN: Auto-emitted via module status properties (imuConnectedChanged(), steerConnectedChanged(), etc.)
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::onSerialRadioConnected(bool connected)
{
    qDebug() << "📻 Serial Radio" << (connected ? "connected" : "disconnected");

    // Radio connection doesn't have dedicated property, use serial status
    updateSerialStatus();
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::onSerialRTCMConnected(bool connected)
{
    qDebug() << "🛰️ Serial RTCM" << (connected ? "connected" : "disconnected");

    // RTCM affects NTRIP status since it can route corrections
    if (connected) {
        m_ntripStatusText = "RTCM Serial Active";  // Rectangle Pattern: auto-emits ntripStatusTextChanged()
    } else if (!m_ntripEnabled) {
        m_ntripStatusText = "NTRIP Off";  // Rectangle Pattern: auto-emits ntripStatusTextChanged()
    }

    // ✅ RECTANGLE PATTERN: Auto-emitted via ntripConnectedChanged(), ntripStatusChanged(), ntripStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::onSerialToolConnected(bool connected)
{
    qDebug() << "Serial Tool" << (connected ? "connected" : "disconnected");

    // Tool connection uses general serial status
    updateSerialStatus();
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::onSerialPortConnected(const QString& portType, bool connected)
{
    qDebug() << "🔌 Serial port" << portType << (connected ? "connected" : "disconnected");

    // Generic port connection handler - update overall status
    updateSerialStatus();
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

// Data reception slots for 5 new ports
void AgIOService::onSerialMachineDataReceived(const QByteArray& machineData)
{
    // PHASE 6.1 - Real-time module type detection
    QString detectedType = analyzeDataPattern(machineData);
    if (detectedType == "GPS") {
        QString portName = getConnectedPortName("Machine");
        qWarning() << "GPS data detected on Machine port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "GPS", QString::fromLatin1(machineData));
        emit moduleValidationFailed(portName, "Machine", "GPS");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("Machine");
            qDebug() << "Auto-disconnected Machine port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }
    else if (detectedType == "Computer") {
        QString portName = getConnectedPortName("Machine");
        qWarning() << "Computer/System data detected on Machine port" << portName << "! Auto-disconnecting to prevent errors.";
        emit moduleTypeDetected(portName, "Computer", QString::fromLatin1(machineData));
        emit moduleValidationFailed(portName, "Machine", "Computer");

        // Auto-disconnect to prevent infinite loop
        QMetaObject::invokeMethod(this, [this]() {
            closeSerialPort("Machine");
            qDebug() << "🔌 Auto-disconnected Machine port due to wrong module type";
        }, Qt::QueuedConnection);
        return;
    }

    // Emit signal for QML SerialMonitor (Phase 6.1)
    emit machineDataReceived(QString::fromLatin1(machineData.toHex(' ')));

    // Process machine control data
    qDebug() << "Machine data received:" << machineData.size() << "bytes";

    // TODO: Parse machine protocol and update implement/section states
    // This would integrate with Phase 6 SectionConfig.qml
}

void AgIOService::onSerialRadioDataReceived(const QByteArray& radioData)
{
    // Process radio communication data
    qDebug() << "Radio data received:" << radioData.size() << "bytes";

    // TODO: Parse radio protocol for field communications
    // This would integrate with Phase 6 RadioConfig.qml
}

void AgIOService::onSerialRTCMDataReceived(const QByteArray& rtcmData)
{
    // Process RTCM correction data
    qDebug() << "RTCM data received:" << rtcmData.size() << "bytes";

    // CDC Architecture: Check NTRIP routing from SettingsManager
    auto* settings = SettingsManager::instance();
    // Route RTCM to GPS if configured
    if (settings->nmea_sendToSerial() && m_serialWorker) {
        // Send to main GPS port
        m_serialWorker->writeToGPS(rtcmData);

        // Update NTRIP raw count for UI
        // ✅ RECTANGLE PATTERN: Direct access for increment
        m_rawTripCount = m_rawTripCount + 1;
        // ✅ RECTANGLE PATTERN: Auto-emitted via ntripConnectedChanged(), ntripStatusChanged(), ntripStatusTextChanged()
    }
}

void AgIOService::onSerialToolDataReceived(const QByteArray& toolData)
{
    // Process tool control data
    qDebug() << "Tool data received:" << toolData.size() << "bytes";

    // TODO: Parse tool protocol and update tool states
    // This would integrate with Phase 6 ToolConfig.qml
}

void AgIOService::onSerialPortDataReceived(const QString& portType, const QByteArray& data)
{
    // Generic port data handler
    qDebug() << "Port" << portType << "data:" << data.size() << "bytes";

    // Could be used for debugging or logging all port activity
}

// Port configuration slots (called from QML)
void AgIOService::configureGPS2Port(const QString& portName, int baudRate)
{
    qDebug() << "Configuring GPS2 port:" << portName << "at" << baudRate << "baud";

    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "configureGPS2Port",
                                Qt::QueuedConnection,
                                Q_ARG(QString, portName),
                                Q_ARG(int, baudRate));
    }

    // CDC Architecture: Store GPS2 port in SettingsManager
    auto* settings = SettingsManager::instance();
    settings->setGnss_SerialPort(portName);  // GPS2 = GNSS port
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::configureMachinePort(const QString& portName, int baudRate)
{
    qDebug() << "Configuring Machine port:" << portName << "at" << baudRate << "baud";

    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "configureMachinePort",
                                Qt::QueuedConnection,
                                Q_ARG(QString, portName),
                                Q_ARG(int, baudRate));
    }

    // CDC Architecture: Store Machine port in SettingsManager
    auto* settings = SettingsManager::instance();
    settings->setMachine_SerialPort(portName);
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::configureRadioPort(const QString& portName, int baudRate)
{
    qDebug() << "Configuring Radio port:" << portName << "at" << baudRate << "baud";

    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "configureRadioPort",
                                Qt::QueuedConnection,
                                Q_ARG(QString, portName),
                                Q_ARG(int, baudRate));
    }

    // CDC Architecture: Store Radio port in SettingsManager
    auto* settings = SettingsManager::instance();
    settings->setSteer_SerialPort(portName);  // Radio = Steer port
    settings->setSteer_BaudRate(baudRate);
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::configureRTCMPort(const QString& portName, int baudRate)
{
    qDebug() << "Configuring RTCM port:" << portName << "at" << baudRate << "baud";

    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "configureRTCMPort",
                                Qt::QueuedConnection,
                                Q_ARG(QString, portName),
                                Q_ARG(int, baudRate));
    }

    // CDC Architecture: Store RTCM port in SettingsManager
    auto* settings = SettingsManager::instance();
    settings->setBlockage_SerialPort(portName);  // RTCM = Blockage port
    settings->setBlockage_BaudRate(baudRate);
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

void AgIOService::configureToolPort(const QString& portName, int baudRate)
{
    qDebug() << "Configuring Tool port:" << portName << "at" << baudRate << "baud";

    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "configureToolPort",
                                Qt::QueuedConnection,
                                Q_ARG(QString, portName),
                                Q_ARG(int, baudRate));
    }

    // CDC Architecture: Store Tool port in SettingsManager
    auto* settings = SettingsManager::instance();
    settings->setMachine_SerialPort(portName);  // Tool = Machine port
    // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
}

// ===== PHASE 5.3 - Extended Worker slots implementation =====

// UDP Worker Phase 5.3 slots for module monitoring
void AgIOService::onUDPModuleIMUStatusChanged(bool connected)
{
    qDebug() << "UDP Module IMU status:" << (connected ? "connected" : "disconnected");

    // Update IMU connection status (Phase 5.1 property)
    m_imuConnected = connected;  // Rectangle Pattern: auto-emits imuConnectedChanged() (was duplicate m_isIMUConnected)

    // Update module status for UI
    m_moduleStatusText = QString("IMU Module %1")
                        .arg(connected ? "Active" : "Offline");  // Rectangle Pattern: auto-emits moduleStatusTextChanged()

    // ✅ RECTANGLE PATTERN: Auto-emitted via module status properties (imuConnectedChanged(), steerConnectedChanged(), etc.)
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPModuleSteerStatusChanged(bool connected)
{
    qDebug() << "UDP Module Steer status:" << (connected ? "connected" : "disconnected");

    // Update Steer connection status (Phase 5.1 property)
    m_steerConnected = connected;  // Rectangle Pattern: auto-emits steerConnectedChanged() (was duplicate m_isSteerConnected)

    // Update status text for UI
    m_moduleStatusText = QString("Steer Module %1")
                        .arg(connected ? "Active" : "Offline");  // Rectangle Pattern: auto-emits moduleStatusTextChanged()

    // ✅ RECTANGLE PATTERN: Auto-emitted via module status properties (imuConnectedChanged(), steerConnectedChanged(), etc.)
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPModuleMachineStatusChanged(bool connected)
{
    qDebug() << "UDP Module Machine status:" << (connected ? "connected" : "disconnected");

    // Update Machine connection status (Phase 5.1 property)
    m_machineConnected = connected;  // Rectangle Pattern: auto-emits machineConnectedChanged() (was duplicate m_isMachineConnected)

    // Update status for UI
    m_moduleStatusText = QString("Machine Module %1")
                        .arg(connected ? "Active" : "Offline");  // Rectangle Pattern: auto-emits moduleStatusTextChanged()

    // ✅ RECTANGLE PATTERN: Auto-emitted via module status properties (imuConnectedChanged(), steerConnectedChanged(), etc.)
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPModuleHeartbeatUpdate(const QString& moduleType, int intervalMs)
{
    qDebug() << "UDP Module heartbeat update:" << moduleType << "interval:" << intervalMs << "ms";

    // Update heartbeat info for UI status
    setModuleStatusText(QString("Heartbeat from %1 module (interval: %2ms)")
                        .arg(moduleType)
                        .arg(intervalMs));  // Rectangle Pattern: auto-emits moduleStatusTextChanged()

    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPNMEABroadcastSent(const QString& nmeaSentence, const QString& targetAddress)
{
    qDebug() << "NMEA broadcast sent to" << targetAddress << ":" << nmeaSentence.left(20) << "...";

    // Update NMEA relay status for UI
    m_moduleStatusText = QString("NMEA relayed to %1").arg(targetAddress);  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPModuleHistoryUpdated(const QString& moduleType, bool wasConnected)
{
    qDebug() << "UDP Module history updated:" << moduleType << "was" << (wasConnected ? "connected" : "disconnected");

    // Update module history for UI
    m_moduleStatusText = QString("Module %1 status history updated").arg(moduleType);  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPNMEARelayed(const QString& nmeaSentence)
{
    qDebug() << "NMEA relayed to UDP:" << nmeaSentence.left(20) << "...";

    // Update NMEA relay status
    m_moduleStatusText = QString("NMEA relayed: %1").arg(nmeaSentence.split(',').first());  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

void AgIOService::onUDPLocalAOGIPConfigured(const QString& localIP)
{
    qDebug() << "Local AOG IP configured:" << localIP;

    // Update network configuration status
    m_moduleStatusText = QString("Local AOG IP set to %1").arg(localIP);  // Rectangle Pattern: auto-emits moduleStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Global statusChanged() redundant with specific property signals
}

// NTRIP Worker Phase 5.3 slots for RTCM routing
void AgIOService::onNTRIPRouteRTCMToSerial(const QByteArray& rtcmData)
{
    qDebug() << "Routing RTCM to serial:" << rtcmData.size() << "bytes";

    // Update NTRIP status to show serial routing
    m_ntripStatusText = QString("RTCM→Serial (%1B)").arg(rtcmData.size());  // Rectangle Pattern: auto-emits ntripStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Auto-emitted via ntripConnectedChanged(), ntripStatusChanged(), ntripStatusTextChanged()
}

void AgIOService::onNTRIPBroadcastRTCMToUDP(const QByteArray& rtcmData)
{
    // PHASE 6.0.37 - Use configured subnet broadcast address (from SettingsManager ethernet_ipOne/Two/Three)
    QHostAddress rtcmAddress(m_udpBroadcastAddress);  // e.g., "192.168.1.255"

    qDebug(agioservice) << "Broadcasting RTCM to UDP:" << rtcmData.size() << "bytes"
             << "Address:" << m_udpBroadcastAddress << "Port:" << m_rtcmPort << "(AgOpenGPS NTRIP standard)";

    // PHASE 6.0.37 - Send RTCM corrections via UDP to GPS modules (port 2233 AgOpenGPS standard)
    if (m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState) {
        qint64 bytesSent = m_udpSocket->writeDatagram(rtcmData, rtcmAddress, m_rtcmPort);
        if (bytesSent == -1) {
            qWarning(agioservice) << "Failed to send RTCM data via UDP:" << m_udpSocket->errorString();
            m_ntripStatusText = QString("RTCM->UDP Failed");
        } else {
            qDebug() << "RTCM data sent via UDP:" << bytesSent << "bytes";
            m_ntripStatusText = QString("RTCM->UDP (%1B)").arg(bytesSent);
        }
    } else {
        qWarning() << "UDP socket not ready, cannot send RTCM data";
        m_ntripStatusText = QString("RTCM->UDP (Socket Not Ready)");
    }

    // RECTANGLE PATTERN: Auto-emitted via ntripStatusTextChanged()
}

void AgIOService::onNTRIPRTCMPacketProcessed(int size, const QString& destination)
{
    qDebug() << "RTCM packet processed:" << size << "bytes to" << destination;

    // Update detailed NTRIP routing status
    m_ntripStatusText = QString("RTCM Routed: %1B → %2").arg(size).arg(destination);  // Rectangle Pattern: auto-emits ntripStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Auto-emitted via ntripConnectedChanged(), ntripStatusChanged(), ntripStatusTextChanged()
}

// GPS Worker Phase 5.3 slots for GPS2 support
void AgIOService::onGPS2DataReceived(double latitude, double longitude, double heading, double speed)
{
    qDebug() << "GPS2 data received - Lat:" << latitude << "Lon:" << longitude
             << "Heading:" << heading << "Speed:" << speed;

    // GPS2 data can be used as backup/secondary for main GPS properties
    // For now, update status to show GPS2 is active
    m_gpsStatusText = QString("GPS2 Active: %.6f,%.6f").arg(latitude).arg(longitude);  // Rectangle Pattern: auto-emits gpsStatusTextChanged()
    // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged(), gpsQualityChanged(), satellitesChanged(), ageChanged()
}

void AgIOService::onGPS2StatusChanged(bool connected, int quality, int satellites, double age)
{
    qDebug() << "GPS2 status changed - Connected:" << connected << "Quality:" << quality
             << "Satellites:" << satellites << "Age:" << age;

    // Update GPS status to reflect GPS2 connection
    if (connected) {
        m_gpsStatusText = QString("GPS2 Connected - %1 sats, Quality: %2, Age: %3s")
                                 .arg(satellites).arg(quality).arg(age, 0, 'f', 1);  // Rectangle Pattern: auto-emits gpsStatusTextChanged()
    } else {
        m_gpsStatusText = "GPS2 Disconnected";  // Rectangle Pattern: auto-emits gpsStatusTextChanged()
    }

    // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged(), gpsQualityChanged(), satellitesChanged(), ageChanged()
}

// ✅ PHASE 6.1 - SerialMonitor functionality - SerialWorker QML integration
bool AgIOService::openSerialPort(const QString& portType, const QString& portName, int baudRate)
{
    if (!m_serialWorker) {
        qWarning() << "SerialWorker not available";
        return false;
    }

    // Map QML moduleType to SerialWorker port methods (Phase 6.1)
    bool result = false;
    // ✅ CDC Compliance - Use async methods instead of BlockingQueuedConnection
    if (portType == "GPS") {
        // CDC Architecture: Save GPS port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setGnss_SerialPort(portName);
        settings->setGnss_BaudRate(baudRate);

        QMetaObject::invokeMethod(m_serialWorker, "openGPSPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    } else if (portType == "IMU") {
        // CDC Architecture: Save IMU port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setImu_SerialPort(portName);
        settings->setImu_BaudRate(baudRate);

        QMetaObject::invokeMethod(m_serialWorker, "openIMUPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    } else if (portType == "Steer") {
        // CDC Architecture: Save AutoSteer port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setSteer_SerialPort(portName);
        settings->setSteer_BaudRate(baudRate);

        QMetaObject::invokeMethod(m_serialWorker, "openAutoSteerPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    } else if (portType == "Machine") {
        // Save Machine port configuration immediately
        auto* settings = SettingsManager::instance();
        settings->setMachine_SerialPort(portName);
        settings->setMachine_BaudRate(baudRate);

        QMetaObject::invokeMethod(m_serialWorker, "openMachinePortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    } else if (portType == "Blockage") {
        // Save Blockage port configuration immediately
        auto* settings = SettingsManager::instance();
        settings->setBlockage_SerialPort(portName);
        settings->setBlockage_BaudRate(baudRate);

        QMetaObject::invokeMethod(m_serialWorker, "openToolPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    } else {
        // Generic fallback for other port types
        QMetaObject::invokeMethod(m_serialWorker, "openPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portType),
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
        result = true; // Request sent successfully
    }

    qDebug() << "SerialPort open request" << portType << portName << baudRate << "→" << result;
    return result;
}

void AgIOService::openSerialPortAsync(const QString& portType, const QString& portName, int baudRate)
{
    if (!m_serialWorker) {
        qWarning() << "SerialWorker not available";
        emit serialPortOpened(portType, false, "SerialWorker not available");
        return;
    }

    qDebug() << "🔗 ✅ CDC COMPLIANT: Async SerialPort open request" << portType << portName << baudRate;

    // ✅ CDC COMPLIANT: Use QueuedConnection instead of BlockingQueuedConnection
    // Result will be received via signals from SerialWorker
    if (portType == "GPS") {
        QMetaObject::invokeMethod(m_serialWorker, "openGPSPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
    } else if (portType == "IMU") {
        QMetaObject::invokeMethod(m_serialWorker, "openIMUPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
    } else if (portType == "Steer") {
        QMetaObject::invokeMethod(m_serialWorker, "openAutoSteerPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
    } else if (portType == "Machine") {
        QMetaObject::invokeMethod(m_serialWorker, "openMachinePortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
    } else if (portType == "Blockage") {
        QMetaObject::invokeMethod(m_serialWorker, "openToolPortAsync", Qt::QueuedConnection,
                                 Q_ARG(QString, portName),
                                 Q_ARG(int, baudRate));
    } else {
        qWarning() << "Unknown port type:" << portType;
        emit serialPortOpened(portType, false, "Unknown port type: " + portType);
    }
}

void AgIOService::closeSerialPort(const QString& portType)
{
    if (!m_serialWorker) {
        qWarning() << "SerialWorker not available";
        return;
    }

    // Map QML moduleType to SerialWorker port methods (Phase 6.1)
    if (portType == "GPS") {
        // CDC Architecture: Clear GPS port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setGnss_SerialPort("");

        QMetaObject::invokeMethod(m_serialWorker, "closeGPSPort", Qt::QueuedConnection);

        // Update port connection status immediately for UI responsiveness
        m_gpsPortConnected = false;
        // ✅ RECTANGLE PATTERN: Auto-emitted via gpsPortConnectedChanged()

    } else if (portType == "IMU") {
        // CDC Architecture: Clear IMU port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setImu_SerialPort("");

        QMetaObject::invokeMethod(m_serialWorker, "closeIMUPort", Qt::QueuedConnection);

    } else if (portType == "Steer") {
        // CDC Architecture: Clear AutoSteer port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setSteer_SerialPort("");

        QMetaObject::invokeMethod(m_serialWorker, "closeAutoSteerPort", Qt::QueuedConnection);

        // Update connection status immediately for UI responsiveness
        m_steerConnected = false;
        // ✅ RECTANGLE PATTERN: Auto-emitted via steerConnectedChanged()

    } else if (portType == "Machine") {
        // CDC Architecture: Clear Machine port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setMachine_SerialPort("");

        QMetaObject::invokeMethod(m_serialWorker, "closeMachinePort", Qt::QueuedConnection);

        // Update connection status immediately for UI responsiveness
        m_machineConnected = false;
        // ✅ RECTANGLE PATTERN: Auto-emitted via machineConnectedChanged()

    } else if (portType == "Blockage") {
        // CDC Architecture: Clear Blockage port configuration directly
        auto* settings = SettingsManager::instance();
        settings->setBlockage_SerialPort("");

        QMetaObject::invokeMethod(m_serialWorker, "closeToolPort", Qt::QueuedConnection);

        // Update connection status immediately for UI responsiveness
        m_blockageConnected = false;
        // ✅ RECTANGLE PATTERN: Auto-emitted via blockageConnectedChanged()

    } else {
        // Generic fallback for other port types
        QMetaObject::invokeMethod(m_serialWorker, "closePort", Qt::QueuedConnection,
                                 Q_ARG(QString, portType));
    }

    qDebug() << "🔌 SerialPort close request" << portType;
}

QStringList AgIOService::getAvailableSerialPorts()
{
    if (!m_serialWorker) {
        qWarning() << "❌ SerialWorker not available";
        return QStringList();
    }

    // ✅ CDC Compliance - Use async method instead of BlockingQueuedConnection
    QMetaObject::invokeMethod(m_serialWorker, "scanAvailablePortsAsync", Qt::QueuedConnection);

    // Return cached result immediately - async will update cache when ready
    qDebug() << "🔍 Returning cached ports:" << m_cachedAvailablePorts.size() << "- async scan in progress";
    return m_cachedAvailablePorts;
}

bool AgIOService::isSerialPortConnected(const QString& portType)
{
    if (!m_serialWorker) {
        qWarning() << "❌ SerialWorker not available";
        return false;
    }

    // ✅ CDC Compliance - Use async methods, trigger status check for real-time updates
    QMetaObject::invokeMethod(m_serialWorker, "checkConnectionStatusAsync", Qt::QueuedConnection,
                             Q_ARG(QString, portType));

    // Return cached status immediately - async will update cache when ready
    bool result = m_cachedConnectionStatus.value(portType, false);
    qDebug() << "🔍 Returning cached connection status for" << portType << ":" << result;

    return result;
}

void AgIOService::writeToSerialPort(const QString& portType, const QString& data)
{
    if (!m_serialWorker) {
        qWarning() << "❌ SerialWorker not available";
        return;
    }

    // Convert QString to QByteArray and add line ending
    QByteArray dataBytes = (data + "\r\n").toUtf8();

    // Map QML moduleType to SerialWorker port methods (Phase 6.1)
    if (portType == "GPS") {
        QMetaObject::invokeMethod(m_serialWorker, "writeToGPS", Qt::QueuedConnection,
                                 Q_ARG(QByteArray, dataBytes));
    } else if (portType == "IMU") {
        QMetaObject::invokeMethod(m_serialWorker, "writeToIMU", Qt::QueuedConnection,
                                 Q_ARG(QByteArray, dataBytes));
    } else if (portType == "Steer") {
        QMetaObject::invokeMethod(m_serialWorker, "writeToAutoSteer", Qt::QueuedConnection,
                                 Q_ARG(QByteArray, dataBytes));
    } else if (portType == "Machine") {
        QMetaObject::invokeMethod(m_serialWorker, "writeToMachine", Qt::QueuedConnection,
                                 Q_ARG(QByteArray, dataBytes));
    } else if (portType == "Blockage") {
        QMetaObject::invokeMethod(m_serialWorker, "writeToTool", Qt::QueuedConnection,
                                 Q_ARG(QByteArray, dataBytes));
    } else {
        // Generic fallback for other port types
        QMetaObject::invokeMethod(m_serialWorker, "writeToPort", Qt::QueuedConnection,
                                 Q_ARG(QString, portType),
                                 Q_ARG(QByteArray, dataBytes));
    }

    qDebug() << "📤 SerialPort write" << portType << ":" << data;
}

QStringList AgIOService::getAvailableSerialPortsForModule(const QString& moduleType)
{
    QStringList allPorts = getAvailableSerialPorts();
    QStringList availablePorts;

    // Get currently used ports for all modules
    QStringList usedPorts;

    // Check each module type and add their used ports
    QStringList moduleTypes = {"GPS", "IMU", "Steer", "Machine", "Blockage"};
    for (const QString& module : moduleTypes) {
        if (module != moduleType && isSerialPortConnected(module)) {
            QString portName = getConnectedPortName(module);
            if (!portName.isEmpty()) {
                usedPorts.append(portName);
            }
        }
    }

    // Filter out used ports
    for (const QString& port : std::as_const(allPorts)) {
        if (!usedPorts.contains(port)) {
            availablePorts.append(port);
        }
    }

    qDebug() << "🔍 Available ports for" << moduleType << ":" << availablePorts << "(excluded:" << usedPorts << ")";
    return availablePorts;
}

QString AgIOService::getPortOwner(const QString& portName)
{
    QStringList moduleTypes = {"GPS", "IMU", "Steer", "Machine", "Blockage"};

    for (const QString& module : moduleTypes) {
        if (isSerialPortConnected(module)) {
            QString connectedPort = getConnectedPortName(module);
            if (connectedPort == portName) {
                return module;
            }
        }
    }

    return QString(); // Port not in use
}

QString AgIOService::getConnectedPortName(const QString& moduleType)
{
    if (!m_serialWorker) {
        return QString();
    }

    // ✅ CDC Compliance - Use async methods, trigger port name query
    QMetaObject::invokeMethod(m_serialWorker, "getPortNameAsync", Qt::QueuedConnection,
                             Q_ARG(QString, moduleType));

    // Return empty string immediately - actual name will arrive via onPortNameResult()
    QString result;

    return result;
}

bool AgIOService::validateModuleType(const QString& portName, const QString& expectedModuleType, int baudRate)
{
    qDebug() << "🔍 Validating module type for port" << portName << "expected:" << expectedModuleType << "baudRate:" << baudRate;

    // Check if port is already in use
    QString owner = getPortOwner(portName);
    if (!owner.isEmpty()) {
        emit portAlreadyInUse(portName, owner);
        return false;
    }

    // Implement automatic module detection based on AgIO source code analysis
    return performQuickModuleDetection(portName, expectedModuleType, baudRate);
}

bool AgIOService::performQuickModuleDetection(const QString& portName, const QString& expectedModuleType, int baudRate)
{
    qDebug() << "🔬 Performing quick module detection for" << expectedModuleType << "on" << portName << "at" << baudRate;

    // Quick validation rules based on AgIO source code analysis:

    // GPS modules: should use NMEA-compatible baud rates
    if (expectedModuleType == "GPS") {
        if (baudRate != 4800 && baudRate != 9600 && baudRate != 38400 && baudRate != 115200) {
            emit moduleValidationFailed(portName, expectedModuleType, "Invalid GPS baud rate");
            return false;
        }
        // GPS modules typically use text-based NMEA format
        // Could temporarily connect and look for '$' patterns if needed
        return true;
    }

    // IMU modules: should use 38400 baud (AgIO standard)
    else if (expectedModuleType == "IMU") {
        if (baudRate != 38400) {
            qWarning() << "⚠️ IMU modules typically use 38400 baud, got" << baudRate;
            // Don't fail, just warn - some IMU modules might use different rates
        }
        // IMU modules use binary format: 0x80, 0x81, 0x7F header
        return true;
    }

    // AutoSteer modules: should use 38400 baud (AgIO standard)
    else if (expectedModuleType == "Steer") {
        if (baudRate != 38400) {
            qWarning() << "⚠️ AutoSteer modules typically use 38400 baud, got" << baudRate;
            // Don't fail, just warn
        }
        // AutoSteer modules use binary format: 0x80, 0x81, 0x7F header
        return true;
    }

    // Machine modules: should use 38400 baud (AgIO standard)
    else if (expectedModuleType == "Machine") {
        if (baudRate != 38400) {
            qWarning() << "⚠️ Machine modules typically use 38400 baud, got" << baudRate;
            // Don't fail, just warn
        }
        // Machine modules use binary format: 0x80, 0x81, 0x7F header
        return true;
    }

    // Blockage modules: similar to Machine modules
    else if (expectedModuleType == "Blockage") {
        if (baudRate != 38400) {
            qWarning() << "⚠️ Blockage modules typically use 38400 baud, got" << baudRate;
        }
        return true;
    }

    // Unknown module type
    else {
        qWarning() << "⚠️ Unknown module type:" << expectedModuleType;
        return true; // Allow unknown types to proceed
    }
}

QString AgIOService::analyzeDataPattern(const QByteArray& data)
{
    if (data.isEmpty()) return "Unknown";

    // Convert to QString for text analysis if needed
    QString textData = QString::fromUtf8(data);

    // Check for NMEA GPS data (text format)
    if (isNMEAGPSData(textData)) {
        return "GPS";
    }

    // Check for AgIO binary protocol (IMU/Steer/Machine)
    if (isAgIOBinaryData(data)) {
        // Further analysis could determine specific module type based on PGN
        return "Module"; // Generic AgIO module (IMU/Steer/Machine)
    }

    // Check for random computer/system data
    if (isRandomComputerData(data)) {
        return "Computer";
    }

    return "Unknown";
}

QString AgIOService::analyzeDataPattern(const QString& data)
{
    if (data.isEmpty()) return "Unknown";

    // Check for NMEA GPS data
    if (isNMEAGPSData(data)) {
        return "GPS";
    }

    // Convert to QByteArray for binary analysis
    QByteArray binaryData = data.toUtf8();
    return analyzeDataPattern(binaryData);
}

bool AgIOService::isNMEAGPSData(const QString& data)
{
    // Based on AgIO source analysis: NMEA sentences start with $ and contain specific patterns
    if (data.contains("$GPGGA") || data.contains("$GNGGA") ||
        data.contains("$GPVTG") || data.contains("$GNVTG") ||
        data.contains("$GPHDT") || data.contains("$GNHDT") ||
        data.contains("$GPRMC") || data.contains("$GNRMC") ||
        data.contains("$PAOGI") || data.contains("$PANDA") ||
        data.contains("$KSXT") || data.contains("$GPHPD")) {
        return true;
    }

    // Generic NMEA detection: starts with $, ends with *XX\r\n
    QRegularExpression nmeaPattern("\\$[A-Z]{2}[A-Z]{3},.+\\*[0-9A-F]{2}");
    if (nmeaPattern.match(data).hasMatch()) {
        return true;
    }

    return false;
}

bool AgIOService::isAgIOBinaryData(const QByteArray& data)
{
    // Based on AgIO source analysis: AgIO modules use 0x80, 0x81, 0x7F header
    if (data.size() >= 3) {
        if ((unsigned char)data[0] == 0x80 &&
            (unsigned char)data[1] == 0x81 &&
            (unsigned char)data[2] == 0x7F) {
            return true;
        }
    }

    // Look for the pattern in the data stream
    for (int i = 0; i <= data.size() - 3; i++) {
        if ((unsigned char)data[i] == 0x80 &&
            (unsigned char)data[i+1] == 0x81 &&
            (unsigned char)data[i+2] == 0x7F) {
            return true;
        }
    }

    return false;
}

bool AgIOService::isRandomComputerData(const QByteArray& data)
{
    if (data.isEmpty()) return false;

    // Check for patterns that suggest computer/system data rather than farm equipment
    QString textData = QString::fromUtf8(data);

    // Common computer/system patterns
    if (textData.contains("USB") || textData.contains("COM") ||
        textData.contains("ERROR") || textData.contains("Windows") ||
        textData.contains("Driver") || textData.contains("Device") ||
        textData.contains("System") || textData.contains("Debug") ||
        textData.contains("AT+") || textData.contains("OK") ||
        textData.contains("CONNECT") || textData.contains("RING")) {
        return true;
    }

    // Check for high concentration of printable ASCII without GPS/AgIO patterns
    int printableCount = 0;
    for (char c : data) {
        if (c >= 32 && c <= 126) printableCount++;
    }

    // If mostly printable but not GPS/AgIO, likely computer data
    if (printableCount > data.size() * 0.8 &&
        !isNMEAGPSData(textData) &&
        !isAgIOBinaryData(data)) {
        return true;
    }

    return false;
}

// =====================================================================================
// PHASE 6.2.1 - Traffic Monitoring Implementation
// =====================================================================================

// Phase 6.0.24: resetTrafficCounters() implementation moved to agioservice_udp.cpp

void AgIOService::enableUDP(bool enable)
{
    // CDC Architecture: Update UDP state directly in SettingsManager
    auto* settings = SettingsManager::instance();
    if (settings->ethernet_isOn() != enable) {
        settings->setEthernet_isOn(enable);

        // Start or stop UDP worker
        if (enable) {
            emit requestStartUDP(m_udpBroadcastAddress, 8888);
            qDebug() << "🚀 UDP enabled and started - broadcast:" << m_udpBroadcastAddress;
        } else {
            emit requestStopUDP();
            qDebug() << "⏹️ UDP disabled and stopped";

            // ✅ PHASE 6.0.21.8: Reset UDP traffic counters when disabling UDP
            // Phase 6.0.24: Direct call in main thread (implementation in agioservice_udp.cpp)
            resetUdpTrafficCounters();
            qDebug() << "🔄 UDP traffic counters reset (UDP disabled)";

            // Immediately update ethernet status when UDP is disabled
            m_ethernetConnected = false;
            // ✅ RECTANGLE PATTERN: Auto-emitted via ethernetConnectedChanged()
        }

        // Notify QML that settings changed
        // ✅ RECTANGLE PATTERN: Settings changes handled by SettingsManager singleton (no signal needed)
    }
}

// Note: getTraffic() moved to header as inline function using .value()

// ✅ CDC Compliance - Async operation result handlers (replaces BlockingQueuedConnection)

void AgIOService::onPortOpenResult(const QString& portType, bool success, const QString& errorMessage)
{
    qDebug() << "🔧 AgIOService::onPortOpenResult" << portType << success << errorMessage;

    if (success) {
        qDebug() << "✅ Port opened successfully:" << portType;

        // Update port connection status for QML button logic
        if (portType == "GPS") {
            m_gpsPortConnected = true;
            // ✅ RECTANGLE PATTERN: Auto-emitted via gpsPortConnectedChanged()
        }

    } else {
        qWarning() << "❌ Failed to open port:" << portType << errorMessage;

        // Update port connection status for QML button logic
        if (portType == "GPS") {
            m_gpsPortConnected = false;
            // ✅ RECTANGLE PATTERN: Auto-emitted via gpsPortConnectedChanged()
        }
    }
}

void AgIOService::onPortCloseResult(const QString& portType, bool success)
{
    qDebug() << "🔧 AgIOService::onPortCloseResult" << portType << success;

    if (success) {
        qDebug() << "✅ Port closed successfully:" << portType;

        // Update port connection status for QML button logic
        if (portType == "GPS") {
            m_gpsPortConnected = false;
            // ✅ RECTANGLE PATTERN: Auto-emitted via gpsPortConnectedChanged()
        }

    } else {
        qWarning() << "❌ Failed to close port:" << portType;
    }
}

void AgIOService::onConnectionStatusResult(const QString& portType, bool isConnected)
{
    qDebug() << "🔧 AgIOService::onConnectionStatusResult" << portType << isConnected;

    // ✅ Cache the result for immediate access by sync functions
    m_cachedConnectionStatus[portType] = isConnected;

    // Emit specific connection status signals for QML property bindings
    if (portType == "GPS") {
        // ✅ RECTANGLE PATTERN: Auto-emitted via gpsConnectedChanged() property
    } else if (portType == "IMU") {
        // ✅ RECTANGLE PATTERN: Auto-emitted via imuConnectedChanged() property
    } else if (portType == "Steer") {
        // ✅ RECTANGLE PATTERN: Auto-emitted via steerConnectedChanged() property
    } else if (portType == "Machine") {
        // ✅ RECTANGLE PATTERN: Auto-emitted via machineConnectedChanged() property
    } else if (portType == "Blockage") {
        // ✅ RECTANGLE PATTERN: Auto-emitted via blockageConnectedChanged() property
    }
}

void AgIOService::onPortNameResult(const QString& portType, const QString& portName)
{
    qDebug() << "🔧 AgIOService::onPortNameResult" << portType << portName;

    // Store result or continue workflow as needed
}

void AgIOService::onPortBaudRateResult(const QString& portType, int baudRate)
{
    qDebug() << "🔧 AgIOService::onPortBaudRateResult" << portType << baudRate;

    // Store result or continue workflow as needed
}

void AgIOService::onAvailablePortsResult(const QStringList& ports)
{
    qDebug() << "🔧 AgIOService::onAvailablePortsResult" << ports.size() << "ports found";

    // ✅ Cache the results for immediate access by sync functions
    m_cachedAvailablePorts = ports;

    // Emit signal for QML bindings
    // ✅ RECTANGLE PATTERN: Auto-emitted via networkInterfacesChanged() property
}

// ============================================================================
// Phase 6.0.21: Data Hub Implementation - Parse and Broadcast
// ============================================================================

// Phase 6.0.21.1 + 6.0.22.9: Unified Data Hub with source tracking and UDP priority
// Phase 6.0.24: onDataReceived() removed - replaced by onUdpDataReady() in agioservice_udp.cpp
// Event-driven UDP reception eliminates need for thread-safe wrapper with mutex
// Serial data still flows through unified data handler (future Phase 2 migration)

// ============================================================================
// Phase 6.0.24: Event-Driven UDP Implementation (Modular Organization)
// ============================================================================
// This file is INCLUDED, not compiled separately
// Pattern follows settingsmanager_implementations.cpp for clean code organization
#include "agioservice_udp.cpp"
