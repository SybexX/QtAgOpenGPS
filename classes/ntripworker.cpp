#include "ntripworker.h"
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QHostAddress>
#include "agioservice.h"  // Pour utiliser Q_DECLARE_LOGGING_CATEGORY(agioservice)

NTRIPWorker::NTRIPWorker(QObject *parent)
    : QObject(parent)
    , m_tcpSocket(nullptr)
    , m_statusTimer(nullptr)
    , m_reconnectTimer(nullptr)
    , m_ntripPort(2101)
    , m_connectionState(Disconnected)
    , m_isRunning(false)
    , m_reconnectAttempts(0)
    , m_lastDataTime(0)
    , m_dataTimeoutMs(30000) // 30 second timeout
    , m_headerReceived(false)
    , m_bytesReceived(0)
    , m_packetsReceived(0)
    , m_dataRate(0.0)
    // ✅ PHASE 5.3 - Advanced RTCM Routing Configuration
    , m_rtcmPacketSize(256)              // Default RTCM packet size (C# compatible)
    , m_sendToSerialEnabled(false)       // Default: no serial routing (Qt modern architecture)
    , m_sendToUDPEnabled(true)           // Default: UDP broadcast to GPS modules
    , m_udpBroadcastAddress("255.255.255.255")  // Default broadcast address
    , m_udpBroadcastPort(10110)          // Standard RTCM UDP port
    , m_totalBytesRouted(0)
    , m_serialPacketsSent(0)
    , m_udpPacketsSent(0)
{
    qDebug() << "🔧 NTRIPWorker constructor - Thread:" << QThread::currentThread();

    qDebug() << "✅ PHASE 5.3 - Advanced RTCM routing initialized:"
             << "PacketSize:" << m_rtcmPacketSize
             << "Serial:" << m_sendToSerialEnabled
             << "UDP:" << m_sendToUDPEnabled;
    
    // Create TCP socket
    m_tcpSocket = new QTcpSocket(this);
    connect(m_tcpSocket, &QTcpSocket::connected, this, &NTRIPWorker::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NTRIPWorker::onTcpDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NTRIPWorker::onTcpDataReceived);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &NTRIPWorker::onTcpError);
    
    // Status timer for connection monitoring
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(5000); // 5s status check
    connect(m_statusTimer, &QTimer::timeout, this, &NTRIPWorker::checkConnectionStatus);
    
    // Reconnection timer
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(10000); // 10s reconnect attempts
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NTRIPWorker::attemptReconnect);
    
    qDebug() << "✅ NTRIPWorker created";
}

NTRIPWorker::~NTRIPWorker()
{
    qDebug() << "🔧 NTRIPWorker destructor";
    stopNTRIP();
}

void NTRIPWorker::startNTRIP(const QString& url, const QString& user, 
                            const QString& password, const QString& mount, int port)
{
    qDebug() << "🚀 Starting NTRIP worker - URL:" << url << "Mount:" << mount 
             << "Port:" << port << "Thread:" << QThread::currentThread();
    
    if (m_isRunning) {
        qDebug() << "⚠️ NTRIP worker already running";
        return;
    }
    
    // Store connection settings
    m_ntripUrl = url;
    m_ntripUser = user;
    m_ntripPassword = password;
    m_ntripMount = mount;
    m_ntripPort = port;
    
    // Validate settings
    if (!validateSettings()) {
        emit errorOccurred("Invalid NTRIP settings");
        return;
    }
    
    m_isRunning = true;
    m_reconnectAttempts = 0;
    resetStatistics();
    
    // Set initial state
    setState(Connecting);
    
    // Start connection
    m_tcpSocket->connectToHost(m_ntripUrl, m_ntripPort);
    
    // Start status monitoring
    m_statusTimer->start();
}

void NTRIPWorker::stopNTRIP()
{
    if (!m_isRunning) {
        return;
    }

    qDebug(agioservice) << "NTRIP worker stopping";

    m_isRunning = false;

    // Stop timers
    m_statusTimer->stop();
    m_reconnectTimer->stop();

    // Reset state for next connection
    m_headerReceived = false;
    m_receiveBuffer.clear();
    m_reconnectAttempts = 0;

    // Close connection
    cleanupConnection();

    // Set final state
    setState(Disconnected);

    qDebug(agioservice) << "NTRIP worker stopped";
}

void NTRIPWorker::onTcpConnected()
{
    qDebug() << "✅ NTRIP TCP connected to" << m_ntripUrl << ":" << m_ntripPort;
    
    setState(Authenticating);
    
    // Send NTRIP request
    sendNtripRequest();
}

void NTRIPWorker::onTcpDisconnected()
{
    qDebug() << "❌ NTRIP TCP disconnected";
    
    if (m_connectionState != Disconnected) {
        setState(Disconnected);
        
        // Schedule reconnection if running
        if (m_isRunning && m_reconnectAttempts < 5) {
            m_reconnectTimer->start();
        }
    }
}

void NTRIPWorker::onTcpDataReceived()
{
    if (!m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    // Read available data
    QByteArray data = m_tcpSocket->readAll();
    if (data.isEmpty()) {
        return;
    }
    
    // Add to receive buffer
    m_receiveBuffer.append(data);
    
    // Update statistics
    m_bytesReceived += data.size();
    m_lastDataTime = QDateTime::currentMSecsSinceEpoch();
    updateDataRate();
    
    // Process data based on current state
    if (m_connectionState == Authenticating && !m_headerReceived) {
        processNtripResponse();
    } else if (m_connectionState == ReceivingData) {
        // Process RTCM data
        processRtcmData(m_receiveBuffer);
        m_receiveBuffer.clear(); // Clear buffer after processing
        m_packetsReceived++;
        
        // Debug output (limited frequency)
        static int dataCounter = 0;
        if (++dataCounter % 20 == 0) { // Every 20 packets
            qDebug() << "📡 NTRIP data:" << data.size() << "bytes, Rate:" 
                    << QString::number(m_dataRate, 'f', 1) << "B/s";
        }
    }
}

void NTRIPWorker::onTcpError(QAbstractSocket::SocketError error)
{
    QString errorString = m_tcpSocket->errorString();
    qDebug() << "❌ NTRIP TCP error:" << error << errorString;
    
    setState(Error);
    emit errorOccurred("NTRIP connection error: " + errorString);
    
    // Schedule reconnection if running
    if (m_isRunning && m_reconnectAttempts < 5) {
        m_reconnectTimer->start();
    }
}

void NTRIPWorker::checkConnectionStatus()
{
    if (!m_isRunning) {
        return;
    }
    
    // Check for data timeout
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSinceData = currentTime - m_lastDataTime;
    
    if (m_connectionState == ReceivingData && timeSinceData > m_dataTimeoutMs) {
        qDebug() << "⚠️ NTRIP data timeout - last data" << timeSinceData << "ms ago";
        setState(Error);
        
        // Schedule reconnection
        if (m_reconnectAttempts < 5) {
            m_reconnectTimer->start();
        }
    }
}

void NTRIPWorker::attemptReconnect()
{
    if (!m_isRunning) {
        return;
    }
    
    m_reconnectAttempts++;
    qDebug() << "🔄 NTRIP reconnection attempt" << m_reconnectAttempts;
    
    // Clean up existing connection
    cleanupConnection();
    
    // Reset state
    m_headerReceived = false;
    m_receiveBuffer.clear();
    
    // Set connecting state
    setState(Connecting);
    
    // Attempt reconnection
    m_tcpSocket->connectToHost(m_ntripUrl, m_ntripPort);
}

void NTRIPWorker::sendNtripRequest()
{
    if (!m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    QString request = buildNtripRequest();
    QByteArray requestData = request.toUtf8();
    
    qDebug() << "📤 Sending NTRIP request:" << request.left(100) << "...";
    
    qint64 bytesWritten = m_tcpSocket->write(requestData);
    if (bytesWritten == -1) {
        qDebug() << "❌ Failed to send NTRIP request";
        setState(Error);
        emit errorOccurred("Failed to send NTRIP request");
    } else {
        qDebug() << "✅ NTRIP request sent:" << bytesWritten << "bytes";
    }
}

void NTRIPWorker::processNtripResponse()
{
    // Look for HTTP response header end
    int headerEndIndex = m_receiveBuffer.indexOf("\r\n\r\n");
    if (headerEndIndex == -1) {
        return; // Need more data
    }
    
    // Extract header
    QByteArray header = m_receiveBuffer.left(headerEndIndex);
    QString headerText = QString::fromUtf8(header);
    
    qDebug() << "📥 NTRIP response header:" << headerText.left(200) << "...";
    
    // Parse response
    if (parseNtripResponse(header)) {
        // Success - switch to data receiving mode
        setState(ReceivingData);
        m_headerReceived = true;
        
        // Remove header from buffer
        m_receiveBuffer.remove(0, headerEndIndex + 4);
        
        // Process any remaining data as RTCM
        if (!m_receiveBuffer.isEmpty()) {
            processRtcmData(m_receiveBuffer);
            m_receiveBuffer.clear();
        }
        
        qDebug() << "✅ NTRIP authentication successful, receiving data";
    } else {
        // Authentication failed
        setState(Error);
        emit errorOccurred("NTRIP authentication failed");
    }
}

void NTRIPWorker::setState(ConnectionState newState)
{
    if (m_connectionState != newState) {
        ConnectionState oldState = m_connectionState;
        m_connectionState = newState;
        
        qDebug() << "🔄 NTRIP state change:" << stateToString(oldState) 
                << "→" << stateToString(newState);
        
        // Emit status change
        emit ntripStatusChanged(static_cast<int>(newState), stateToString(newState));
    }
}

QString NTRIPWorker::stateToString(ConnectionState state) const
{
    switch (state) {
        case Disconnected: return "Disconnected";
        case Connecting: return "Connecting...";
        case Authenticating: return "Authenticating...";
        case WaitingForData: return "Connected - Waiting for Data";
        case ReceivingData: return "Connected - Receiving Data";
        case Error: return "Connection Error";
        default: return "Unknown State";
    }
}

void NTRIPWorker::resetStatistics()
{
    m_bytesReceived = 0;
    m_packetsReceived = 0;
    m_dataRate = 0.0;
    m_lastDataTime = 0;
}

void NTRIPWorker::updateDataRate()
{
    static qint64 lastUpdateTime = 0;
    static qint64 lastBytesReceived = 0;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (lastUpdateTime > 0) {
        qint64 timeDelta = currentTime - lastUpdateTime;
        qint64 bytesDelta = m_bytesReceived - lastBytesReceived;
        
        if (timeDelta > 0) {
            m_dataRate = (bytesDelta * 1000.0) / timeDelta; // bytes/second
        }
    }
    
    lastUpdateTime = currentTime;
    lastBytesReceived = m_bytesReceived;
}

QString NTRIPWorker::buildNtripRequest() const
{
    // Build NTRIP HTTP request
    QString request;
    request += QString("GET /%1 HTTP/1.1\r\n").arg(m_ntripMount);
    request += QString("Host: %1:%2\r\n").arg(m_ntripUrl).arg(m_ntripPort);
    request += "User-Agent: QtAgOpenGPS NTRIP Client\r\n";
    request += "Accept: */*\r\n";
    request += "Connection: close\r\n";
    
    // Add authentication if provided
    if (!m_ntripUser.isEmpty() && !m_ntripPassword.isEmpty()) {
        QString credentials = m_ntripUser + ":" + m_ntripPassword;
        QByteArray credentialsBase64 = credentials.toUtf8().toBase64();
        request += QString("Authorization: Basic %1\r\n").arg(QString::fromUtf8(credentialsBase64));
    }
    
    request += "\r\n";
    
    return request;
}

bool NTRIPWorker::parseNtripResponse(const QByteArray& response)
{
    QString responseText = QString::fromUtf8(response);
    
    // Check for successful HTTP response
    if (responseText.startsWith("HTTP/1.1 200 OK") || 
        responseText.startsWith("HTTP/1.0 200 OK")) {
        return true;
    }
    
    // Check for specific NTRIP responses
    if (responseText.contains("ICY 200 OK")) {
        return true; // NTRIP streaming response
    }
    
    // Log error responses
    if (responseText.contains("401")) {
        qDebug() << "❌ NTRIP authentication failed (401)";
    } else if (responseText.contains("404")) {
        qDebug() << "❌ NTRIP mount point not found (404)";
    } else if (responseText.contains("403")) {
        qDebug() << "❌ NTRIP access forbidden (403)";
    }
    
    return false;
}

void NTRIPWorker::processRtcmData(const QByteArray& data)
{
    if (data.isEmpty()) {
        return;
    }

    // ✅ PHASE 5.3 - Use advanced RTCM processing with routing
    processRTCMPackets(data);

    // Emit raw RTCM data for backward compatibility
    emit ntripDataReceived(data);

    // Update statistics
    if (m_connectionState != ReceivingData) {
        setState(ReceivingData);
    }
}

void NTRIPWorker::cleanupConnection()
{
    if (!m_tcpSocket) {
        return;
    }

    qDebug(agioservice) << "NTRIP cleanup connection, current state:" << m_tcpSocket->state();

    // PHASE 6.0.38: Asynchronous disconnect (Qt modern architecture)
    // Never use waitForDisconnected() - it blocks the thread and breaks signal/slot
    QAbstractSocket::SocketState currentState = m_tcpSocket->state();

    if (currentState == QAbstractSocket::ConnectedState ||
        currentState == QAbstractSocket::ConnectingState ||
        currentState == QAbstractSocket::HostLookupState) {
        // Graceful disconnect - let Qt signals handle it asynchronously
        m_tcpSocket->disconnectFromHost();
        qDebug(agioservice) << "NTRIP disconnectFromHost called, state will change via signals";
    }
    else if (currentState != QAbstractSocket::UnconnectedState) {
        // Force abort for other states
        m_tcpSocket->abort();
        qDebug(agioservice) << "NTRIP connection aborted";
    }
    else {
        qDebug(agioservice) << "NTRIP socket already disconnected";
    }
}

bool NTRIPWorker::validateSettings() const
{
    if (m_ntripUrl.isEmpty()) {
        qDebug() << "❌ NTRIP URL is empty";
        return false;
    }
    
    if (m_ntripMount.isEmpty()) {
        qDebug() << "❌ NTRIP mount point is empty";
        return false;
    }
    
    if (m_ntripPort < 1 || m_ntripPort > 65535) {
        qDebug() << "❌ NTRIP port is invalid:" << m_ntripPort;
        return false;
    }
    
    return true;
}

// ===== PHASE 5.3 - Advanced NTRIP Configuration & RTCM Routing =====

void NTRIPWorker::configurePacketSize(int size)
{
    qDebug() << "⚙️ Configuring RTCM packet size:" << size << "bytes";

    if (size < 64 || size > 1024) {
        qWarning() << "⚠️ Invalid packet size, using default 256 bytes";
        m_rtcmPacketSize = 256;
    } else {
        m_rtcmPacketSize = size;
    }

    qDebug() << "✅ RTCM packet size configured:" << m_rtcmPacketSize << "bytes";
}

void NTRIPWorker::enableSerialRouting(bool enable)
{
    qDebug() << "⚙️ RTCM serial routing:" << (enable ? "enabled" : "disabled");

    m_sendToSerialEnabled = enable;

    if (!enable && !m_sendToUDPEnabled) {
        qWarning() << "⚠️ Warning: Both serial and UDP routing disabled - RTCM data will not be forwarded!";
    }

    qDebug() << "✅ RTCM serial routing configured:" << m_sendToSerialEnabled;
}

void NTRIPWorker::enableUDPBroadcast(bool enable)
{
    qDebug() << "⚙️ RTCM UDP broadcast:" << (enable ? "enabled" : "disabled");

    m_sendToUDPEnabled = enable;

    if (!enable && !m_sendToSerialEnabled) {
        qWarning() << "⚠️ Warning: Both serial and UDP routing disabled - RTCM data will not be forwarded!";
    }

    qDebug() << "✅ RTCM UDP broadcast configured:" << m_sendToUDPEnabled;
}

void NTRIPWorker::setRoutingTargets(const QStringList& serialPorts, const QString& udpAddress, int udpPort)
{
    qDebug() << "⚙️ Configuring RTCM routing targets:"
             << "Serial ports:" << serialPorts
             << "UDP:" << udpAddress << ":" << udpPort;

    m_serialRoutingTargets = serialPorts;
    m_udpBroadcastAddress = udpAddress.isEmpty() ? "255.255.255.255" : udpAddress;
    m_udpBroadcastPort = (udpPort > 0 && udpPort < 65536) ? udpPort : 10110;

    qDebug() << "✅ RTCM routing targets configured:"
             << "Serial:" << m_serialRoutingTargets.size() << "ports"
             << "UDP:" << m_udpBroadcastAddress << ":" << m_udpBroadcastPort;
}

void NTRIPWorker::processRTCMPackets(const QByteArray& data)
{
    // Add new data to buffer
    m_rtcmBuffer.append(data);

    // Process complete RTCM packets
    while (m_rtcmBuffer.size() >= 3) { // Minimum RTCM packet size
        // Find RTCM packet start (0xD3)
        int packetStart = -1;
        for (int i = 0; i < m_rtcmBuffer.size(); ++i) {
            if (static_cast<quint8>(m_rtcmBuffer[i]) == 0xD3) {
                packetStart = i;
                break;
            }
        }

        if (packetStart == -1) {
            // No RTCM packet found, clear buffer
            m_rtcmBuffer.clear();
            break;
        }

        // Remove data before packet start
        if (packetStart > 0) {
            m_rtcmBuffer.remove(0, packetStart);
        }

        // Check if we have enough data for packet length
        if (m_rtcmBuffer.size() < 3) break;

        // Extract packet length
        quint16 packetLength = ((static_cast<quint8>(m_rtcmBuffer[1]) & 0x03) << 8) |
                               static_cast<quint8>(m_rtcmBuffer[2]);
        int totalPacketSize = packetLength + 6; // Header(3) + Data + CRC(3)

        // Check if complete packet is available
        if (m_rtcmBuffer.size() < totalPacketSize) {
            break; // Wait for more data
        }

        // Extract complete packet
        QByteArray packet = m_rtcmBuffer.left(totalPacketSize);
        m_rtcmBuffer.remove(0, totalPacketSize);

        // Validate and route packet
        if (isValidRTCMPacket(packet)) {
            routeRTCMPacket(packet);
        } else {
            qDebug() << "⚠️ Invalid RTCM packet discarded, size:" << packet.size();
        }
    }
}

void NTRIPWorker::routeRTCMPacket(const QByteArray& packet)
{
    if (packet.isEmpty()) return;

    qDebug() << "📏 Routing RTCM packet:" << packet.size() << "bytes"
             << "Serial:" << m_sendToSerialEnabled
             << "UDP:" << m_sendToUDPEnabled;

    // Route to serial ports if enabled
    if (m_sendToSerialEnabled) {
        sendRTCMToSerial(packet);
    }

    // Broadcast to UDP if enabled
    if (m_sendToUDPEnabled) {
        sendRTCMToUDP(packet);
    }

    // Emit routing signals for AgIOService integration
    if (m_sendToSerialEnabled) {
        emit routeRTCMToSerial(packet);
    }
    if (m_sendToUDPEnabled) {
        emit broadcastRTCMToUDP(packet);
    }

    // Update statistics
    m_totalBytesRouted += packet.size();
    emit rtcmPacketProcessed(packet.size(), m_sendToSerialEnabled ? "Serial" : "UDP");
}

void NTRIPWorker::sendRTCMToSerial(const QByteArray& packet)
{
    // This will be connected to SerialWorker via AgIOService
    // For now, just emit the signal
    qDebug() << "📶 Routing RTCM to serial ports:" << m_serialRoutingTargets.size() << "targets";

    m_serialPacketsSent++;
    updateRoutingStatistics("Serial", packet.size());
}

void NTRIPWorker::sendRTCMToUDP(const QByteArray& packet)
{
    // This will be connected to UDPWorker via AgIOService
    // For now, just emit the signal
    qDebug() << "📶 Broadcasting RTCM to UDP:" << m_udpBroadcastAddress << ":" << m_udpBroadcastPort;

    m_udpPacketsSent++;
    updateRoutingStatistics("UDP", packet.size());
}

bool NTRIPWorker::isValidRTCMPacket(const QByteArray& packet) const
{
    if (packet.size() < 6) return false; // Minimum RTCM packet size

    // Check RTCM header (0xD3)
    if (static_cast<quint8>(packet[0]) != 0xD3) {
        return false;
    }

    // Extract and validate length
    quint16 declaredLength = ((static_cast<quint8>(packet[1]) & 0x03) << 8) |
                            static_cast<quint8>(packet[2]);
    int expectedSize = declaredLength + 6;

    if (packet.size() != expectedSize) {
        return false;
    }

    // Basic length validation
    if (declaredLength > 1023) { // RTCM maximum message length
        return false;
    }

    return true;
}

void NTRIPWorker::updateRoutingStatistics(const QString& destination, int bytes)
{
    qDebug() << "📊 RTCM routing stats:" << destination << bytes << "bytes"
             << "Total routed:" << m_totalBytesRouted
             << "Serial packets:" << m_serialPacketsSent
             << "UDP packets:" << m_udpPacketsSent;
}