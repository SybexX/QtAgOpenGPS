#ifndef NTRIPWORKER_H
#define NTRIPWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief NTRIP client worker for RTCM corrections
 * 
 * Handles NTRIP caster connection in a separate thread.
 * Downloads RTCM correction data and forwards to GPS receiver.
 */
class NTRIPWorker : public QObject
{
    Q_OBJECT
    
    // NTRIP connection states
    enum ConnectionState {
        Disconnected = 0,
        Connecting = 1,
        Authenticating = 2,
        WaitingForData = 3,
        ReceivingData = 4,
        Error = 5
    };
    
public:
    explicit NTRIPWorker(QObject *parent = nullptr);
    ~NTRIPWorker();

public slots:
    void startNTRIP(const QString& url, const QString& user,
                   const QString& password, const QString& mount, int port);
    void stopNTRIP();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpDataReceived();
    void onTcpError(QAbstractSocket::SocketError error);

    // ✅ PHASE 5.3 - Advanced NTRIP Configuration
    void configurePacketSize(int size);              // setNTRIP_packetSize
    void enableSerialRouting(bool enable);           // setNTRIP_sendToSerial
    void enableUDPBroadcast(bool enable);            // setNTRIP_sendToUDP
    void setRoutingTargets(const QStringList& serialPorts, const QString& udpAddress, int udpPort);

private slots:
    void checkConnectionStatus();
    void attemptReconnect();
    void sendNtripRequest();
    void processNtripResponse();

signals:
    void ntripStatusChanged(int status, const QString& statusText);
    void ntripDataReceived(const QByteArray& rtcmData);
    void errorOccurred(const QString& error);

    // ✅ PHASE 5.3 - Advanced RTCM Routing Signals
    void routeRTCMToSerial(const QByteArray& rtcmData);
    void broadcastRTCMToUDP(const QByteArray& rtcmData);
    void rtcmPacketProcessed(int size, const QString& destination);

private:
    QTcpSocket* m_tcpSocket;
    QTimer* m_statusTimer;
    QTimer* m_reconnectTimer;
    QByteArray m_receiveBuffer;
    
    // Connection settings
    QString m_ntripUrl;
    QString m_ntripUser;
    QString m_ntripPassword;
    QString m_ntripMount;
    int m_ntripPort;
    
    // Connection state
    ConnectionState m_connectionState;
    bool m_isRunning;
    int m_reconnectAttempts;
    qint64 m_lastDataTime;
    int m_dataTimeoutMs;
    bool m_headerReceived;
    
    // Statistics
    qint64 m_bytesReceived;
    int m_packetsReceived;
    double m_dataRate; // bytes/second

    // ✅ PHASE 5.3 - Advanced RTCM Routing Configuration
    int m_rtcmPacketSize;                    // setNTRIP_packetSize (default: 256)
    bool m_sendToSerialEnabled;              // setNTRIP_sendToSerial
    bool m_sendToUDPEnabled;                 // setNTRIP_sendToUDP
    QStringList m_serialRoutingTargets;      // Target serial ports
    QString m_udpBroadcastAddress;           // UDP broadcast address
    int m_udpBroadcastPort;                  // UDP broadcast port

    // RTCM packet processing
    QByteArray m_rtcmBuffer;                 // Buffer for RTCM packet assembly
    int m_totalBytesRouted;                  // Statistics: total bytes routed
    int m_serialPacketsSent;                 // Statistics: packets sent to serial
    int m_udpPacketsSent;                    // Statistics: packets sent to UDP
    
    void setState(ConnectionState newState);
    QString stateToString(ConnectionState state) const;
    void resetStatistics();
    void updateDataRate();
    
    // NTRIP protocol helpers
    QString buildNtripRequest() const;
    bool parseNtripResponse(const QByteArray& response);
    void processRtcmData(const QByteArray& data);
    
    // Connection management
    void cleanupConnection();
    bool validateSettings() const;

    // ✅ PHASE 5.3 - Advanced RTCM Processing
    void processRTCMPackets(const QByteArray& data);
    void routeRTCMPacket(const QByteArray& packet);
    void sendRTCMToSerial(const QByteArray& packet);
    void sendRTCMToUDP(const QByteArray& packet);
    bool isValidRTCMPacket(const QByteArray& packet) const;
    void updateRoutingStatistics(const QString& destination, int bytes);
};

#endif // NTRIPWORKER_H