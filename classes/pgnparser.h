// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// PGNParser - Centralized NMEA and PGN binary parser

#ifndef PGNPARSER_H
#define PGNPARSER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QStringList>

/**
 * @brief Centralized PGN & NMEA parser
 *
 * Single source of truth for ALL parsing:
 * - NMEA text: $PANDA, $GPGGA, $GPRMC, $GPVTG, $GPGSA, $GPGSV (Serial + UDP)
 * - PGN binary: 0x80 0x81 ... (UDP only)
 *
 * Replaces duplicated parsing in:
 * - GPSWorker (deleted in Phase 6.0.21)
 * - AgIOService::parseNmeaGpsData() (removed in Phase 6.0.21)
 * - UDPWorker::processGpsPgn() (removed in Phase 6.0.21)
 */
class PGNParser : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Parsed data structure (unified for NMEA + PGN)
     */
    struct ParsedData {
        // GPS data
        double latitude = 0.0;
        double longitude = 0.0;
        double altitude = 0.0;
        double speed = 0.0;           // km/h
        double heading = 0.0;         // degrees (single antenna / COG)
        double headingDual = 0.0;     // degrees (dual antenna)
        double headingHDT = 0.0;      // degrees (HDT/AVR/KSXT true heading)
        int quality = 0;              // Fix quality (0-9)
        int satellites = 0;
        double hdop = 0.0;
        double age = 0.0;             // Age of differential (seconds)

        // IMU data (from $PANDA field 12-15 OR PGN 129)
        // Reference: AIO firmware zHandlers.ino BuildNmea() lines 320-333
        double imuHeading = 0.0;      // degrees (PANDA field 12 - heading/yaw)
        double imuRoll = 0.0;         // degrees (PANDA field 13 - roll)
        double imuPitch = 0.0;        // degrees (PANDA field 14 - pitch)
        double yawRate = 0.0;         // degrees/sec (PANDA field 15 - yaw rate x10 integer)
        bool hasIMU = false;          // Flag: IMU data present

        // WAS data (Wheel Angle Sensor - PGN 126 ONLY)
        int wasValue = 0;
        bool hasWAS = false;

        // PHASE 6.0.23: AutoSteer control data (PGN 253/250)
        int16_t steerAngleActual = 0; // PGN 253 byte 5-6 (int16 x100 degrees)
        uint8_t switchByte = 0;       // PGN 253 byte 11 (bit 0=work, bit 1=steer)
        uint8_t pwmDisplay = 0;       // PGN 253 byte 12 (0-255 motor drive)
        uint8_t sensorValue = 0;      // PGN 250 byte 5 (pressure/current 0-255)
        bool hasSteerData = false;    // Flag: AutoSteer data present

        // Metadata
        QString sourceType;           // "NMEA" or "PGN"
        QString sentenceType;         // "GGA", "RMC", "PANDA", etc.
        int pgnNumber = 0;            // If PGN binary (127, 128, 129, 203, etc.)
        bool isValid = false;

        // PHASE 6.0.22.1: Source tracking and module identification
        QString sourceTransport;      // "UDP" or "Serial"
        QString sourceID;             // IP address (UDP) or COM port name (Serial)
        QString moduleType;           // "GPS", "IMU", "Steer", "Machine", "Unknown"
        qint64 timestampMs = 0;       // Reception timestamp (msecs since epoch)

        // Blockage
        int blockagesection[4] = {0, 0, 0, 0};
        int rateControlInData[5] = {0, 0, 0, 0, 0};
    };

    explicit PGNParser(QObject *parent = nullptr);

    // Main parsers

    /**
     * @brief Auto-detect format and parse data (NMEA text or PGN binary)
     * @param data Raw data from UDP/Serial (NMEA starts with '$', PGN starts with 0x80 0x81)
     * @return ParsedData with GPS/IMU/WAS data
     *
     * Phase 6.0.21.1: Unified entry point for ALL parsing
     * - Detects NMEA text (starts with '$')
     * - Detects PGN binary (starts with 0x80 0x81)
     * - Routes to appropriate parser
     */
    ParsedData parse(const QByteArray& data);

    /**
     * @brief Parse NMEA text sentence (auto-detects type)
     * @param sentence NMEA sentence (e.g., "$GPGGA,...")
     * @return ParsedData with GPS/IMU data
     */
    ParsedData parseNMEA(const QString& sentence);

    /**
     * @brief Parse PGN binary data (auto-detects PGN number)
     * @param binaryData PGN binary (e.g., 0x80 0x81 0x80 ...)
     * @return ParsedData with GPS/IMU/WAS data
     */
    ParsedData parsePGN(const QByteArray& binaryData);

    // Validation

    static bool isValidNMEA(const QString& sentence);
    static bool isValidPGN(const QByteArray& data);
    static bool validateChecksum(const QString& sentence);

    // Utilities

    static QString extractSentenceType(const QString& sentence);
    static int extractPGNNumber(const QByteArray& data);
    static QStringList splitNMEA(const QString& sentence);

private:
    // NMEA specific parsers

    ParsedData parseGGA(const QString& sentence);   // GPS position
    ParsedData parseRMC(const QString& sentence);   // GPS + speed + heading
    ParsedData parseVTG(const QString& sentence);   // Speed over ground
    ParsedData parsePANDA(const QString& sentence); // AgOpenGPS single antenna + IMU
    ParsedData parsePAOGI(const QString& sentence); // PHASE 6.0.22.12: AgOpenGPS dual antenna + IMU
    ParsedData parseGSA(const QString& sentence);   // DOP and satellites
    ParsedData parseGSV(const QString& sentence);   // Satellites in view
    ParsedData parseHDT(const QString& sentence);   // PHASE 6.0.22.12: Heading True
    ParsedData parseAVR(const QString& sentence);   // PHASE 6.0.22.12: Trimble dual antenna
    ParsedData parseKSXT(const QString& sentence);  // PHASE 6.0.22.12: Unicore GNSS/IMU

    // PGN binary parsers

    // Phase 6.0.21.1: REAL PGN parsers (protocol-compliant)
    ParsedData parsePGN126(const QByteArray& data); // Hello AutoSteer + WAS angle
    ParsedData parsePGN253(const QByteArray& data); // AutoSteer Status (periodic)
    ParsedData parsePGN211(const QByteArray& data); // IMU Data (heading, roll, gyro)
    ParsedData parsePGN212(const QByteArray& data); // IMU Disconnect (Phase 6.0.25)
    ParsedData parsePGN214(const QByteArray& data); // GPS Main Antenna
    ParsedData parsePGN203(const QByteArray& data); // Scan Reply (subnet discovery)
    ParsedData parsePGN121(const QByteArray& data); // Hello IMU (heartbeat only)
    ParsedData parsePGN122(const QByteArray& data); // Hello RateControl
    ParsedData parsePGN123(const QByteArray& data); // Hello Machine (relay status)
    ParsedData parsePGN124(const QByteArray& data); // Hello Blockage
    ParsedData parsePGN250(const QByteArray& data); // AutoSteer Sensor (pressure/current)
    ParsedData parsePGN244(const QByteArray& data); // Blockage Data In
    ParsedData parsePGN240(const QByteArray& data); // RateControl Data In

    // Legacy PGN parsers (incorrect byte extraction - to be removed)
    ParsedData parsePGN127(const QByteArray& data); // OLD - was extracting Source ID 0x7F
    ParsedData parsePGN128(const QByteArray& data); // OLD - was extracting header 0x80
    ParsedData parsePGN129(const QByteArray& data); // OLD - was extracting header 0x81

    // Conversion helpers

    double convertNmeaLatLon(const QString& value, const QString& direction);
    double convertKnotsToKmh(double knots);
    static quint8 calculateChecksum(const QString& sentence);
};

// Qt meta-type registration for signal/slot with Qt::QueuedConnection
Q_DECLARE_METATYPE(PGNParser::ParsedData)

#endif // PGNPARSER_H
