// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// SerialWorker implementation - Multi-port serial communication

#include "serialworker.h"
#include <QCoreApplication>
#include <QThread>

// ✅ PHASE 5.2 - Port type constants for 5 new ports
const QString SerialWorker::PORT_GPS2 = "GPS2";
const QString SerialWorker::PORT_MACHINE = "Machine";
const QString SerialWorker::PORT_RADIO = "Radio";
const QString SerialWorker::PORT_RTCM = "RTCM";
const QString SerialWorker::PORT_TOOL = "Tool";

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
    , m_gpsPort(nullptr)
    , m_imuPort(nullptr)
    , m_autosteerPort(nullptr)
    , m_gpsConnected(false)
    , m_imuConnected(false)
    , m_autosteerConnected(false)
    , m_gpsBaudRate(9600)
    , m_imuBaudRate(9600)
    , m_autosteerBaudRate(9600)
    , m_connectionTimer(nullptr)
    , m_isRunning(false)
{
    qDebug() << "🔧 SerialWorker constructor - Thread:" << QThread::currentThread();

    // ✅ PHASE 5.2 - Initialize dynamic ports maps for 5 new ports
    initializeDynamicPorts();
    
    // Initialize connection monitoring timer
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(false);
    m_connectionTimer->setInterval(5000); // Check connections every 5 seconds
    connect(m_connectionTimer, &QTimer::timeout, this, &SerialWorker::checkConnections);
}

SerialWorker::~SerialWorker()
{
    qDebug() << "🔧 SerialWorker destructor";
    stopWorker();
    closeAllPorts();
}

void SerialWorker::startWorker()
{
    qDebug() << "🚀 Starting SerialWorker - Thread:" << QThread::currentThread();
    
    QMutexLocker locker(&m_mutex);
    
    if (m_isRunning) {
        qWarning() << "SerialWorker already running";
        return;
    }
    
    m_isRunning = true;
    m_connectionTimer->start();
    
    emit workerStarted();
    qDebug() << "✅ SerialWorker started successfully";
}

void SerialWorker::stopWorker()
{
    qDebug() << "🛑 Stopping SerialWorker";
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    m_connectionTimer->stop();
    closeAllPorts();
    
    emit workerStopped();
    qDebug() << "✅ SerialWorker stopped successfully";
}

// Port Management Methods
bool SerialWorker::openGPSPort(const QString& portName, int baudRate)
{
    qDebug() << "📡 Opening GPS port:" << portName << "at" << baudRate << "baud";
    
    if (openPort(m_gpsPort, portName, baudRate, "GPS")) {
        m_gpsPortName = portName;
        m_gpsBaudRate = baudRate;
        m_gpsConnected = true;
        
        // Connect data reception
        connect(m_gpsPort, &QSerialPort::readyRead, this, &SerialWorker::onGPSDataReady);
        connect(m_gpsPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
                this, &SerialWorker::onGPSError);
        
        emit gpsConnected(true);
        qDebug() << "✅ GPS port opened successfully:" << portName;
        return true;
    }
    
    return false;
}

bool SerialWorker::openIMUPort(const QString& portName, int baudRate)
{
    qDebug() << "🧭 Opening IMU port:" << portName << "at" << baudRate << "baud";
    
    if (openPort(m_imuPort, portName, baudRate, "IMU")) {
        m_imuPortName = portName;
        m_imuBaudRate = baudRate;
        m_imuConnected = true;
        
        // Connect data reception
        connect(m_imuPort, &QSerialPort::readyRead, this, &SerialWorker::onIMUDataReady);
        connect(m_imuPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
                this, &SerialWorker::onIMUError);
        
        emit imuConnected(true);
        qDebug() << "✅ IMU port opened successfully:" << portName;
        return true;
    }
    
    return false;
}

bool SerialWorker::openAutoSteerPort(const QString& portName, int baudRate)
{
    qDebug() << "🎛️ Opening AutoSteer port:" << portName << "at" << baudRate << "baud";
    
    if (openPort(m_autosteerPort, portName, baudRate, "AutoSteer")) {
        m_autosteerPortName = portName;
        m_autosteerBaudRate = baudRate;
        m_autosteerConnected = true;
        
        // Connect data reception
        connect(m_autosteerPort, &QSerialPort::readyRead, this, &SerialWorker::onAutoSteerDataReady);
        connect(m_autosteerPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
                this, &SerialWorker::onAutoSteerError);
        
        emit autosteerConnected(true);
        qDebug() << "✅ AutoSteer port opened successfully:" << portName;
        return true;
    }
    
    return false;
}

void SerialWorker::closeGPSPort()
{
    closePort(m_gpsPort, "GPS");
    m_gpsConnected = false;
    m_gpsPortName.clear();
    emit gpsConnected(false);
}

void SerialWorker::closeIMUPort()
{
    closePort(m_imuPort, "IMU");
    m_imuConnected = false;
    m_imuPortName.clear();
    emit imuConnected(false);
}

void SerialWorker::closeAutoSteerPort()
{
    closePort(m_autosteerPort, "AutoSteer");
    m_autosteerConnected = false;
    m_autosteerPortName.clear();
    emit autosteerConnected(false);
}

void SerialWorker::closeAllPorts()
{
    qDebug() << "🔌 Closing all serial ports";

    // Close legacy 3 ports
    closeGPSPort();
    closeIMUPort();
    closeAutoSteerPort();

    // ✅ PHASE 5.2 - Close all dynamic ports
    for (auto it = m_serialPorts.begin(); it != m_serialPorts.end(); ++it) {
        closeDynamicPort(it.key());
    }
}

QStringList SerialWorker::scanAvailablePorts()
{
    QStringList availablePorts;
    
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : serialPorts) {
        availablePorts << portInfo.portName();
    }
    
    qDebug() << "🔍 Available serial ports:" << availablePorts;
    return availablePorts;
}

bool SerialWorker::isPortAvailable(const QString& portName)
{
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : serialPorts) {
        if (portInfo.portName() == portName) {
            return true;
        }
    }
    return false;
}

// Data Transmission Methods
void SerialWorker::writeToGPS(const QByteArray& data)
{
    if (m_gpsPort && m_gpsPort->isOpen()) {
        qint64 bytesWritten = m_gpsPort->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write to GPS port:" << m_gpsPort->errorString();
            emit serialError("GPS", m_gpsPort->errorString());
        } else {
            qDebug() << "📤 GPS data sent:" << bytesWritten << "bytes";
        }
    } else {
        qWarning() << "GPS port not open for writing";
        emit serialError("GPS", "Port not open");
    }
}

void SerialWorker::writeToIMU(const QByteArray& data)
{
    if (m_imuPort && m_imuPort->isOpen()) {
        qint64 bytesWritten = m_imuPort->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write to IMU port:" << m_imuPort->errorString();
            emit serialError("IMU", m_imuPort->errorString());
        } else {
            qDebug() << "📤 IMU data sent:" << bytesWritten << "bytes";
        }
    } else {
        qWarning() << "IMU port not open for writing";
        emit serialError("IMU", "Port not open");
    }
}

void SerialWorker::writeToAutoSteer(const QByteArray& data)
{
    if (m_autosteerPort && m_autosteerPort->isOpen()) {
        qint64 bytesWritten = m_autosteerPort->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write to AutoSteer port:" << m_autosteerPort->errorString();
            emit serialError("AutoSteer", m_autosteerPort->errorString());
        } else {
            qDebug() << "📤 AutoSteer data sent:" << bytesWritten << "bytes";
        }
    } else {
        qWarning() << "AutoSteer port not open for writing";
        emit serialError("AutoSteer", "Port not open");
    }
}

// Configuration Methods
void SerialWorker::configureGPSPort(const QString& portName, int baudRate)
{
    if (m_gpsConnected) {
        closeGPSPort();
    }
    openGPSPort(portName, baudRate);
}

void SerialWorker::configureIMUPort(const QString& portName, int baudRate)
{
    if (m_imuConnected) {
        closeIMUPort();
    }
    openIMUPort(portName, baudRate);
}

void SerialWorker::configureAutoSteerPort(const QString& portName, int baudRate)
{
    if (m_autosteerConnected) {
        closeAutoSteerPort();
    }
    openAutoSteerPort(portName, baudRate);
}

// Data Reception Handlers
void SerialWorker::onGPSDataReady()
{
    if (!m_gpsPort) return;
    
    QByteArray data = m_gpsPort->readAll();
    m_gpsBuffer.append(data);
    
    emit serialDataReceived("GPS", data);
    processGPSBuffer();
}

void SerialWorker::onIMUDataReady()
{
    if (!m_imuPort) return;
    
    QByteArray data = m_imuPort->readAll();
    m_imuBuffer.append(data);
    
    emit serialDataReceived("IMU", data);
    processIMUBuffer();
}

void SerialWorker::onAutoSteerDataReady()
{
    if (!m_autosteerPort) return;
    
    QByteArray data = m_autosteerPort->readAll();
    m_autosteerBuffer.append(data);
    
    emit serialDataReceived("AutoSteer", data);
    processAutoSteerBuffer();
}

// Error Handlers
void SerialWorker::onGPSError(QSerialPort::SerialPortError error)
{
    handleSerialError(m_gpsPort, "GPS", error);
}

void SerialWorker::onIMUError(QSerialPort::SerialPortError error)
{
    handleSerialError(m_imuPort, "IMU", error);
}

void SerialWorker::onAutoSteerError(QSerialPort::SerialPortError error)
{
    handleSerialError(m_autosteerPort, "AutoSteer", error);
}

void SerialWorker::checkConnections()
{
    // Check legacy ports
    if (m_gpsPort && !m_gpsPort->isOpen()) {
        m_gpsConnected = false;
        emit gpsConnected(false);
    }

    if (m_imuPort && !m_imuPort->isOpen()) {
        m_imuConnected = false;
        emit imuConnected(false);
    }

    if (m_autosteerPort && !m_autosteerPort->isOpen()) {
        m_autosteerConnected = false;
        emit autosteerConnected(false);
    }

    // ✅ PHASE 5.2 - Check dynamic ports
    for (auto it = m_serialPorts.begin(); it != m_serialPorts.end(); ++it) {
        const QString& portType = it.key();
        QSerialPort* port = it.value();

        if (port && !port->isOpen() && m_portConnected.value(portType, false)) {
            m_portConnected[portType] = false;
            emit portConnected(portType, false);
            emitSpecificConnectionSignal(portType, false);
        }
    }
}

// Helper Methods
bool SerialWorker::openPort(QSerialPort*& port, const QString& portName, int baudRate, const QString& description)
{
    // Close existing port if open
    if (port && port->isOpen()) {
        port->close();
        delete port;
        port = nullptr;
    }
    
    // Create new port
    port = new QSerialPort(this);
    port->setPortName(portName);
    port->setBaudRate(baudRate);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    
    if (port->open(QIODevice::ReadWrite)) {
        qDebug() << "✅" << description << "port opened:" << portName << "at" << baudRate << "baud";
        return true;
    } else {
        QString errorStr = port->errorString();
        qWarning() << "❌ Failed to open" << description << "port:" << portName << "Error:" << errorStr;
        emit portOpenError(portName, errorStr);
        
        delete port;
        port = nullptr;
        return false;
    }
}

void SerialWorker::closePort(QSerialPort*& port, const QString& description)
{
    if (port) {
        if (port->isOpen()) {
            port->close();
            qDebug() << "🔌" << description << "port closed";
        }
        delete port;
        port = nullptr;
    }
}

void SerialWorker::processGPSBuffer()
{
    // Extract complete NMEA sentences from buffer
    QStringList sentences = extractNMEASentences(m_gpsBuffer);

    for (const QString& sentence : std::as_const(sentences)) {
        if (isValidNMEASentence(sentence)) {
            emit gpsDataReceived(sentence);

            // PHASE 6.0.22.7: Unified data signal with Serial source prefix
            if (m_gpsPort) {
                QString source = "Serial:" + m_gpsPort->portName();
                emit dataReceived(sentence.toUtf8(), source);
            }
        } else {
            qDebug() << "⚠️ Invalid NMEA sentence:" << sentence;
        }
    }
}

void SerialWorker::processIMUBuffer()
{
    // For IMU, we typically process binary data or specific protocols
    // This is a simplified version - actual implementation depends on IMU protocol

    if (m_imuBuffer.size() >= 32) { // Assuming 32-byte IMU packets
        QByteArray packet = m_imuBuffer.left(32);
        m_imuBuffer.remove(0, 32);

        emit imuDataReceived(packet);

        // PHASE 6.0.22.7: Unified data signal with Serial source prefix
        if (m_imuPort) {
            QString source = "Serial:" + m_imuPort->portName();
            emit dataReceived(packet, source);
        }
    }
}

void SerialWorker::processAutoSteerBuffer()
{
    // Process AutoSteer responses - typically status or acknowledgment messages

    while (m_autosteerBuffer.contains('\n')) {
        int index = m_autosteerBuffer.indexOf('\n');
        QByteArray line = m_autosteerBuffer.left(index);
        m_autosteerBuffer.remove(0, index + 1);

        if (!line.isEmpty()) {
            emit autosteerResponseReceived(line);

            // PHASE 6.0.22.7: Unified data signal with Serial source prefix
            if (m_autosteerPort) {
                QString source = "Serial:" + m_autosteerPort->portName();
                emit dataReceived(line, source);
            }
        }
    }
}

void SerialWorker::handleSerialError(QSerialPort* port, const QString& portName, QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    QString errorString;
    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorString = "Device not found";
            break;
        case QSerialPort::PermissionError:
            errorString = "Permission denied";
            break;
        case QSerialPort::OpenError:
            errorString = "Unable to open port";
            break;
        case QSerialPort::WriteError:
            errorString = "Write error";
            break;
        case QSerialPort::ReadError:
            errorString = "Read error";
            break;
        case QSerialPort::ResourceError:
            errorString = "Resource error (device disconnected)";
            break;
        case QSerialPort::UnsupportedOperationError:
            errorString = "Unsupported operation";
            break;
        case QSerialPort::TimeoutError:
            errorString = "Timeout error";
            break;
        default:
            errorString = "Unknown error";
            break;
    }
    
    qWarning() << "⚠️ Serial error on" << portName << "port:" << errorString;
    emit serialError(portName, errorString);
    
    // Handle critical errors by closing the port
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        if (port && port->isOpen()) {
            port->close();
        }
        
        // Update connection status
        if (portName == "GPS") {
            m_gpsConnected = false;
            emit gpsConnected(false);
        } else if (portName == "IMU") {
            m_imuConnected = false;
            emit imuConnected(false);
        } else if (portName == "AutoSteer") {
            m_autosteerConnected = false;
            emit autosteerConnected(false);
        }
    }
}

// NMEA Processing Helpers
QStringList SerialWorker::extractNMEASentences(QByteArray& buffer)
{
    QStringList sentences;
    
    while (buffer.contains('\n')) {
        int index = buffer.indexOf('\n');
        QByteArray line = buffer.left(index);
        buffer.remove(0, index + 1);
        
        // Remove carriage return if present
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        
        QString sentence = QString::fromLatin1(line);
        if (!sentence.isEmpty()) {
            sentences << sentence;
        }
    }
    
    return sentences;
}

bool SerialWorker::isValidNMEASentence(const QString& sentence)
{
    // Basic NMEA validation
    if (sentence.length() < 7) return false;
    if (!sentence.startsWith('$')) return false;
    if (!sentence.contains('*')) return false;
    
    // Extract and verify checksum
    int checksumIndex = sentence.lastIndexOf('*');
    if (checksumIndex == -1 || checksumIndex >= sentence.length() - 2) return false;
    
    QString datapart = sentence.mid(1, checksumIndex - 1);
    QString providedChecksum = sentence.mid(checksumIndex + 1, 2);
    QString calculatedChecksum = calculateNMEAChecksum(datapart);
    
    return (providedChecksum.toUpper() == calculatedChecksum.toUpper());
}

QString SerialWorker::calculateNMEAChecksum(const QString& sentence)
{
    uint8_t checksum = 0;
    for (const QChar &ch : sentence) {
        checksum ^= ch.toLatin1();
    }
    return QString("%1").arg(checksum, 2, 16, QChar('0')).toUpper();
}

// ===== PHASE 5.2 - Dynamic 8-Port Architecture Implementation =====

void SerialWorker::initializeDynamicPorts()
{
    qDebug() << "🔧 Initializing dynamic ports for 5 new ports";

    // Initialize the 5 new ports with default values
    QStringList newPorts = {PORT_GPS2, PORT_MACHINE, PORT_RADIO, PORT_RTCM, PORT_TOOL};

    for (const QString& portType : newPorts) {
        m_serialPorts[portType] = nullptr;
        m_portBuffers[portType] = QByteArray();
        m_portConnected[portType] = false;
        m_portNames[portType] = QString("");
        m_baudRates[portType] = 9600; // Default baud rate
    }

    qDebug() << "✅ Dynamic ports initialized:" << newPorts;
}

// === Extended Port Management Methods ===

bool SerialWorker::openGPS2Port(const QString& portName, int baudRate)
{
    return openDynamicPort(PORT_GPS2, portName, baudRate);
}

bool SerialWorker::openMachinePort(const QString& portName, int baudRate)
{
    return openDynamicPort(PORT_MACHINE, portName, baudRate);
}

bool SerialWorker::openRadioPort(const QString& portName, int baudRate)
{
    return openDynamicPort(PORT_RADIO, portName, baudRate);
}

bool SerialWorker::openRTCMPort(const QString& portName, int baudRate)
{
    return openDynamicPort(PORT_RTCM, portName, baudRate);
}

bool SerialWorker::openToolPort(const QString& portName, int baudRate)
{
    return openDynamicPort(PORT_TOOL, portName, baudRate);
}

void SerialWorker::closeGPS2Port()
{
    closeDynamicPort(PORT_GPS2);
}

void SerialWorker::closeMachinePort()
{
    closeDynamicPort(PORT_MACHINE);
}

void SerialWorker::closeRadioPort()
{
    closeDynamicPort(PORT_RADIO);
}

void SerialWorker::closeRTCMPort()
{
    closeDynamicPort(PORT_RTCM);
}

void SerialWorker::closeToolPort()
{
    closeDynamicPort(PORT_TOOL);
}

// Generic port management
bool SerialWorker::openPort(const QString& portType, const QString& portName, int baudRate)
{
    if (portType == "GPS") return openGPSPort(portName, baudRate);
    if (portType == "IMU") return openIMUPort(portName, baudRate);
    if (portType == "AutoSteer") return openAutoSteerPort(portName, baudRate);

    // Dynamic ports
    return openDynamicPort(portType, portName, baudRate);
}

void SerialWorker::closePort(const QString& portType)
{
    if (portType == "GPS") { closeGPSPort(); return; }
    if (portType == "IMU") { closeIMUPort(); return; }
    if (portType == "AutoSteer") { closeAutoSteerPort(); return; }

    // Dynamic ports
    closeDynamicPort(portType);
}

QStringList SerialWorker::getActivePortTypes()
{
    QStringList activeTypes;

    // Check legacy ports
    if (m_gpsConnected) activeTypes << "GPS";
    if (m_imuConnected) activeTypes << "IMU";
    if (m_autosteerConnected) activeTypes << "AutoSteer";

    // Check dynamic ports
    for (auto it = m_portConnected.begin(); it != m_portConnected.end(); ++it) {
        if (it.value()) {
            activeTypes << it.key();
        }
    }

    return activeTypes;
}

// === Extended Data Transmission Methods ===

void SerialWorker::writeToGPS2(const QByteArray& data)
{
    writeToPort(PORT_GPS2, data);
}

void SerialWorker::writeToMachine(const QByteArray& data)
{
    writeToPort(PORT_MACHINE, data);
}

void SerialWorker::writeToRadio(const QByteArray& data)
{
    writeToPort(PORT_RADIO, data);
}

void SerialWorker::writeToRTCM(const QByteArray& data)
{
    writeToPort(PORT_RTCM, data);
}

void SerialWorker::writeToTool(const QByteArray& data)
{
    writeToPort(PORT_TOOL, data);
}

void SerialWorker::writeToPort(const QString& portType, const QByteArray& data)
{
    // Handle legacy ports
    if (portType == "GPS") { writeToGPS(data); return; }
    if (portType == "IMU") { writeToIMU(data); return; }
    if (portType == "AutoSteer") { writeToAutoSteer(data); return; }

    // Handle dynamic ports
    QSerialPort* port = m_serialPorts.value(portType, nullptr);
    if (port && port->isOpen()) {
        qint64 bytesWritten = port->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write to" << portType << "port:" << port->errorString();
            emit serialError(portType, port->errorString());
        } else {
            qDebug() << "📤" << portType << "data sent:" << bytesWritten << "bytes";
        }
    } else {
        qWarning() << portType << "port not open for writing";
        emit serialError(portType, "Port not open");
    }
}

// === Generic Status Queries ===

bool SerialWorker::isPortConnected(const QString& portType) const
{
    // Handle legacy ports
    if (portType == "GPS") return m_gpsConnected;
    if (portType == "IMU") return m_imuConnected;
    if (portType == "AutoSteer") return m_autosteerConnected;

    // Handle dynamic ports
    return m_portConnected.value(portType, false);
}

QString SerialWorker::getPortName(const QString& portType) const
{
    // Handle legacy ports
    if (portType == "GPS") return m_gpsPortName;
    if (portType == "IMU") return m_imuPortName;
    if (portType == "AutoSteer") return m_autosteerPortName;

    // Handle dynamic ports
    return m_portNames.value(portType, "");
}

int SerialWorker::getPortBaudRate(const QString& portType) const
{
    // Handle legacy ports
    if (portType == "GPS") return m_gpsBaudRate;
    if (portType == "IMU") return m_imuBaudRate;
    if (portType == "AutoSteer") return m_autosteerBaudRate;

    // Handle dynamic ports
    return m_baudRates.value(portType, 9600);
}

// === Extended Configuration Methods ===

void SerialWorker::configureGPS2Port(const QString& portName, int baudRate)
{
    configurePort(PORT_GPS2, portName, baudRate);
}

void SerialWorker::configureMachinePort(const QString& portName, int baudRate)
{
    configurePort(PORT_MACHINE, portName, baudRate);
}

void SerialWorker::configureRadioPort(const QString& portName, int baudRate)
{
    configurePort(PORT_RADIO, portName, baudRate);
}

void SerialWorker::configureRTCMPort(const QString& portName, int baudRate)
{
    configurePort(PORT_RTCM, portName, baudRate);
}

void SerialWorker::configureToolPort(const QString& portName, int baudRate)
{
    configurePort(PORT_TOOL, portName, baudRate);
}

void SerialWorker::configurePort(const QString& portType, const QString& portName, int baudRate)
{
    // Handle legacy ports
    if (portType == "GPS") { configureGPSPort(portName, baudRate); return; }
    if (portType == "IMU") { configureIMUPort(portName, baudRate); return; }
    if (portType == "AutoSteer") { configureAutoSteerPort(portName, baudRate); return; }

    // Handle dynamic ports
    if (m_portConnected.value(portType, false)) {
        closeDynamicPort(portType);
    }
    openDynamicPort(portType, portName, baudRate);
}

// === Extended Data Reception Handlers ===

void SerialWorker::onGPS2DataReady()
{
    handleDynamicPortData(PORT_GPS2);
}

void SerialWorker::onMachineDataReady()
{
    handleDynamicPortData(PORT_MACHINE);
}

void SerialWorker::onRadioDataReady()
{
    handleDynamicPortData(PORT_RADIO);
}

void SerialWorker::onRTCMDataReady()
{
    handleDynamicPortData(PORT_RTCM);
}

void SerialWorker::onToolDataReady()
{
    handleDynamicPortData(PORT_TOOL);
}

void SerialWorker::onPortDataReady()
{
    // This is called by sender() to determine which port triggered the signal
    QSerialPort* senderPort = qobject_cast<QSerialPort*>(sender());
    if (!senderPort) return;

    // Find the port type from the sender
    QString portType;
    for (auto it = m_serialPorts.begin(); it != m_serialPorts.end(); ++it) {
        if (it.value() == senderPort) {
            portType = it.key();
            break;
        }
    }

    if (!portType.isEmpty()) {
        handleDynamicPortData(portType);
    }
}

void SerialWorker::onPortError(QSerialPort::SerialPortError error)
{
    // This is called by sender() to determine which port had the error
    QSerialPort* senderPort = qobject_cast<QSerialPort*>(sender());
    if (!senderPort) return;

    // Find the port type from the sender
    QString portType;
    for (auto it = m_serialPorts.begin(); it != m_serialPorts.end(); ++it) {
        if (it.value() == senderPort) {
            portType = it.key();
            break;
        }
    }

    if (!portType.isEmpty()) {
        handleDynamicPortError(portType, error);
    }
}

// === Helper Methods - Dynamic Port Support ===

bool SerialWorker::openDynamicPort(const QString& portType, const QString& portName, int baudRate)
{
    qDebug() << "🔌 Opening" << portType << "port:" << portName << "at" << baudRate << "baud";

    // Close existing port if open
    if (m_serialPorts.contains(portType) && m_serialPorts[portType]) {
        closeDynamicPort(portType);
    }

    // Create new port
    QSerialPort* port = new QSerialPort(this);
    port->setPortName(portName);
    port->setBaudRate(baudRate);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);

    if (port->open(QIODevice::ReadWrite)) {
        // Store port and configuration
        m_serialPorts[portType] = port;
        m_portNames[portType] = portName;
        m_baudRates[portType] = baudRate;
        m_portConnected[portType] = true;
        m_portBuffers[portType].clear();

        // Setup connections
        setupPortConnections(portType, port);

        // Emit connection signal
        emit portConnected(portType, true);
        emitSpecificConnectionSignal(portType, true);

        qDebug() << "✅" << portType << "port opened successfully:" << portName;
        return true;
    } else {
        QString errorStr = port->errorString();
        qWarning() << "❌ Failed to open" << portType << "port:" << portName << "Error:" << errorStr;
        emit portOpenError(portName, errorStr);

        delete port;
        return false;
    }
}

void SerialWorker::closeDynamicPort(const QString& portType)
{
    if (m_serialPorts.contains(portType) && m_serialPorts[portType]) {
        QSerialPort* port = m_serialPorts[portType];
        if (port->isOpen()) {
            port->close();
            qDebug() << "🔌" << portType << "port closed";
        }
        delete port;
        m_serialPorts[portType] = nullptr;
    }

    // Update status
    m_portConnected[portType] = false;
    m_portNames[portType].clear();
    m_portBuffers[portType].clear();

    // Emit disconnection signal
    emit portConnected(portType, false);
    emitSpecificConnectionSignal(portType, false);
}

void SerialWorker::processPortBuffer(const QString& portType)
{
    if (!m_portBuffers.contains(portType)) return;

    QByteArray& buffer = m_portBuffers[portType];

    // Process buffer based on port type
    if (portType == PORT_GPS2) {
        // GPS2 uses NMEA like GPS
        QStringList sentences = extractNMEASentences(buffer);
        for (const QString& sentence : std::as_const(sentences)) {
            if (isValidNMEASentence(sentence)) {
                emit gps2DataReceived(sentence);
                emit portDataReceived(portType, sentence.toUtf8());
            }
        }
    } else if (portType == PORT_RTCM) {
        // RTCM data is binary, process in chunks
        if (buffer.size() >= 3) { // Minimum RTCM message size
            emit rtcmDataReceived(buffer);
            emit portDataReceived(portType, buffer);
            buffer.clear(); // Process all data
        }
    } else {
        // For Machine, Radio, Tool - process line-by-line
        while (buffer.contains('\n')) {
            int index = buffer.indexOf('\n');
            QByteArray line = buffer.left(index);
            buffer.remove(0, index + 1);

            if (line.endsWith('\r')) {
                line.chop(1);
            }

            if (!line.isEmpty()) {
                // Emit specific signal
                if (portType == PORT_MACHINE) {
                    emit machineDataReceived(line);
                } else if (portType == PORT_RADIO) {
                    emit radioDataReceived(line);
                } else if (portType == PORT_TOOL) {
                    emit toolDataReceived(line);
                }

                // Emit generic signal
                emit portDataReceived(portType, line);
            }
        }
    }
}

void SerialWorker::setupPortConnections(const QString& portType, QSerialPort* port)
{
    // Connect data reception - use generic handler
    connect(port, &QSerialPort::readyRead, this, &SerialWorker::onPortDataReady);

    // Connect error handling - use generic handler
    connect(port, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &SerialWorker::onPortError);

    qDebug() << "🔗 Port connections established for" << portType;
}

void SerialWorker::handleDynamicPortData(const QString& portType)
{
    if (!m_serialPorts.contains(portType) || !m_serialPorts[portType]) return;

    QSerialPort* port = m_serialPorts[portType];
    QByteArray data = port->readAll();
    m_portBuffers[portType].append(data);

    emit serialDataReceived(m_portNames[portType], data);
    processPortBuffer(portType);
}

void SerialWorker::handleDynamicPortError(const QString& portType, QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QSerialPort* port = m_serialPorts.value(portType, nullptr);
    if (!port) return;

    QString errorString = getErrorString(error);
    QString portName = m_portNames.value(portType, portType);

    qWarning() << "⚠️ Serial error on" << portType << "port (" << portName << "):" << errorString;
    emit serialError(portName, errorString);

    // Handle critical errors
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        closeDynamicPort(portType);
    }
}

void SerialWorker::emitSpecificConnectionSignal(const QString& portType, bool connected)
{
    // Emit specific connection signals for compatibility
    if (portType == PORT_GPS2) {
        emit gps2Connected(connected);
    } else if (portType == PORT_MACHINE) {
        emit machineConnected(connected);
    } else if (portType == PORT_RADIO) {
        emit radioConnected(connected);
    } else if (portType == PORT_RTCM) {
        emit rtcmConnected(connected);
    } else if (portType == PORT_TOOL) {
        emit toolConnected(connected);
    }
}

QString SerialWorker::getErrorString(QSerialPort::SerialPortError error)
{
    switch (error) {
        case QSerialPort::DeviceNotFoundError: return "Device not found";
        case QSerialPort::PermissionError: return "Permission denied";
        case QSerialPort::OpenError: return "Unable to open port";
        case QSerialPort::WriteError: return "Write error";
        case QSerialPort::ReadError: return "Read error";
        case QSerialPort::ResourceError: return "Resource error (device disconnected)";
        case QSerialPort::UnsupportedOperationError: return "Unsupported operation";
        case QSerialPort::TimeoutError: return "Timeout error";
        default: return "Unknown error";
    }
}

// ✅ CDC Compliance - Async methods implementation (replaces BlockingQueuedConnection)

void SerialWorker::openGPSPortAsync(const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openGPSPortAsync" << portName << baudRate;

    bool success = openGPSPort(portName, baudRate);
    QString errorMessage = success ? "" : "Failed to open GPS port";

    emit portOpenResult("GPS", success, errorMessage);
}

void SerialWorker::openIMUPortAsync(const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openIMUPortAsync" << portName << baudRate;

    bool success = openIMUPort(portName, baudRate);
    QString errorMessage = success ? "" : "Failed to open IMU port";

    emit portOpenResult("IMU", success, errorMessage);
}

void SerialWorker::openAutoSteerPortAsync(const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openAutoSteerPortAsync" << portName << baudRate;

    bool success = openAutoSteerPort(portName, baudRate);
    QString errorMessage = success ? "" : "Failed to open AutoSteer port";

    emit portOpenResult("AutoSteer", success, errorMessage);
}

void SerialWorker::openMachinePortAsync(const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openMachinePortAsync" << portName << baudRate;

    bool success = openMachinePort(portName, baudRate);
    QString errorMessage = success ? "" : "Failed to open Machine port";

    emit portOpenResult("Machine", success, errorMessage);
}

void SerialWorker::openToolPortAsync(const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openToolPortAsync" << portName << baudRate;

    bool success = openToolPort(portName, baudRate);
    QString errorMessage = success ? "" : "Failed to open Tool port";

    emit portOpenResult("Tool", success, errorMessage);
}

void SerialWorker::openPortAsync(const QString& portType, const QString& portName, int baudRate)
{
    qDebug() << "🔧 SerialWorker::openPortAsync" << portType << portName << baudRate;

    bool success = openPort(portType, portName, baudRate);
    QString errorMessage = success ? "" : QString("Failed to open %1 port").arg(portType);

    emit portOpenResult(portType, success, errorMessage);
}

void SerialWorker::checkConnectionStatusAsync(const QString& portType)
{
    qDebug() << "🔧 SerialWorker::checkConnectionStatusAsync" << portType;

    bool isConnected = isPortConnected(portType);
    emit connectionStatusResult(portType, isConnected);
}

void SerialWorker::getPortNameAsync(const QString& portType)
{
    qDebug() << "🔧 SerialWorker::getPortNameAsync" << portType;

    QString portName = getPortName(portType);
    emit portNameResult(portType, portName);
}

void SerialWorker::getPortBaudRateAsync(const QString& portType)
{
    qDebug() << "🔧 SerialWorker::getPortBaudRateAsync" << portType;

    int baudRate = getPortBaudRate(portType);
    emit portBaudRateResult(portType, baudRate);
}

void SerialWorker::scanAvailablePortsAsync()
{
    qDebug() << "🔧 SerialWorker::scanAvailablePortsAsync";

    QStringList ports = scanAvailablePorts();
    emit availablePortsResult(ports);
}
