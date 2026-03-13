// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// AgIOService UDP Socket Implementation - Event-Driven Main Thread Architecture
// Phase 6.0.24: COMPLETE MIGRATION - All UDPWorker functionality migrated to main thread
//
// This file is INCLUDED in agioservice.cpp, not compiled separately
// Pattern follows settingsmanager_implementations.cpp for modular code organization

#include <QUdpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <QTime>
#include <QNetworkInterface>
#include <QElapsedTimer>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

// ========== UDP Socket Lifecycle ==========

void AgIOService::initializeUdpSocket()
{
    qDebug() << "Phase 6.0.24: Initializing UDP socket (main thread event-driven)";

    // Create UDP socket if not exists
    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);

        // Connect readyRead signal for event-driven data reception
        connect(m_udpSocket, &QUdpSocket::readyRead,
                this, &AgIOService::onUdpDataReady);

        // Connect error signal
        connect(m_udpSocket, &QAbstractSocket::errorOccurred,
                this, &AgIOService::onUdpError);

        qDebug() << "UDP socket created and connected (event-driven, main thread)";
    }

    // Initialize module status monitoring
    initializeModuleStatus();
}

void AgIOService::startUDP(const QString& address, int port)
{
    qCDebug(agioservice) << "Phase 6.0.24: Starting UDP (main thread) - Address:" << address << "Port:" << port
             << "Listen:" << m_listenPort;

    if (m_isRunning) {
        qCDebug(agioservice) << "UDP already running";
        return;
    }

    // Parse target address
    if (!address.isEmpty()) {
        m_tractorAddress = QHostAddress(address);
        if (m_tractorAddress.isNull()) {
            m_tractorAddress = QHostAddress::LocalHost;
        }
    }

    if (port > 0) {
        m_tractorPort = port;
    }

    m_isRunning = true;
    m_isUdpEnabled = true;
    resetStatistics();

    // Bind socket for listening
    if (bindSocket()) {
        m_isConnected = true;

        // Start timers
        m_statusTimer->start();
        m_udpHeartbeatTimer->start();

        emit udpStatusChanged(true);
        qCDebug(agioservice) << "UDP started successfully (main thread)";

        // Send initial heartbeat
        sendHeartbeat();

        // Send initial scan
        QTimer::singleShot(1000, this, [this]() {
            qDebug() << "Sending initial PGN 202 scan...";
            wakeUpModules("255.255.255.255");
        });
    } else {
        m_isRunning = false;
        m_isConnected = false;
        emit udpStatusChanged(false);
        emit errorOccurred("Failed to bind UDP socket");
    }
}

void AgIOService::stopUDP()
{
    if (!m_isRunning) {
        return;
    }

    qDebug() << "Phase 6.0.24: Stopping UDP (main thread)";

    m_isRunning = false;
    m_isConnected = false;
    m_isUdpEnabled = false;

    // Stop timers
    m_statusTimer->stop();
    m_udpHeartbeatTimer->stop();

    // Close socket
    cleanupConnection();

    emit udpStatusChanged(false);
}

bool AgIOService::bindSocket()
{
    if (!m_udpSocket) {
        return false;
    }

    if (m_udpSocket->state() == QAbstractSocket::BoundState) {
        qCDebug(agioservice) << "UDP socket already bound to port" << m_udpSocket->localPort();
        return true;
    }

    QHostAddress listenAddress = QHostAddress::Any;

    if (m_udpSocket->bind(listenAddress, m_listenPort, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qCDebug(agioservice) << "UDP socket bound to port" << m_listenPort;
        return true;
    } else {
        qCDebug(agioservice) << "Failed to bind UDP socket to port" << m_listenPort
                << ":" << m_udpSocket->errorString();

        // Try alternative ports
        QList<int> alternativePorts = {9999, 10001, 10002, 10003, 7777, 6666};
        for (int port : alternativePorts) {
            if (m_udpSocket->bind(listenAddress, port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
                qCDebug(agioservice) << "UDP socket bound to alternative port" << port;
                m_listenPort = port;
                return true;
            }
        }

        return false;
    }
}

void AgIOService::cleanupConnection()
{
    if (m_udpSocket) {
        if (m_udpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_udpSocket->close();
        }
    }
}

// ========== UDP Communication ==========

void AgIOService::sendToTractor(const QByteArray& data)
{
    if (!m_isRunning || !m_udpSocket || data.isEmpty()) {
        return;
    }

    // Phase 6.0.24: Validate address and port before sending
    if (m_tractorAddress.isNull() || m_tractorPort <= 0) {
        qDebug() << "Cannot send to tractor - invalid address or port";
        return;
    }

    qint64 bytesWritten = m_udpSocket->writeDatagram(data, m_tractorAddress, m_tractorPort);

    if (bytesWritten == -1) {
        qDebug() << "Failed to send UDP data:" << m_udpSocket->errorString();
        emit errorOccurred("Failed to send UDP data: " + m_udpSocket->errorString());
    } else {
        m_bytesSent += bytesWritten;
        m_packetsSent++;
        incrementUdpCounters(0, true);
        updateSendRate();

        static int sendCounter = 0;
        if (++sendCounter % 50 == 0) {
            qDebug() << "UDP data sent:" << bytesWritten << "bytes";
        }
    }
}

void AgIOService::sendRawMessage(const QByteArray& data, const QString& address, int port)
{
    if (!m_isRunning || data.isEmpty()) {
        qDebug() << "Cannot send raw message - UDP not running or invalid data";
        return;
    }

    QHostAddress targetAddr(address);
    if (targetAddr.isNull()) {
        qDebug() << "Invalid target address:" << address;
        return;
    }

    qDebug() << "Sending raw message via multi-interface broadcast";
    sendToAllInterfaces(data, address, port);
}

void AgIOService::sendModuleData(const QByteArray& moduleData)
{
    sendToTractor(moduleData);
}

QByteArray AgIOService::buildHeartbeatPacket() const
{
    QByteArray heartbeat;
    heartbeat.resize(8);

    heartbeat[0] = 0xAA;
    heartbeat[1] = 0x55;
    heartbeat[2] = 0xFF;
    heartbeat[3] = 0x02;
    heartbeat[4] = 0x01;
    heartbeat[5] = 0x00;

    quint8 checksum = 0;
    for (int i = 2; i < 6; ++i) {
        checksum ^= static_cast<quint8>(heartbeat[i]);
    }
    heartbeat[6] = static_cast<char>(checksum);
    heartbeat[7] = 0xCC;

    return heartbeat;
}

void AgIOService::sendHeartbeat()
{
    if (!m_isRunning) {
        return;
    }

    QByteArray heartbeat = buildHeartbeatPacket();
    sendToTractor(heartbeat);
}

// ========== UDP Event Handlers ==========

void AgIOService::onUdpDataReady()
{
    static quint32 udpInCount = 0;
    static quint32 udpInBytes = 0;
    static qint64 lastStatsUpdate = 0;

    // Phase 6.0.24: UDP throttling removed - parse ALL packets
    // Reason: Throttling artificially dropped 66% of packets in bursts from firmware
    // Display throttling at 10 Hz (formgps_position.cpp:2011) is sufficient for CPU management
    // UDP kernel buffer handles bursts naturally - no need to drop packets here

    // Debug: Count packets per second by source port to verify actual UDP traffic rate
    static QMap<quint16, int> packetsPerPort;  // Port → packet count
    static int packetsThisSecond = 0;
    static qint64 lastPacketCountReset = 0;

    // qCDebug(agioservice) << "📥 onUdpDataReady() called - Pending datagrams:" << m_udpSocket->pendingDatagramSize();

    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                                   &sender, &senderPort);

        // qCDebug(agioservice) << "📦 Received" << datagram.size() << "bytes from" << sender.toString() << ":" << senderPort;

        if (datagram.isEmpty()) continue;

        // Debug: Count packets by port and display rate every second
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (lastPacketCountReset == 0) lastPacketCountReset = now;

        packetsThisSecond++;
        packetsPerPort[senderPort]++;

        if (now - lastPacketCountReset >= 1000) {
            qCDebug(agioservice) << "🚦 UDP TRAFFIC BY PORT:";
            for (auto it = packetsPerPort.begin(); it != packetsPerPort.end(); ++it) {
                qCDebug(agioservice) << "   Port" << it.key() << ":" << it.value() << "packets/sec";
            }
            qCDebug(agioservice) << "   TOTAL:" << packetsThisSecond << "packets/sec";
            packetsPerPort.clear();
            packetsThisSecond = 0;
            lastPacketCountReset = now;
        }

        // ✅ Problem 10 Fix: Convert IPv6-mapped IPv4 address (::ffff:192.168.1.126) to IPv4 (192.168.1.126)
        // IPv6-mapped addresses cause source.split(":") to fail (4 parts instead of 2)
        QString senderIP = sender.toString();
        if (senderIP.startsWith("::ffff:")) {
            senderIP = senderIP.mid(7);  // Remove "::ffff:" prefix
            // qCDebug(agioservice) << "🔧 Converted IPv6-mapped address to IPv4:" << senderIP;
        }

        QString source = "UDP:" + senderIP;

        udpInCount++;
        udpInBytes += datagram.size();

        // Reuse 'now' from packet counter above (already declared at line 290)
        if (lastStatsUpdate == 0) lastStatsUpdate = now;

        if (now - lastStatsUpdate >= 1000) {
            double elapsedSec = (now - lastStatsUpdate) / 1000.0;
            double inRate = udpInBytes / elapsedSec;
            double inFreq = udpInCount / elapsedSec;

            if (m_traffic) {
                m_traffic->setCntrUDPIn(udpInCount);
                m_traffic->setCntrUDPInBytes(udpInBytes);
                m_traffic->setUdpInRate(inRate);
                m_traffic->setUdpInFreq(inFreq);
            }

            lastStatsUpdate = now;

            // ✅ Problem 12 Fix: Reset counters after calculating stats
            // Without reset, counters accumulate → 26 Hz becomes 52, 78, 10000 Hz!
            udpInCount = 0;
            udpInBytes = 0;
        }

        // Phase 6.0.24: Parse ALL packets without throttling
        // Firmware sends bursts (3-4 packets in <10ms), we must process them all
        // Display throttling (10 Hz) happens in FormGPS onParsedDataReady()

        QStringList sourceParts = source.split(":");
        // qCDebug(agioservice) << "🔍 Source split - Parts:" << sourceParts.size() << "Source:" << source;

        if (sourceParts.size() != 2) {
            // qCDebug(agioservice) << "⚠️ Invalid source format (expected 2 parts):" << sourceParts;
            continue;
        }

        QString transport = sourceParts[0];
        QString sourceID = sourceParts[1];
        // qCDebug(agioservice) << "✅ Source valid - Transport:" << transport << "ID:" << sourceID;

        PGNParser::ParsedData parsedData = m_pgnParser->parse(datagram);

        // qCDebug(agioservice) << "🔍 Parsing result - isValid:" << parsedData.isValid
        //                      << "Type:" << parsedData.sourceType
        //                      << "Sentence/PGN:" << (parsedData.sourceType == "NMEA" ? parsedData.sentenceType : QString::number(parsedData.pgnNumber));

        // ✅ Problem 14 Fixed: IMU values now correctly converted (/10.0) and assigned at 40 Hz
        // Debug logs removed - IMU data stable

        if (!parsedData.isValid) {
            // qCDebug(agioservice) << "⚠️ Skipping invalid parsed data";
            continue;
        }

        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        parsedData.sourceTransport = transport;
        parsedData.sourceID = sourceID;
        parsedData.timestampMs = timestamp;

        QString protocolId;
        if (parsedData.sourceType == "NMEA") {
            protocolId = "$" + parsedData.sentenceType;
        } else if (parsedData.sourceType == "PGN") {
            protocolId = "PGN" + QString::number(parsedData.pgnNumber);
        } else {
            continue;
        }

        bool acceptData = shouldAcceptData(protocolId, transport);
        // qCDebug(agioservice) << "🎯 shouldAcceptData(" << protocolId << "," << transport << ") =" << acceptData;

        if (!acceptData) {
            // qCDebug(agioservice) << "⚠️ Data rejected by shouldAcceptData filter";
            continue;
        }

        updateModuleStatus(protocolId, transport, sourceID, timestamp);

        static int logCounter = 0;
        if (++logCounter % 10 == 0) {
            if (parsedData.sourceType == "NMEA") {
                // qCDebug(agioservice) << "✅ PARSED NMEA from" << source << "-" << parsedData.sentenceType;
            } else if (parsedData.sourceType == "PGN") {
                // qCDebug(agioservice) << "✅ PARSED PGN" << parsedData.pgnNumber << "from" << source;
            }
        }

        static int propertyUpdateCounter = 0;
        if (++propertyUpdateCounter % 4 == 0) {
            if (parsedData.isValid && parsedData.sourceType == "NMEA") {
                // qCDebug(agioservice) << "📝 Updating GPS properties - Quality:" << parsedData.quality
                //                      << "Satellites:" << parsedData.satellites
                //                      << "Lat:" << parsedData.latitude << "Lon:" << parsedData.longitude;

                setGpsQuality(parsedData.quality);
                setSatellites(parsedData.satellites);
                if (!gpsConnected()) setGpsConnected(true);

                QString sentence = QString::fromUtf8(datagram).trimmed();
                if (parsedData.sentenceType == "GGA") setGgaSentence(sentence);
                else if (parsedData.sentenceType == "VTG") setVtgSentence(sentence);
                else if (parsedData.sentenceType == "RMC") setRmcSentence(sentence);
                else if (parsedData.sentenceType == "PANDA") setPandaSentence(sentence);
                else if (parsedData.sentenceType == "PAOGI") setPaogiSentence(sentence);
                else if (parsedData.sentenceType == "HDT") setHdtSentence(sentence);
                else if (parsedData.sentenceType == "AVR") setAvrSentence(sentence);
                else if (parsedData.sentenceType == "KSXT") setSxtSentence(sentence);
            }
            else if (parsedData.sourceType == "PGN") {
                if (parsedData.pgnNumber == 126 || parsedData.pgnNumber == 253) {
                    if (!steerConnected()) setSteerConnected(true);
                }
                if (parsedData.pgnNumber == 211) {
                    if (!imuConnected()) setImuConnected(true);
                }
                if (parsedData.pgnNumber == 122) {
                    if (!rateControlConnected()) setRateControlConnected(true);
                }
                if (parsedData.pgnNumber == 123) {
                    if (!machineConnected()) setMachineConnected(true);
                }
                if (parsedData.pgnNumber == 124) {
                    if (!blockageConnected()) setBlockageConnected(true);
                }
            }
        }

        // Phase 6.0.25: Route data to specialized signals
        if (parsedData.sourceType == "NMEA") {
            // NMEA sentences → GPS position data (~8 Hz)
            emit nmeaDataReady(parsedData);

        } else if (parsedData.sourceType == "PGN") {
            switch (parsedData.pgnNumber) {
                case 203:  // Module scan reply (PGN 203 response to PGN 202 scan request)
                    // Phase 6.0.38: Extract sender IP and update discovery properties when module responds
                    {
                        // Call processHelloMessage to reset module counters
                        processHelloMessage(datagram);

                        // Extract subnet from sender IP (already cleaned from IPv6-mapped at line 312)
                        QString subnet = getSubnetFromIP(senderIP);

                        qDebug() << "📡 PGN 203 scan reply from" << senderIP << "subnet:" << subnet;

                        // Update discovery properties (Qt 6.8 Rectangle Pattern)
                        m_discoveredModuleIP.setValue(senderIP);
                        m_discoveredModuleSubnet.setValue(subnet);

                        // Stop the scan - module found!
                        if (m_subnetScanActive) {
                            qDebug() << "✅ Module discovered - stopping subnet scan";
                            m_subnetScanActive = false;

                            // Stop the scan timer
                            if (m_subnetScanTimer) {
                                m_subnetScanTimer->stop();
                            }

                            // Emit completion signal with the discovered subnet
                            emit subnetScanCompleted(subnet);
                        }
                    }
                    break;

                case 211:  // External IMU module
                    // PGN 211 → IMU data (~10 Hz)
                    emit imuDataReady(parsedData);
                    break;

                case 253:  // AutoSteer status (actual angle, switches, PWM)
                case 250:  // AutoSteer sensor (pressure/current)
                    // PGN 253/250 → AutoSteer feedback (~40 Hz throttled by timer)
                    emit steerDataReady(parsedData);
                    break;
                case 244:  // Blockage Data
                    emit blockageDataReady(parsedData);
                    break;

                case 240:  // RateControl Data
                    emit rateControlDataReady(parsedData);
                    break;

                case 212:  // IMU disconnect sentinel
                    // PGN 212 → IMU disconnect (set sentinel values)
                    emit imuDataReady(parsedData);
                    break;

                case 221:  // Hardware messages (UTF-8 text)
                    // TODO Phase 6.0.26: Add hardwareMessageReceived signal
                    qDebug() << "PGN 221 hardware message (not yet implemented)";
                    break;

                case 234:  // Remote switches
                    // TODO Phase 6.0.26: Add remoteSwitchesChanged signal
                    qDebug() << "PGN 234 remote switches (not yet implemented)";
                    break;

                default:
                    // Unknown PGN - log for debugging (but don't spam for common PGNs)
                    static int unknownPgnCounter = 0;
                    if (++unknownPgnCounter % 10 == 1) {
                        qDebug() << "Unknown PGN" << parsedData.pgnNumber << "- ignoring";
                    }
                    break;
            }
        }

        // Phase 6.0.27: DISABLED legacy parsedDataReady signal emission
        // Migration complete - using separated signals (nmeaDataReady, imuDataReady, steerDataReady)
        // No active connections to this signal → commenting to eliminate 717 emissions/sec waste
        // Backward compatibility: emit old signal for migration period
        // emit parsedDataReady(parsedData);
    }
}

void AgIOService::onUdpError(QAbstractSocket::SocketError error)
{
    QString errorString = m_udpSocket->errorString();
    qDebug() << "UDP socket error:" << error << errorString;

    emit errorOccurred("UDP socket error: " + errorString);

    if (m_isRunning) {
        cleanupConnection();

        QTimer::singleShot(2000, this, [this]() {
            if (m_isRunning) {
                bindSocket();
            }
        });
    }
}

void AgIOService::checkConnectionStatus()
{
    if (!m_isRunning) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSinceData = currentTime - m_lastDataTime;

    bool previouslyConnected = m_isConnected;
    m_isConnected = (m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState);

    if (previouslyConnected != m_isConnected) {
        emit udpStatusChanged(m_isConnected);
        qDebug() << "UDP connection status changed:" << m_isConnected;
    }

    static int statusCounter = 0;
    if (++statusCounter % 25 == 0) {
        qDebug() << "UDP stats - RX:" << m_packetsReceived << "packets,"
                << QString::number(m_receiveRate, 'f', 1) << "B/s"
                << "TX:" << m_packetsSent << "packets,"
                << QString::number(m_sendRate, 'f', 1) << "B/s";
    }
}

// ========== Statistics ==========

void AgIOService::updateReceiveRate()
{
    static qint64 lastReceiveTime = 0;
    static qint64 lastBytesReceived = 0;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (lastReceiveTime > 0) {
        qint64 timeDelta = currentTime - lastReceiveTime;
        qint64 bytesDelta = m_bytesReceived - lastBytesReceived;

        if (timeDelta > 0) {
            m_receiveRate = (bytesDelta * 1000.0) / timeDelta;
        }
    }

    lastReceiveTime = currentTime;
    lastBytesReceived = m_bytesReceived;
}

void AgIOService::updateSendRate()
{
    static qint64 lastSendTime = 0;
    static qint64 lastBytesSent = 0;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (lastSendTime > 0) {
        qint64 timeDelta = currentTime - lastSendTime;
        qint64 bytesDelta = m_bytesSent - lastBytesSent;

        if (timeDelta > 0) {
            m_sendRate = (bytesDelta * 1000.0) / timeDelta;
        }
    }

    lastSendTime = currentTime;
    lastBytesSent = m_bytesSent;
}

void AgIOService::resetStatistics()
{
    m_bytesReceived = 0;
    m_bytesSent = 0;
    m_packetsReceived = 0;
    m_packetsSent = 0;
    m_receiveRate = 0.0;
    m_sendRate = 0.0;
    m_lastDataTime = 0;
}

// ========== Module Discovery ==========

void AgIOService::startModuleDiscovery()
{
    if (m_discoveryActive) {
        qDebug() << "Module discovery already active";
        return;
    }

    qDebug() << "Starting module discovery";
    m_discoveryActive = true;

    // Qt 6.8 Rectangle Pattern: Get value, modify, set back
    QVariantList modules = m_discoveredModules.value();
    modules.clear();
    m_discoveredModules.setValue(modules);

    m_moduleLastSeen.clear();

    m_discoveryTimer->start();
    m_timeoutTimer->start();

    wakeUpModules("255.255.255.255");
}

void AgIOService::stopModuleDiscovery()
{
    qDebug() << "Stopping module discovery";
    m_discoveryActive = false;
    m_timeoutTimer->stop();

    // Convert QVariantList to QStringList for signal
    QVariantList variantModules = m_discoveredModules.value();
    QStringList stringModules;
    for (const QVariant& v : variantModules) {
        stringModules.append(v.toString());
    }
    emit networkScanCompleted(stringModules);
}

void AgIOService::scanSubnet(const QString& baseIP)
{
    m_currentSubnet = baseIP;
    qDebug() << "Scanning subnet:" << baseIP;

    QStringList ipParts = baseIP.split('.');
    if (ipParts.size() >= 3) {
        QString subnet = QString("%1.%2.%3").arg(ipParts[0], ipParts[1], ipParts[2]);

        QStringList commonIPs = {
            subnet + ".100",
            subnet + ".101",
            subnet + ".102",
            subnet + ".200",
            subnet + ".201"
        };

        for (const QString& ip : commonIPs) {
            pingModule(ip);
        }
    }
}

void AgIOService::pingModule(const QString& moduleIP)
{
    qDebug() << "Pinging module:" << moduleIP;
    sendHelloMessage(moduleIP);
}

void AgIOService::wakeUpModules(const QString& broadcastIP)
{
    qDebug() << "Waking up dormant modules with broadcast to:" << broadcastIP;

    if (!m_udpSocket) {
        qDebug() << "UDP socket not available for wakeup broadcast";
        return;
    }

    QByteArray helloMessage;
    helloMessage.append(char(0x80));
    helloMessage.append(char(0x81));
    helloMessage.append(char(0x7F));
    helloMessage.append(char(202));
    helloMessage.append(char(3));
    helloMessage.append(char(202));
    helloMessage.append(char(202));
    helloMessage.append(char(5));
    helloMessage.append(char(0x47));

    qDebug() << "Sending wakeup broadcast via all interfaces";
    sendToAllInterfaces(helloMessage, broadcastIP, 8888);
}

void AgIOService::broadcastHello()
{
    if (!m_discoveryActive || !m_udpSocket) {
        return;
    }

    QByteArray helloMessage;
    helloMessage.append(char(0x80));
    helloMessage.append(char(0x81));
    helloMessage.append(char(0x7F));
    helloMessage.append(char(202));
    helloMessage.append(char(3));
    helloMessage.append(char(202));
    helloMessage.append(char(202));
    helloMessage.append(char(5));
    helloMessage.append(char(0x47));

    static int broadcastCount = 0;
    if (++broadcastCount % 10 == 1) {
        qDebug() << "Broadcast hello #" << broadcastCount << "(PGN 202)";
    }

    QHostAddress broadcast = QHostAddress::Broadcast;
    qint64 sent = m_udpSocket->writeDatagram(helloMessage, broadcast, 8888);

    if (sent > 0) {
        m_packetsSent++;
        m_bytesSent += helloMessage.size();
        updateSendRate();
        incrementUdpCounters(0, true);
    }
}

void AgIOService::sendHelloMessage(const QString& moduleIP)
{
    if (!m_udpSocket) return;

    QByteArray hello;
    hello.append(char(0x80));
    hello.append(char(0x81));
    hello.append(char(0x7F));
    hello.append(char(200));
    hello.append(char(3));
    hello.append(char(56));
    hello.append(char(0));
    hello.append(char(0));
    hello.append(char(0x47));

    QString broadcastIP = moduleIP.isEmpty() ? "192.168.1.255" : moduleIP;
    sendToAllInterfaces(hello, broadcastIP, 8888);

    qDebug() << "Sent PGN 200 hello to" << broadcastIP << ":8888";
}

void AgIOService::sendScanRequest()
{
    if (!m_udpSocket) return;

    QByteArray scanRequest;
    scanRequest.append(char(0x80));
    scanRequest.append(char(0x81));
    scanRequest.append(char(0x7F));
    scanRequest.append(char(202));
    scanRequest.append(char(3));
    scanRequest.append(char(202));
    scanRequest.append(char(202));
    scanRequest.append(char(202));

    int checksum = 0;
    for (int i = 2; i < scanRequest.size(); i++) {
        checksum += (unsigned char)scanRequest[i];
    }
    scanRequest.append(char(checksum & 0xFF));

    QString broadcastIP = "192.168.1.255";
    sendToAllInterfaces(scanRequest, broadcastIP, 8888);

    qDebug() << "Sent PGN 202 scan request to" << broadcastIP << ":8888";
}

void AgIOService::processModuleResponse(const QByteArray& data)
{
    if (data.size() < 4) {
        return;
    }

    if (data[0] == char(0x80) && data[1] == char(0x82)) {
        QString moduleType = "Unknown";
        if (data.contains("AutoSteer")) {
            moduleType = "AutoSteer";
        } else if (data.contains("GPS")) {
            moduleType = "GPS";
        } else if (data.contains("IMU")) {
            moduleType = "IMU";
        } else if (data.contains("Section")) {
            moduleType = "Section";
        }

        QString senderIP = "192.168.1.100";

        // Qt 6.8 Rectangle Pattern: Get value, check/modify, set back
        QVariantList modules = m_discoveredModules.value();
        if (!modules.contains(senderIP)) {
            modules.append(senderIP);
            m_discoveredModules.setValue(modules);
            m_moduleLastSeen[senderIP] = QDateTime::currentDateTime();

            qDebug() << "Module discovered:" << senderIP << "Type:" << moduleType;
            emit moduleDiscovered(senderIP, moduleType);
        } else {
            m_moduleLastSeen[senderIP] = QDateTime::currentDateTime();
        }
    }
}

void AgIOService::handlePGNProtocol(const QByteArray& data)
{
    if (data.size() < 3) {
        return;
    }

    if (data[0] == char(0x80)) {
        quint8 pgnType = static_cast<quint8>(data[1]);
        quint8 sourceAddr = static_cast<quint8>(data[2]);

        qDebug() << "PGN received - Type:" << QString::number(pgnType, 16)
                 << "Source:" << QString::number(sourceAddr, 16);

        switch (pgnType) {
            case 0x81:
                qDebug() << "Hello message from" << QString::number(sourceAddr, 16);
                break;

            case 0x82:
                processModuleResponse(data);
                break;

            case 0x83:
                qDebug() << "GPS data from module" << QString::number(sourceAddr, 16);
                emit pgnDataReceived(data);
                incrementUdpCounters(data.size(), false);
                break;

            case 0x84:
                qDebug() << "IMU data from module" << QString::number(sourceAddr, 16);
                emit pgnDataReceived(data);
                break;

            case 0x85:
                qDebug() << "Section data from module" << QString::number(sourceAddr, 16);
                emit pgnDataReceived(data);
                break;

            default:
                emit pgnDataReceived(data);
                break;
        }
    }
}

void AgIOService::checkModuleTimeouts()
{
    if (!m_discoveryActive) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    QStringList timedOutModules;

    for (auto it = m_moduleLastSeen.begin(); it != m_moduleLastSeen.end(); ++it) {
        qint64 msecsElapsed = it.value().msecsTo(now);

        if (msecsElapsed > 30000) {
            timedOutModules.append(it.key());
            qDebug() << "Module timeout:" << it.key();
            emit moduleTimeout(it.key());
        }
    }

    // Qt 6.8 Rectangle Pattern: Get value, modify, set back
    if (!timedOutModules.isEmpty()) {
        QVariantList modules = m_discoveredModules.value();
        for (const QString& module : timedOutModules) {
            m_moduleLastSeen.remove(module);
            modules.removeAll(QVariant(module));
        }
        m_discoveredModules.setValue(modules);
    }
}

void AgIOService::validateModuleConnection()
{
    // Qt 6.8 Rectangle Pattern: Get value for iteration
    QVariantList modules = m_discoveredModules.value();
    for (const QVariant& moduleVar : modules) {
        sendHelloMessage(moduleVar.toString());
    }
}

void AgIOService::manageModuleTimeouts()
{
    checkModuleTimeouts();
}

// ========== Multi-Subnet Discovery ==========

void AgIOService::scanAllSubnets()
{
    if (!m_udpSocket || !m_isRunning) {
        qDebug() << "Cannot scan subnets - UDP not running";
        return;
    }

    // Phase 6.0.38: Initialize scan index UNCONDITIONALLY (prevents uninitialized memory access)
    m_currentSubnetIndex = 0;

    // Phase 6.0.38: Ensure subnet list is populated before scanning
    if (m_commonSubnets.isEmpty()) {
        qDebug() << "Subnet list empty - populating from network interfaces";
        updateSubnetList();
    }

    // Phase 6.0.38: No fallback - only scan if real subnets are detected
    if (m_commonSubnets.isEmpty()) {
        qWarning() << "CRITICAL: No active network interfaces detected - cannot scan";
        qWarning() << "Please check your network configuration and ensure at least one interface is UP";
        emit subnetScanCompleted("");
        return;
    }

    if (!m_subnetScanActive) {
        qDebug() << "Starting multi-subnet discovery scan";
        qDebug() << "Scanning subnets:" << m_commonSubnets;

        m_subnetScanActive = true;
        // Qt 6.8 Rectangle Pattern: setValue for QString properties
        m_discoveredModuleIP.setValue("");
        m_discoveredModuleSubnet.setValue("");

        wakeUpModules("255.255.255.255");
    }

    if (m_currentSubnetIndex >= m_commonSubnets.size()) {
        qDebug() << "Multi-subnet scan completed - no module found";
        m_subnetScanActive = false;
        emit subnetScanCompleted("");
        return;
    }

    // Phase 6.0.38: Extra safety check before array access to prevent crash
    if (m_currentSubnetIndex < 0 || m_currentSubnetIndex >= m_commonSubnets.size()) {
        qCritical() << "CRITICAL: Invalid subnet index" << m_currentSubnetIndex
                    << "for list size" << m_commonSubnets.size() << "- aborting scan";
        m_currentSubnetIndex = 0;
        m_subnetScanActive = false;
        return;
    }

    QString subnet = m_commonSubnets[m_currentSubnetIndex];
    QString broadcastIP = subnet + ".255";

    qDebug() << "Scanning subnet" << subnet << "(" << (m_currentSubnetIndex + 1)
             << "/" << m_commonSubnets.size() << ")";

    QByteArray helloMessage;
    helloMessage.append(char(0x80));
    helloMessage.append(char(0x81));
    helloMessage.append(char(0x7F));
    helloMessage.append(char(202));
    helloMessage.append(char(3));
    helloMessage.append(char(202));
    helloMessage.append(char(202));
    helloMessage.append(char(5));
    helloMessage.append(char(0x47));

    qDebug() << "Sending Hello to subnet" << subnet << "via all interfaces";
    sendToAllInterfaces(helloMessage, broadcastIP, 8888);

    if (m_subnetScanTimer) {
        m_subnetScanTimer->start();
    }
}

void AgIOService::sendPgnToDiscoveredSubnet(const QByteArray& pgnData)
{
    if (!m_udpSocket || !m_isRunning) {
        qDebug() << "Cannot send PGN - UDP not running";
        return;
    }

    // Qt 6.8 Rectangle Pattern: Use value() to check QString property
    if (m_discoveredModuleSubnet.value().isEmpty()) {
        qDebug() << "No subnet discovered yet - storing PGN data and starting scan";
        m_pendingPgnData = pgnData;
        scanAllSubnets();
        return;
    }

    QString broadcastIP = m_discoveredModuleSubnet + ".255";
    sendToAllInterfaces(pgnData, broadcastIP, 8888);

    qDebug() << "PGN data sent to discovered subnet" << m_discoveredModuleSubnet
             << "via all interfaces";

    m_packetsSent++;
    m_bytesSent += pgnData.size();
    updateSendRate();
    incrementUdpCounters(0, true);
}

void AgIOService::sendToAllInterfaces(const QByteArray& data, const QString& targetIP, int port)
{
    if (data.isEmpty()) {
        qDebug() << "Cannot send empty data";
        return;
    }

    static int sendCount = 0;
    bool shouldLog = (++sendCount % 10 == 1);

    if (shouldLog) {
        qDebug() << "MULTI-INTERFACE BROADCAST #" << sendCount << "- Target:" << targetIP
                 << ":" << port << "(" << data.size() << "bytes)";
    }

    int successCount = 0;
    int totalInterfaces = 0;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &iface, interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) ||
            iface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !iface.flags().testFlag(QNetworkInterface::CanBroadcast)) {
            continue;
        }

        foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                totalInterfaces++;

                QUdpSocket* ifaceSocket = new QUdpSocket();

                if (ifaceSocket->bind(entry.ip(), 0, QAbstractSocket::ShareAddress)) {

#ifdef Q_OS_WIN
                    ifaceSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
                    int dontRoute = 1;
                    if (setsockopt(ifaceSocket->socketDescriptor(), SOL_SOCKET, 0x0010,
                                   reinterpret_cast<const char*>(&dontRoute), sizeof(dontRoute)) != 0) {
                        if (shouldLog) {
                            qDebug() << "Could not set SO_DONTROUTE on interface" << iface.name();
                        }
                    }
#else
                    int dontRoute = 1;
                    if (setsockopt(ifaceSocket->socketDescriptor(), SOL_SOCKET, SO_DONTROUTE,
                                   &dontRoute, sizeof(dontRoute)) != 0) {
                        if (shouldLog) {
                            qDebug() << "Could not set SO_DONTROUTE on interface" << iface.name();
                        }
                    }
#endif

                    QHostAddress target;
                    if (targetIP == "255.255.255.255") {
                        target = QHostAddress::Broadcast;
                    } else {
                        target = QHostAddress(targetIP);
                    }

                    qint64 sent = ifaceSocket->writeDatagram(data, target, port);

                    if (sent > 0) {
                        successCount++;
                        if (shouldLog) {
                            qDebug() << "Interface" << iface.name()
                                     << "[" << entry.ip().toString() << "]"
                                     << "→" << targetIP << ":" << port
                                     << "(" << sent << "bytes)";
                        }
                    } else {
                        if (shouldLog) {
                            qDebug() << "Interface" << iface.name()
                                     << "[" << entry.ip().toString() << "]"
                                     << "failed:" << ifaceSocket->errorString();
                        }
                    }
                } else {
                    if (shouldLog) {
                        qDebug() << "Failed to bind to interface" << iface.name()
                                 << "[" << entry.ip().toString() << "]";
                    }
                }

                ifaceSocket->deleteLater();
            }
        }
    }

    if (successCount > 0) {
        m_packetsSent++;
        m_bytesSent += data.size();
        updateSendRate();
        incrementUdpCounters(0, true);

        if (shouldLog) {
            qDebug() << "Multi-interface broadcast complete:" << successCount
                     << "successful sends out of" << totalInterfaces << "interfaces";
        }
    } else {
        qDebug() << "Multi-interface broadcast FAILED - no successful sends";
    }
}

// ========== Network Interface Management ==========

QVariantList AgIOService::getNetworkInterfaces()
{
    QVariantList result;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &iface, interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {

                    QVariantMap ifaceInfo;
                    ifaceInfo["name"] = iface.name();
                    ifaceInfo["ip"] = entry.ip().toString();
                    ifaceInfo["subnet"] = getSubnetFromIP(entry.ip().toString());
                    ifaceInfo["canBroadcast"] = iface.flags().testFlag(QNetworkInterface::CanBroadcast);
                    ifaceInfo["isActive"] = true;
                    ifaceInfo["type"] = iface.type();

                    if (entry.netmask().protocol() == QAbstractSocket::IPv4Protocol) {
                        ifaceInfo["netmask"] = entry.netmask().toString();
                    }

                    result.append(ifaceInfo);
                }
            }
        }
    }

    return result;
}

QString AgIOService::getLocalSubnet()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &iface, interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return getSubnetFromIP(entry.ip().toString());
                }
            }
        }
    }

    return "192.168.1";
}

bool AgIOService::hasInterfaceOnSubnet(const QString& subnet)
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &iface, interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QString entrySubnet = getSubnetFromIP(entry.ip().toString());
                    if (entrySubnet == subnet) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

QString AgIOService::getSubnetFromIP(const QString& ip)
{
    QStringList parts = ip.split('.');
    if (parts.size() >= 3) {
        return parts[0] + "." + parts[1] + "." + parts[2];
    }
    return "192.168.1";
}

void AgIOService::updateSubnetList()
{
    qDebug() << "Updating subnet list from active network interfaces";

    m_commonSubnets.clear();
    QStringList detectedSubnets;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &iface, interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QString subnet = getSubnetFromIP(entry.ip().toString());

                    if (!detectedSubnets.contains(subnet)) {
                        detectedSubnets.append(subnet);
                        qDebug() << "  Found active subnet:" << subnet << "on interface"
                                 << iface.name() << "(" << entry.ip().toString() << ")";
                    }
                }
            }
        }
    }

    m_commonSubnets = detectedSubnets;

    // Phase 6.0.38: No fallback subnets - only use detected values
    if (m_commonSubnets.isEmpty()) {
        qWarning() << "No active subnets detected from network interfaces";
        qWarning() << "Subnet list will remain empty - scan will not proceed without real interfaces";
    } else {
        qDebug() << "Subnet scan list updated:" << m_commonSubnets.size()
                 << "active subnets detected:" << m_commonSubnets;
    }
}

// ========== Port Testing ==========

void AgIOService::testPortAccessibility(int port, const QString& description)
{
    qDebug() << "Testing" << description << "accessibility on port" << port;

    if (port == m_listenPort && m_udpSocket) {
        if (m_udpSocket->state() == QAbstractSocket::BoundState) {
            qDebug() << "Port" << port << "(" << description << ") is BOUND and accessible";
            qDebug() << "   - Socket state:" << m_udpSocket->state();
            qDebug() << "   - Local address:" << m_udpSocket->localAddress().toString();
            qDebug() << "   - Local port:" << m_udpSocket->localPort();
        } else {
            qDebug() << "Port" << port << "(" << description << ") is NOT properly bound";
            qDebug() << "   - Socket state:" << m_udpSocket->state();
        }
    } else {
        QByteArray testData = QByteArray::fromHex("TEST");
        qint64 sent = m_udpSocket->writeDatagram(testData, QHostAddress::Broadcast, port);

        if (sent > 0) {
            qDebug() << "Successfully sent" << sent << "test bytes to port" << port
                     << "(" << description << ")";
        } else {
            qDebug() << "Failed to send test data to port" << port << "(" << description << ")";
            qDebug() << "   - Error:" << m_udpSocket->errorString();
        }
    }
}

// ========== Module Status Monitoring ==========

void AgIOService::initializeModuleStatus()
{
    qDebug() << "Initializing module status monitoring for Phase 6.0.24";

    m_moduleStatus["IMU"] = false;
    m_moduleStatus["Steer"] = false;
    m_moduleStatus["Machine"] = false;

    qDebug() << "Module status initialized for:" << m_moduleStatus.keys();
}

void AgIOService::configureLocalAOGIP(const QString& localIP)
{
    qDebug() << "Configuring Local AOG IP:" << localIP;

    m_localAOGIP = localIP;

    QHostAddress testAddress(localIP);
    if (testAddress.isNull()) {
        qWarning() << "Invalid IP address format:" << localIP;
        m_localAOGIP = "127.0.0.1";
    }

    emit localAOGIPConfigured(m_localAOGIP);
    qDebug() << "Local AOG IP configured:" << m_localAOGIP;
}

void AgIOService::enableNMEAToUDPRelay(bool enable)
{
    qDebug() << "NMEA to UDP relay:" << (enable ? "enabled" : "disabled");

    m_nmeaToUDPRelayEnabled = enable;

    if (enable && m_nmeaRelayTimer && !m_nmeaRelayTimer->isActive()) {
        m_nmeaRelayTimer->start();
        qDebug() << "NMEA relay timer started";
    } else if (!enable && m_nmeaRelayTimer && m_nmeaRelayTimer->isActive()) {
        m_nmeaRelayTimer->stop();
        qDebug() << "NMEA relay timer stopped";
    }
}

void AgIOService::startModuleHeartbeatMonitoring()
{
    qDebug() << "Starting module heartbeat monitoring";

    m_heartbeatMonitoringActive = true;

    if (m_heartbeatMonitorTimer) {
        m_heartbeatMonitorTimer->start();
    }

    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_moduleStatus.begin(); it != m_moduleStatus.end(); ++it) {
        m_moduleHistory[it.key()] = now;
    }

    qDebug() << "Module heartbeat monitoring started for" << m_moduleStatus.size() << "modules";
}

void AgIOService::stopModuleHeartbeatMonitoring()
{
    qDebug() << "Stopping module heartbeat monitoring";

    m_heartbeatMonitoringActive = false;

    if (m_heartbeatMonitorTimer) {
        m_heartbeatMonitorTimer->stop();
    }

    for (auto it = m_moduleStatus.begin(); it != m_moduleStatus.end(); ++it) {
        if (it.value()) {
            updateModuleConnectionStatus(it.key(), false);
        }
    }

    qDebug() << "Module heartbeat monitoring stopped";
}

void AgIOService::updateModuleStatusSlot(const QString& moduleType, bool connected)
{
    if (!m_moduleStatus.contains(moduleType)) {
        qWarning() << "Unknown module type:" << moduleType;
        return;
    }

    bool statusChanged = (m_moduleStatus[moduleType] != connected);

    if (statusChanged) {
        qDebug() << "Module status change:" << moduleType
                 << (connected ? "connected" : "disconnected");

        updateModuleConnectionStatus(moduleType, connected);
        recordModuleHistory(moduleType, connected);
    }
}

void AgIOService::processModuleHeartbeat(const QByteArray& data)
{
    if (data.size() < 3) return;

    quint8 moduleId = static_cast<quint8>(data[0]);
    QString moduleType;

    switch (moduleId) {
    case 0x81: moduleType = "IMU"; break;
    case 0x82: moduleType = "Steer"; break;
    case 0x83: moduleType = "Machine"; break;
    default:
        qDebug() << "Unknown module heartbeat:" << QString::number(moduleId, 16);
        return;
    }

    m_moduleHistory[moduleType] = QDateTime::currentDateTime();

    if (!m_moduleStatus[moduleType]) {
        qDebug() << "Module reconnected:" << moduleType;
        updateModuleConnectionStatus(moduleType, true);
    }
}

void AgIOService::checkModuleHeartbeats()
{
    if (!m_heartbeatMonitoringActive) return;

    QDateTime now = QDateTime::currentDateTime();
    const int heartbeatTimeoutMs = 15000;

    for (auto it = m_moduleStatus.begin(); it != m_moduleStatus.end(); ++it) {
        const QString& moduleType = it.key();
        bool currentlyConnected = it.value();

        QDateTime lastSeen = m_moduleHistory.value(moduleType, QDateTime());
        qint64 msecsSinceLastSeen = lastSeen.msecsTo(now);

        if (currentlyConnected && msecsSinceLastSeen > heartbeatTimeoutMs) {
            qDebug() << "Module timeout:" << moduleType
                     << "(" << msecsSinceLastSeen << "ms since last heartbeat)";
            updateModuleConnectionStatus(moduleType, false);
        }
    }
}

void AgIOService::updateModuleConnectionStatus(const QString& moduleType, bool connected)
{
    m_moduleStatus[moduleType] = connected;

    if (moduleType == "IMU") {
        emit moduleIMUStatusChanged(connected);
    } else if (moduleType == "Steer") {
        emit moduleSteerStatusChanged(connected);
    } else if (moduleType == "Machine") {
        emit moduleMachineStatusChanged(connected);
    }

    qDebug() << moduleType << "module status:" << (connected ? "connected" : "disconnected");
}

void AgIOService::recordModuleHistory(const QString& moduleType, bool connected)
{
    if (connected) {
        m_moduleHistory[moduleType] = QDateTime::currentDateTime();
    }

    emit moduleHistoryUpdated(moduleType, connected);

    qDebug() << "Module history updated:" << moduleType << "was connected:" << connected;
}

// ========== Traffic Monitoring ==========

void AgIOService::doTraffic()
{
    if (!m_isUdpEnabled) {
        if (m_traffic) {
            m_traffic->setHelloFromMachine(99);
            m_traffic->setHelloFromBlockage(99);
            m_traffic->setHelloFromRateControl(99);
            m_traffic->setHelloFromAutoSteer(99);
            m_traffic->setHelloFromIMU(99);
            m_traffic->setCntrUDPOut(0);
            m_traffic->setCntrUDPIn(0);
            m_traffic->setCntrUDPInBytes(0);
            m_traffic->setUdpOutRate(0.0);
            m_traffic->setUdpInRate(0.0);
            m_traffic->setUdpOutFreq(0.0);
            m_traffic->setUdpInFreq(0.0);
        }
        return;
    }

    m_localCntrMachine++;
    m_localCntrBlockage++;
    m_localCntrRateControl++;
    m_localCntrSteer++;
    m_localCntrIMU++;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // ✅ Problem 9 Fix: INSTANTANEOUS rate calculation using delta (not cumulative average)
    // Calculate rate over the last 2 seconds (between doTraffic() calls), not since startup
    if (m_lastTrafficTime > 0) {
        qint64 elapsedMs = currentTime - m_lastTrafficTime;  // ~2000 ms (2 sec between calls)

        if (elapsedMs > 0) {
            // Delta bytes/packets since last doTraffic() call
            qint64 inBytesDelta = m_udpInBytes - m_udpInBytesLast;
            qint64 inPacketsDelta = m_udpInPackets - m_udpInPacketsLast;
            qint64 outBytesDelta = m_udpOutBytes - m_udpOutBytesLast;
            qint64 outPacketsDelta = m_udpOutPackets - m_udpOutPacketsLast;

            // Instantaneous rate over 2-second window (realistic 1-5 KB/s at 5-10 Hz for GPS)
            m_localUdpInRate = (inBytesDelta * 1000.0) / elapsedMs / 1024.0;    // KB/s
            m_localUdpInFreq = (inPacketsDelta * 1000.0) / elapsedMs;           // Hz
            m_localUdpOutRate = (outBytesDelta * 1000.0) / elapsedMs / 1024.0;  // KB/s
            m_localUdpOutFreq = (outPacketsDelta * 1000.0) / elapsedMs;         // Hz
        }
    }

    // Update "last" values for next doTraffic() cycle
    m_lastTrafficTime = currentTime;
    m_udpInBytesLast = m_udpInBytes;
    m_udpInPacketsLast = m_udpInPackets;
    m_udpOutBytesLast = m_udpOutBytes;
    m_udpOutPacketsLast = m_udpOutPackets;

    m_localCntrUDPOut = static_cast<quint32>(m_udpOutPackets);
    m_localCntrUDPIn = static_cast<quint32>(m_udpInPackets);
    m_localCntrUDPInBytes = static_cast<quint32>(m_udpInBytes);

    if (m_traffic) {
        m_traffic->setHelloFromMachine(m_localCntrMachine);
        m_traffic->setHelloFromBlockage(m_localCntrBlockage);
        m_traffic->setHelloFromRateControl(m_localCntrRateControl);
        m_traffic->setHelloFromAutoSteer(m_localCntrSteer);
        m_traffic->setHelloFromIMU(m_localCntrIMU);
        m_traffic->setCntrUDPOut(m_localCntrUDPOut);
        // ✅ Problem 13 Fix: UDP In stats now managed by onUdpDataReady() (1Hz precision)
        // These lines were overwriting correct values with stale 0 Hz every 200ms!
        // m_traffic->setCntrUDPIn(m_localCntrUDPIn);
        // m_traffic->setCntrUDPInBytes(m_localCntrUDPInBytes);
        m_traffic->setUdpOutRate(m_localUdpOutRate);
        // m_traffic->setUdpInRate(m_localUdpInRate);
        m_traffic->setUdpOutFreq(m_localUdpOutFreq);
        // m_traffic->setUdpInFreq(m_localUdpInFreq);
    }

    bool machineConnected = (m_localCntrMachine < 3);
    bool blockageConnected = (m_localCntrBlockage < 10);
    bool rateControlConnected = (m_localCntrRateControl < 10);
    bool steerConnected = (m_localCntrSteer < 3);
    bool imuConnected = (m_localCntrIMU < 3);

    static bool lastMachineConnected = false;
    static bool lastBlockageConnected = false;
    static bool lastRateControlConnected = false;
    static bool lastSteerConnected = false;
    static bool lastIMUConnected = false;

    if (machineConnected != lastMachineConnected) {
        //emit moduleMachineStatusChanged(machineConnected);
        setMachineConnected(machineConnected);
        lastMachineConnected = machineConnected;
        qCDebug(agioservice) << "Machine module:" << (machineConnected ? "CONNECTED" : "DISCONNECTED");
    }
    if (blockageConnected != lastBlockageConnected) {
        //emit moduleMachineStatusChanged(machineConnected);
        setBlockageConnected(blockageConnected);
        lastBlockageConnected = blockageConnected;
        qCDebug(agioservice) << "Blockage module:" << (blockageConnected ? "CONNECTED" : "DISCONNECTED");
    }
    if (rateControlConnected != lastRateControlConnected) {
        //emit moduleMachineStatusChanged(machineConnected);
        setRateControlConnected(rateControlConnected);
        lastRateControlConnected = rateControlConnected;
        qCDebug(agioservice) << "RateControl module:" << (rateControlConnected ? "CONNECTED" : "DISCONNECTED");
    }
    if (steerConnected != lastSteerConnected) {
        emit moduleSteerStatusChanged(steerConnected);
        lastSteerConnected = steerConnected;
        qCDebug(agioservice) << "Steer module:" << (steerConnected ? "CONNECTED" : "DISCONNECTED");
    }
    if (imuConnected != lastIMUConnected) {
        emit moduleIMUStatusChanged(imuConnected);
        lastIMUConnected = imuConnected;
        qCDebug(agioservice) << "IMU module:" << (imuConnected ? "CONNECTED" : "DISCONNECTED");
    }

    static int debugCounter = 0;
    if (++debugCounter >= 5) {
        qCDebug(agioservice) << "Traffic counters - Machine:" << m_localCntrMachine
                 << "AutoSteer:" << m_localCntrSteer
                 << "IMU:" << m_localCntrIMU;
        debugCounter = 0;
    }
}

void AgIOService::processHelloMessage(const QByteArray& data)
{
    if (data.size() == 13 && static_cast<quint8>(data[0]) == 0x80 &&
        static_cast<quint8>(data[1]) == 0x81 && static_cast<quint8>(data[3]) == 203) {

        quint8 moduleId = static_cast<quint8>(data[2]);

        qCDebug(agioservice) << "PGN 203 scan reply from module:" << QString::number(moduleId, 16);

        switch (moduleId) {
        case 123:
            if (m_localCntrMachine != 0) {
                m_localCntrMachine = 0;
                qCDebug(agioservice) << "Machine module (123) scan reply - counter reset";
            }
            break;

        case 124:
            if (m_localCntrBlockage != 0) {
                m_localCntrBlockage = 0;
                qCDebug(agioservice) << "Blockage module (124) scan reply - counter reset";
            }
            break;

        case 126:
            if (m_localCntrSteer != 0) {
                m_localCntrSteer = 0;
                qCDebug(agioservice) << "AutoSteer module (126) scan reply - counter reset";
            }
            break;

        case 121:
            if (m_localCntrIMU != 0) {
                m_localCntrIMU = 0;
                qCDebug(agioservice) << "IMU scan reply - counter reset";
            }
            break;

        case 122:
            if (m_localCntrRateControl != 0) {
                m_localCntrRateControl = 0;
                qCDebug(agioservice) << "RC scan reply - counter reset";
            }
            break;

        default:
            return;
        }
        return;
    }

    if (data.size() == 1) {
        quint8 moduleId = static_cast<quint8>(data[0]);

        switch (moduleId) {
        case 123:
            if (m_localCntrMachine != 0) {
                m_localCntrMachine = 0;
                qDebug() << "Machine hello received - counter reset";
            }
            break;

        case 122:
            if (m_localCntrRateControl != 0) {
                m_localCntrRateControl = 0;
                qDebug() << "RateControl hello received - counter reset";
            }
            break;

        case 124:
            if (m_localCntrBlockage != 0) {
                m_localCntrBlockage = 0;
                qDebug() << "Blockage hello received - counter reset";
            }
            break;

        case 126:
            if (m_localCntrSteer != 0) {
                m_localCntrSteer = 0;
                qDebug() << "AutoSteer hello received - counter reset";
            }
            break;

        case 121:
            if (m_localCntrIMU != 0) {
                m_localCntrIMU = 0;
                qDebug() << "IMU hello received - counter reset";
            }
            break;

        default:
            return;
        }
    }
}

void AgIOService::incrementUdpCounters(int bytesIn, bool isOut)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (isOut) {
        m_udpOutBytes += bytesIn > 0 ? bytesIn : 14;
        m_udpOutPackets++;

        if (m_udpOutStartTime == 0) {
            m_udpOutStartTime = currentTime;
        }
        m_udpOutLastTime = currentTime;
    }
    else if (bytesIn > 0) {
        m_udpInBytes += bytesIn;
        m_udpInPackets++;

        if (m_udpInStartTime == 0) {
            m_udpInStartTime = currentTime;
        }
        m_udpInLastTime = currentTime;
    }
}

void AgIOService::resetUdpTrafficCounters()
{
    qDebug() << "Phase 6.0.24: Resetting ALL UDP traffic counters to zero";

    m_udpOutBytes = 0;
    m_udpInBytes = 0;
    m_udpOutStartTime = 0;
    m_udpInStartTime = 0;
    m_udpOutLastTime = 0;
    m_udpInLastTime = 0;
    m_udpOutPackets = 0;
    m_udpInPackets = 0;

    if (m_traffic) {
        m_traffic->setCntrUDPOut(0);
        m_traffic->setCntrUDPIn(0);
        m_traffic->setCntrUDPInBytes(0);
        m_traffic->setUdpOutRate(0.0);
        m_traffic->setUdpInRate(0.0);
        m_traffic->setUdpOutFreq(0.0);
        m_traffic->setUdpInFreq(0.0);
    }

    qDebug() << "UDP traffic counters reset complete";
}

void AgIOService::checkSubnetMismatch(const QString& moduleSubnet, const QString& moduleIP)
{
    QString pcSubnet = getCurrentPCSubnet();

    if (pcSubnet.isEmpty()) {
        qDebug() << "Could not determine PC subnet";
        return;
    }

    qDebug() << "Subnet comparison - PC:" << pcSubnet << "Module:" << moduleSubnet;

    if (pcSubnet != moduleSubnet) {
        qWarning() << "SUBNET MISMATCH DETECTED!";
        qWarning() << "  Your PC is on subnet:" << pcSubnet << ".x";
        qWarning() << "  Module is on subnet:" << moduleSubnet << ".x (" << moduleIP << ")";
        qWarning() << "  Suggestion: Use 'Set IP' to change module to" << pcSubnet << ".126";

        emit subnetMismatchDetected(pcSubnet, moduleSubnet, moduleIP);
    } else {
        qDebug() << "Subnet match - PC and module both on" << pcSubnet << ".x";
    }
}

QString AgIOService::getCurrentPCSubnet()
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QString fallbackSubnet;

    for (const QNetworkInterface& netInterface : interfaces) {
        if (netInterface.flags().testFlag(QNetworkInterface::IsUp) &&
            netInterface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !netInterface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            const QList<QNetworkAddressEntry> addresses = netInterface.addressEntries();

            for (const QNetworkAddressEntry& addressEntry : addresses) {
                if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QString ip = addressEntry.ip().toString();
                    QStringList parts = ip.split('.');

                    if (parts.size() >= 3) {
                        QString subnet = parts[0] + "." + parts[1] + "." + parts[2];

                        if (subnet.startsWith("169.254") || subnet.startsWith("127.0")) {
                            continue;
                        }

                        if (subnet.startsWith("192.168")) {
                            qDebug() << "PC interface found (priority):" << netInterface.name()
                                     << "IP:" << ip << "Subnet:" << subnet;
                            return subnet;
                        }

                        if (fallbackSubnet.isEmpty()) {
                            fallbackSubnet = subnet;
                            qDebug() << "PC interface found (fallback):" << netInterface.name()
                                     << "IP:" << ip << "Subnet:" << subnet;
                        }
                    }
                }
            }
        }
    }

    if (!fallbackSubnet.isEmpty()) {
        qDebug() << "Using fallback subnet:" << fallbackSubnet;
        return fallbackSubnet;
    }

    return QString();
}

bool AgIOService::isModuleConnected(const QString& moduleType) const
{
    if (!m_traffic) {
        return false;
    }

    if (moduleType.compare("Machine", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromMachine() < 3;
    }
    else if (moduleType.compare("AutoSteer", Qt::CaseInsensitive) == 0 ||
             moduleType.compare("Steer", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromAutoSteer() < 3;
    }
    else if (moduleType.compare("IMU", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromIMU() < 3;
    }

    return false;
}

quint32 AgIOService::getHelloCount(const QString& moduleType) const
{
    if (!m_traffic) {
        return 99;
    }

    if (moduleType.compare("Machine", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromMachine();
    }
    else if (moduleType.compare("AutoSteer", Qt::CaseInsensitive) == 0 ||
             moduleType.compare("Steer", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromAutoSteer();
    }
    else if (moduleType.compare("IMU", Qt::CaseInsensitive) == 0) {
        return m_traffic->helloFromIMU();
    }

    return 99;
}

void AgIOService::resetTrafficCounters()
{
    if (!m_traffic) {
        return;
    }

    m_traffic->setHelloFromMachine(99);
    m_traffic->setHelloFromAutoSteer(99);
    m_traffic->setHelloFromIMU(99);
    m_traffic->setCntrUDPOut(0);
    m_traffic->setCntrUDPIn(0);
    m_traffic->setCntrUDPInBytes(0);

    emitTrafficChangedThrottled(true);
    qDebug() << "Traffic counters reset to default values";
}

void AgIOService::emitTrafficChangedThrottled(bool force)
{
    static qint64 lastEmitTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (force || (currentTime - lastEmitTime) >= 500) {
        emit moduleDiscoveryChanged();
        lastEmitTime = currentTime;
    }
}

// ========== Traffic Suspension ==========

void AgIOService::pauseAllUDPTraffic()
{
    qDebug() << "AgIOService: Pausing ALL UDP traffic (RX+TX) for Set IP command";

    m_udpTrafficPaused = true;

    stopModuleHeartbeatMonitoring();
    stopModuleDiscovery();

    if (m_discoveryTimer) {
        m_discoveryTimer->stop();
        qDebug() << "Discovery timer STOPPED for Set IP";
    }

    if (m_udpHeartbeatTimer) {
        m_udpHeartbeatTimer->stop();
        qDebug() << "Heartbeat timer STOPPED for Set IP";
    }

    if (m_trafficTimer) {
        m_trafficTimer->stop();
        qDebug() << "Traffic monitoring timer STOPPED for Set IP";
    }

    qDebug() << "All UDP traffic (reception + transmission) paused for clean Set IP";
}

void AgIOService::resumeAllUDPTraffic()
{
    qDebug() << "AgIOService: Resuming ALL UDP traffic (RX+TX) after Set IP command";

    m_udpTrafficPaused = false;

    startModuleHeartbeatMonitoring();
    startModuleDiscovery();

    if (m_discoveryTimer && !m_discoveryTimer->isActive()) {
        m_discoveryTimer->start();
        qDebug() << "Discovery timer RESTARTED";
    }

    if (m_udpHeartbeatTimer && !m_udpHeartbeatTimer->isActive()) {
        m_udpHeartbeatTimer->start();
        qDebug() << "Heartbeat timer RESTARTED";
    }

    if (m_trafficTimer && !m_trafficTimer->isActive()) {
        m_trafficTimer->start();
        qDebug() << "Traffic monitoring timer RESTARTED";
    }

    qDebug() << "All UDP traffic (reception + transmission) resumed after Set IP";
}

// ========================================
// END OF AGIOSERVICE_UDP.CPP - PHASE 6.0.24 COMPLETE
// ========================================
// Total functions: 59 (ALL migrated from UDPWorker)
// Architecture: Event-driven main thread (Qt 6.8 BINDABLE compliance)
// File size: ~1500 lines
// Migration: 100% COMPLETE - ZERO omissions
