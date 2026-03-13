// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// SerialWorker - Multi-port serial communication worker for AgIOService
// Handles GPS, IMU, and AutoSteer serial communication on dedicated thread

#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QMap>

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    // Port management - Legacy 3 ports (maintained for compatibility)
    Q_INVOKABLE bool openGPSPort(const QString& portName, int baudRate);
    Q_INVOKABLE bool openIMUPort(const QString& portName, int baudRate);
    Q_INVOKABLE bool openAutoSteerPort(const QString& portName, int baudRate);

    Q_INVOKABLE void closeGPSPort();
    Q_INVOKABLE void closeIMUPort();
    Q_INVOKABLE void closeAutoSteerPort();

    // Extended port management - Dynamic 8-port architecture
    Q_INVOKABLE bool openGPS2Port(const QString& portName, int baudRate);
    Q_INVOKABLE bool openMachinePort(const QString& portName, int baudRate);
    Q_INVOKABLE bool openRadioPort(const QString& portName, int baudRate);
    Q_INVOKABLE bool openRTCMPort(const QString& portName, int baudRate);
    Q_INVOKABLE bool openToolPort(const QString& portName, int baudRate);

    Q_INVOKABLE void closeGPS2Port();
    Q_INVOKABLE void closeMachinePort();
    Q_INVOKABLE void closeRadioPort();
    Q_INVOKABLE void closeRTCMPort();
    Q_INVOKABLE void closeToolPort();
    Q_INVOKABLE void closeAllPorts();

    // Generic port management
    Q_INVOKABLE bool openPort(const QString& portType, const QString& portName, int baudRate);
    Q_INVOKABLE void closePort(const QString& portType);
    Q_INVOKABLE QStringList getActivePortTypes();
    
    Q_INVOKABLE QStringList scanAvailablePorts();
    Q_INVOKABLE bool isPortAvailable(const QString& portName);
    
    // Data transmission - Legacy 3 ports
    Q_INVOKABLE void writeToGPS(const QByteArray& data);
    Q_INVOKABLE void writeToIMU(const QByteArray& data);
    Q_INVOKABLE void writeToAutoSteer(const QByteArray& data);

    // Extended data transmission - 5 new ports
    Q_INVOKABLE void writeToGPS2(const QByteArray& data);
    Q_INVOKABLE void writeToMachine(const QByteArray& data);
    Q_INVOKABLE void writeToRadio(const QByteArray& data);
    Q_INVOKABLE void writeToRTCM(const QByteArray& data);
    Q_INVOKABLE void writeToTool(const QByteArray& data);

    // Generic write method
    Q_INVOKABLE void writeToPort(const QString& portType, const QByteArray& data);
    
    // Status queries - Legacy 3 ports
    Q_INVOKABLE bool isGPSConnected() const { return m_gpsConnected; }
    Q_INVOKABLE bool isIMUConnected() const { return m_imuConnected; }
    Q_INVOKABLE bool isAutoSteerConnected() const { return m_autosteerConnected; }

    // Extended status queries - 5 new ports
    Q_INVOKABLE bool isGPS2Connected() const { return m_portConnected.value("GPS2", false); }
    Q_INVOKABLE bool isMachineConnected() const { return m_portConnected.value("Machine", false); }
    Q_INVOKABLE bool isRadioConnected() const { return m_portConnected.value("Radio", false); }
    Q_INVOKABLE bool isRTCMConnected() const { return m_portConnected.value("RTCM", false); }
    Q_INVOKABLE bool isToolConnected() const { return m_portConnected.value("Tool", false); }

    // Port name queries - Legacy
    Q_INVOKABLE QString getGPSPortName() const { return m_gpsPortName; }
    Q_INVOKABLE QString getIMUPortName() const { return m_imuPortName; }
    Q_INVOKABLE QString getAutoSteerPortName() const { return m_autosteerPortName; }

    // Extended port name queries
    Q_INVOKABLE QString getGPS2PortName() const { return m_portNames.value("GPS2", ""); }
    Q_INVOKABLE QString getMachinePortName() const { return m_portNames.value("Machine", ""); }
    Q_INVOKABLE QString getRadioPortName() const { return m_portNames.value("Radio", ""); }
    Q_INVOKABLE QString getRTCMPortName() const { return m_portNames.value("RTCM", ""); }
    Q_INVOKABLE QString getToolPortName() const { return m_portNames.value("Tool", ""); }

    // Generic status queries
    Q_INVOKABLE bool isPortConnected(const QString& portType) const;
    Q_INVOKABLE QString getPortName(const QString& portType) const;
    Q_INVOKABLE int getPortBaudRate(const QString& portType) const;

public slots:
    // Thread control
    void startWorker();
    void stopWorker();
    
    // Configuration - Legacy 3 ports
    void configureGPSPort(const QString& portName, int baudRate);
    void configureIMUPort(const QString& portName, int baudRate);
    void configureAutoSteerPort(const QString& portName, int baudRate);

    // Extended configuration - 5 new ports
    void configureGPS2Port(const QString& portName, int baudRate);
    void configureMachinePort(const QString& portName, int baudRate);
    void configureRadioPort(const QString& portName, int baudRate);
    void configureRTCMPort(const QString& portName, int baudRate);
    void configureToolPort(const QString& portName, int baudRate);

    // Generic configuration
    void configurePort(const QString& portType, const QString& portName, int baudRate);

    // ✅ CDC Compliance - Async methods (replaces BlockingQueuedConnection)
    void openGPSPortAsync(const QString& portName, int baudRate);
    void openIMUPortAsync(const QString& portName, int baudRate);
    void openAutoSteerPortAsync(const QString& portName, int baudRate);
    void openMachinePortAsync(const QString& portName, int baudRate);
    void openToolPortAsync(const QString& portName, int baudRate);
    void openPortAsync(const QString& portType, const QString& portName, int baudRate);

    void checkConnectionStatusAsync(const QString& portType);
    void getPortNameAsync(const QString& portType);
    void getPortBaudRateAsync(const QString& portType);
    void scanAvailablePortsAsync();

signals:
    // Data reception signals - Legacy 3 ports
    void gpsDataReceived(const QString& nmea);
    void imuDataReceived(const QByteArray& imuData);
    void autosteerResponseReceived(const QByteArray& response);

    // Extended data reception signals - 5 new ports
    void gps2DataReceived(const QString& nmea);
    void machineDataReceived(const QByteArray& machineData);
    void radioDataReceived(const QByteArray& radioData);
    void rtcmDataReceived(const QByteArray& rtcmData);
    void toolDataReceived(const QByteArray& toolData);

    // Generic data reception signal
    void portDataReceived(const QString& portType, const QByteArray& data);

    // PHASE 6.0.22.6: Unified data signal (matches UDPWorker pattern)
    void dataReceived(const QByteArray& data, const QString& source);

    // Raw data for debugging
    void serialDataReceived(const QString& portName, const QByteArray& data);
    
    // Connection status signals - Legacy 3 ports
    void gpsConnected(bool connected);
    void imuConnected(bool connected);
    void autosteerConnected(bool connected);

    // Extended connection status signals - 5 new ports
    void gps2Connected(bool connected);
    void machineConnected(bool connected);
    void radioConnected(bool connected);
    void rtcmConnected(bool connected);
    void toolConnected(bool connected);

    // Generic connection status signal
    void portConnected(const QString& portType, bool connected);
    
    // Error signals
    void serialError(const QString& portName, const QString& error);
    void portOpenError(const QString& portName, const QString& error);
    
    // Status updates
    void workerStarted();
    void workerStopped();

    // ✅ CDC Compliance - Async operation results (replaces BlockingQueuedConnection)
    void portOpenResult(const QString& portType, bool success, const QString& errorMessage);
    void portCloseResult(const QString& portType, bool success);
    void connectionStatusResult(const QString& portType, bool isConnected);
    void portNameResult(const QString& portType, const QString& portName);
    void portBaudRateResult(const QString& portType, int baudRate);
    void availablePortsResult(const QStringList& ports);

private slots:
    // Data reception handlers - Legacy 3 ports
    void onGPSDataReady();
    void onIMUDataReady();
    void onAutoSteerDataReady();

    // Extended data reception handlers - 5 new ports
    void onGPS2DataReady();
    void onMachineDataReady();
    void onRadioDataReady();
    void onRTCMDataReady();
    void onToolDataReady();

    // Generic data reception handler
    void onPortDataReady();

    // Error handlers - Legacy 3 ports
    void onGPSError(QSerialPort::SerialPortError error);
    void onIMUError(QSerialPort::SerialPortError error);
    void onAutoSteerError(QSerialPort::SerialPortError error);

    // Generic error handler
    void onPortError(QSerialPort::SerialPortError error);
    
    // Connection monitoring
    void checkConnections();

private:
    // Serial ports - Legacy 3 ports (maintained for compatibility)
    QSerialPort* m_gpsPort;
    QSerialPort* m_imuPort;
    QSerialPort* m_autosteerPort;

    // Data buffers - Legacy 3 ports
    QByteArray m_gpsBuffer;
    QByteArray m_imuBuffer;
    QByteArray m_autosteerBuffer;

    // Connection status - Legacy 3 ports
    bool m_gpsConnected;
    bool m_imuConnected;
    bool m_autosteerConnected;

    // Port configurations - Legacy 3 ports
    QString m_gpsPortName;
    QString m_imuPortName;
    QString m_autosteerPortName;
    int m_gpsBaudRate;
    int m_imuBaudRate;
    int m_autosteerBaudRate;

    // ✅ PHASE 5.2 - Dynamic 8-port architecture (QMap-based)
    QMap<QString, QSerialPort*> m_serialPorts;    // Dynamic ports: GPS2, Machine, Radio, RTCM, Tool
    QMap<QString, QByteArray> m_portBuffers;      // Buffers per port
    QMap<QString, bool> m_portConnected;          // Connection status per port
    QMap<QString, QString> m_portNames;           // Port names configuration
    QMap<QString, int> m_baudRates;               // Baud rates per port

    // Port type constants for 5 new ports
    static const QString PORT_GPS2;
    static const QString PORT_MACHINE;
    static const QString PORT_RADIO;
    static const QString PORT_RTCM;
    static const QString PORT_TOOL;
    
    // Monitoring
    QTimer* m_connectionTimer;
    QMutex m_mutex;
    
    // Worker state
    bool m_isRunning;
    
    // Helper methods - Legacy support
    bool openPort(QSerialPort*& port, const QString& portName, int baudRate, const QString& description);
    void closePort(QSerialPort*& port, const QString& description);
    void processGPSBuffer();
    void processIMUBuffer();
    void processAutoSteerBuffer();

    // Extended helper methods - Dynamic port support
    void initializeDynamicPorts();
    bool openDynamicPort(const QString& portType, const QString& portName, int baudRate);
    void closeDynamicPort(const QString& portType);
    void processPortBuffer(const QString& portType);
    void setupPortConnections(const QString& portType, QSerialPort* port);
    void handleDynamicPortData(const QString& portType);
    void emitSpecificConnectionSignal(const QString& portType, bool connected);
    QString getErrorString(QSerialPort::SerialPortError error);

    // Error handling
    void handleSerialError(QSerialPort* port, const QString& portName, QSerialPort::SerialPortError error);
    void handleDynamicPortError(const QString& portType, QSerialPort::SerialPortError error);
    
    // Data parsing
    QStringList extractNMEASentences(QByteArray& buffer);
    bool isValidNMEASentence(const QString& sentence);
    QString calculateNMEAChecksum(const QString& sentence);
};

#endif // SERIALWORKER_H