// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// PGNParser - Centralized NMEA and PGN binary parser implementation

#include "pgnparser.h"
#include <QDebug>
#include <QtMath>

PGNParser::PGNParser(QObject *parent) : QObject(parent) {
    qDebug() << "PGNParser created - Centralized NMEA + PGN binary parsing";
}

// ========== MAIN PARSERS ==========

// Phase 6.0.21.1: Auto-detection entry point
PGNParser::ParsedData PGNParser::parse(const QByteArray& data) {
    ParsedData result;

    if (data.isEmpty()) {
        return result;  // Invalid - empty data
    }

    // Auto-detect format based on first byte
    unsigned char firstByte = static_cast<unsigned char>(data[0]);

    if (firstByte == '$') {
        // NMEA text format (ASCII, starts with '$')
        QString nmea = QString::fromUtf8(data).trimmed();
        return parseNMEA(nmea);
    }
    else if (data.size() >= 2 && firstByte == 0x80 &&
             static_cast<unsigned char>(data[1]) == 0x81) {
        // PGN binary format (starts with 0x80 0x81)
        return parsePGN(data);
    }
    else {
        // Unknown format
        qDebug() << "PGNParser::parse() - Unknown format, first byte: 0x"
                 << QString::number(firstByte, 16);
        return result;
    }
}

PGNParser::ParsedData PGNParser::parseNMEA(const QString& sentence) {
    ParsedData data;

    if (!isValidNMEA(sentence)) {
        return data;  // Invalid sentence
    }

    // Auto-detect sentence type
    QString type = extractSentenceType(sentence);
    data.sentenceType = type;
    data.sourceType = "NMEA";

    // Route to specific parser
    if (type == "GGA" || type == "GPGGA" || type == "GNGGA") {
        return parseGGA(sentence);
    }
    else if (type == "RMC" || type == "GPRMC" || type == "GNRMC") {
        return parseRMC(sentence);
    }
    else if (type == "VTG" || type == "GPVTG" || type == "GNVTG") {
        return parseVTG(sentence);
    }
    else if (type == "PANDA") {
        return parsePANDA(sentence);  // AgOpenGPS single antenna + IMU
    }
    else if (type == "PAOGI") {
        return parsePAOGI(sentence);  // PHASE 6.0.22.12: AgOpenGPS dual antenna + IMU
    }
    else if (type == "GSA" || type == "GPGSA") {
        return parseGSA(sentence);
    }
    else if (type == "GSV" || type == "GPGSV") {
        return parseGSV(sentence);
    }
    else if (type == "HDT" || type == "GPHDT" || type == "GNHDT") {
        return parseHDT(sentence);  // PHASE 6.0.22.12: Heading True
    }
    else if (type == "PTNL") {
        // PHASE 6.0.22.12: Trimble proprietary - check second field
        QStringList fields = splitNMEA(sentence);
        if (fields.size() >= 2 && fields[1] == "AVR") {
            return parseAVR(sentence);
        }
    }
    else if (type == "KSXT") {
        return parseKSXT(sentence);  // PHASE 6.0.22.12: Unicore GNSS/IMU
    }

    return data;  // Unknown type
}

PGNParser::ParsedData PGNParser::parsePGN(const QByteArray& data) {
    ParsedData result;

    if (!isValidPGN(data)) {
        return result;
    }

    // Extract PGN number (byte 3 - FIXED in Phase 6.0.21.1)
    int pgn = extractPGNNumber(data);
    result.pgnNumber = pgn;
    result.sourceType = "PGN";

    // Route to specific PGN parser
    // Phase 6.0.21.1: Use REAL PGN numbers from protocol specification
    switch (pgn) {
        case 126:  // 0x7E - Hello AutoSteer (WAS angle + status)
            return parsePGN126(data);
        case 253:  // 0xFD - AutoSteer Status (periodic)
            return parsePGN253(data);
        case 250:  // 0xFA - AutoSteer Sensor (pressure/current monitoring)
            return parsePGN250(data);
        case 244:  // 0xFA - Blockage Data
            return parsePGN244(data);
        case 240:  // 0xF0 - RateControl Data
            return parsePGN240(data);
        case 211:  // 0xD3 - IMU Data (heading, roll, pitch)
            return parsePGN211(data);
        case 212:  // 0xD4 - IMU Disconnect (Phase 6.0.25)
            return parsePGN212(data);
        case 214:  // 0xD6 - GPS Main Antenna
            return parsePGN214(data);
        case 203:  // 0xCB - Scan Reply (subnet discovery)
            return parsePGN203(data);
        case 121:  // 0x79 - Hello IMU (heartbeat only)
            return parsePGN121(data);
        case 122:  // 0x7B - Hello RateControl (relay status)
            return parsePGN122(data);
        case 123:  // 0x7B - Hello Machine (relay status)
            return parsePGN123(data);
        case 124:  // 0x7B - Hello Blockage
            return parsePGN124(data);

        // Legacy PGN (to be removed after validation)
        case 127:  // OLD - incorrectly extracted Source ID 0x7F
            qDebug() << "Warning: PGN 127 detected (should be 126 - check byte extraction)";
            return result;
        case 128:  // OLD - incorrectly extracted header byte 0x80
            qDebug() << "Warning: PGN 128 detected (invalid - check byte extraction)";
            return result;
        case 129:  // OLD - incorrectly extracted header byte 0x81
            qDebug() << "Warning: PGN 129 detected (invalid - check byte extraction)";
            return result;

        default:
            qDebug() << "Unknown PGN:" << pgn << "(0x" << QString::number(pgn, 16) << ")";
            return result;
    }
}

// ========== NMEA PARSERS ==========

PGNParser::ParsedData PGNParser::parseGGA(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 15) {
        qDebug() << "GGA sentence too short:" << fields.size();
        return data;
    }

    // ✅ Problem 15 Fix: Validate critical fields NOT empty (C# AgIO line 382-383)
    // C# checks: !string.IsNullOrEmpty(words[2/3/4/5]) before parsing
    // Reject sentence if lat/lon fields empty (GPS fix lost momentarily)
    if (fields[2].isEmpty() || fields[3].isEmpty() || fields[4].isEmpty() || fields[5].isEmpty()) {
        qDebug() << "GGA critical fields empty - rejecting sentence (GPS fix lost)";
        return data;  // Return invalid data (isValid = false)
    }

    // Field 1: UTC time (skip for now)

    // Field 2-3: Latitude (already validated non-empty above)
    data.latitude = convertNmeaLatLon(fields[2], fields[3]);

    // Field 4-5: Longitude (already validated non-empty above)
    data.longitude = convertNmeaLatLon(fields[4], fields[5]);

    // Field 6: Fix quality
    data.quality = fields[6].toInt();

    // Field 7: Satellites
    data.satellites = fields[7].toInt();

    // Field 8: HDOP
    data.hdop = fields[8].toDouble();

    // Field 9: Altitude
    data.altitude = fields[9].toDouble();

    // Field 13: Age of differential
    if (fields.size() > 13 && !fields[13].isEmpty()) {
        data.age = fields[13].toDouble();
    }

    // Phase 6.0.23: Always mark valid - FormGPS decides what to use
    data.isValid = true;
    data.sentenceType = "GGA";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parseRMC(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    // PHASE 6.0.22.12: Accept partial RMC sentences (ModSim sends only 11 fields)
    if (fields.size() < 9) {
        qDebug() << "RMC sentence too short:" << fields.size();
        return data;
    }

    // Field 3-4: Latitude (optional for partial sentences)
    if (fields.size() >= 5 && !fields[3].isEmpty() && !fields[4].isEmpty()) {
        data.latitude = convertNmeaLatLon(fields[3], fields[4]);
    }

    // Field 5-6: Longitude (optional for partial sentences)
    if (fields.size() >= 7 && !fields[5].isEmpty() && !fields[6].isEmpty()) {
        data.longitude = convertNmeaLatLon(fields[5], fields[6]);
    }

    // Field 7: Speed (knots)
    if (fields.size() >= 8 && !fields[7].isEmpty()) {
        data.speed = convertKnotsToKmh(fields[7].toDouble());
    }

    // Field 8: Heading
    if (fields.size() >= 9 && !fields[8].isEmpty()) {
        data.heading = fields[8].toDouble();
    }

    // Phase 6.0.23: Always mark valid - FormGPS decides what to use
    data.isValid = true;
    data.sentenceType = "RMC";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parseVTG(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 9) {
        return data;
    }

    // Field 1: True heading
    if (!fields[1].isEmpty()) {
        data.heading = fields[1].toDouble();
    }

    // Field 7: Speed (km/h)
    if (!fields[7].isEmpty()) {
        data.speed = fields[7].toDouble();
    }

    data.isValid = true;
    data.sentenceType = "VTG";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parsePANDA(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    // Detect $PANDA format by checking field 3
    // NMEA format: $PANDA,time,lat_DDMM,N/S,lon_DDDMM,E/W,quality,sats,hdop,alt,...
    // New format: $PANDA,lat_decimal,lon_decimal,altitude,speed,...

    bool isNmeaFormat = (fields.size() >= 4 && (fields[3] == "N" || fields[3] == "S"));

    if (isNmeaFormat) {
        // AgIO NMEA format: $PANDA,time,lat_DDMM,N/S,lon_DDDMM,E/W,quality,sats,hdop,alt,...
        if (fields.size() < 10) {
            qDebug() << "PANDA NMEA sentence too short:" << fields.size();
            return data;
        }

        // ✅ Problem 15 Fix: Validate critical fields are NOT empty (C# AgIO line 705)
        // C# checks: !string.IsNullOrEmpty(words[1/2/3/5]) before parsing
        // Reject sentence if lat/lon fields empty (GPS fix lost momentarily)
        if (fields[2].isEmpty() || fields[3].isEmpty() || fields[4].isEmpty() || fields[5].isEmpty()) {
            qDebug() << "PANDA NMEA critical fields empty - rejecting sentence (GPS fix lost)";
            return data;  // Return invalid data (isValid = false)
        }

        // Field 1: Time (skip for now)
        // Fields 2-3: Latitude NMEA format
        data.latitude = convertNmeaLatLon(fields[2], fields[3]);

        // Fields 4-5: Longitude NMEA format
        data.longitude = convertNmeaLatLon(fields[4], fields[5]);

        // Field 6: Fix quality
        data.quality = fields[6].toInt();

        // Field 7: Satellites
        data.satellites = fields[7].toInt();

        // Field 8: HDOP
        data.hdop = fields[8].toDouble();

        // Field 9: Altitude
        data.altitude = fields[9].toDouble();

        // Field 10: Age (if present)
        if (fields.size() >= 11) data.age = fields[10].toDouble();

        // Field 11: Speed (if present) - Phase 6.0.34: Knots from firmware, convert to km/h
        // C# AgIO NMEA.Designer.cs:768-770 does: speed *= 1.852f (knots → km/h)
        if (fields.size() >= 12) {
            data.speed = fields[11].toDouble() * 1.852;  // Knots → km/h
        }

        // Fields 12-15: IMU data (from firmware zHandlers.ino BuildNmea lines 320-333)
        // ✅ Problem 14 Fix (Enhanced): Check isEmpty() before parsing IMU fields
        // Note: toInt() returns 0 for BOTH empty string AND "0" - we MUST distinguish:
        // - Empty field → IMU not connected/data unavailable (don't assign)
        // - "0" → Valid 0° roll/pitch/heading (DO assign)

        // Fields 12-15: IMU data (from firmware zHandlers.ino BuildNmea lines 320-333)
        // ALL IMU values are stored as INTEGER x10 in NMEA for precision

        // Field 12: IMU Heading in degrees (imuHeading - INTEGER x10)
        if (fields.size() >= 13 && !fields[12].isEmpty()) {
            int headingRaw = fields[12].toInt();
            data.imuHeading = headingRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 13: Roll angle in degrees (imuRoll - INTEGER x10)
        if (fields.size() >= 14 && !fields[13].isEmpty()) {
            int rollRaw = fields[13].toInt();
            data.imuRoll = rollRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 14: Pitch angle in degrees (imuPitch - INTEGER x10)
        if (fields.size() >= 15 && !fields[14].isEmpty()) {
            int pitchRaw = fields[14].toInt();
            data.imuPitch = pitchRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 15: Yaw Rate in degrees/sec (imuYawRate - INTEGER x10)
        if (fields.size() >= 16 && !fields[15].isEmpty()) {
            int yawRateRaw = fields[15].toInt();
            data.yawRate = yawRateRaw / 10.0;  // Convert x10 integer to degrees/sec
            data.hasIMU = true;
        }

    } else {
        // New decimal format (original code)
        if (fields.size() < 15) {
            qDebug() << "PANDA decimal sentence too short:" << fields.size();
            return data;
        }

        // Field 1: Latitude (decimal degrees)
        data.latitude = fields[1].toDouble();

        // Field 2: Longitude (decimal degrees)
        data.longitude = fields[2].toDouble();

        // Field 3: Altitude
        data.altitude = fields[3].toDouble();

        // Field 4: Speed (km/h)
        data.speed = fields[4].toDouble();

        // Field 5: Heading dual antenna
        data.headingDual = fields[5].toDouble();

        // Field 6: Heading single antenna
        data.heading = fields[6].toDouble();

        // Field 7: Fix quality
        data.quality = fields[7].toInt();

        // Field 8: Satellites
        data.satellites = fields[8].toInt();

        // Field 9: HDOP
        data.hdop = fields[9].toDouble();

        // Field 10: Age
        data.age = fields[10].toDouble();

        // Fields 11-14: IMU data in decimal format (from firmware BuildNmea)
        // ALL IMU values are stored as INTEGER x10 in NMEA for precision

        // Field 11: IMU Heading in degrees (imuHeading - INTEGER x10)
        if (fields.size() >= 12 && !fields[11].isEmpty()) {
            int headingRaw = fields[11].toInt();
            data.imuHeading = headingRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 12: Roll angle in degrees (imuRoll - INTEGER x10)
        if (fields.size() >= 13 && !fields[12].isEmpty()) {
            int rollRaw = fields[12].toInt();
            data.imuRoll = rollRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 13: Pitch angle in degrees (imuPitch - INTEGER x10)
        if (fields.size() >= 14 && !fields[13].isEmpty()) {
            int pitchRaw = fields[13].toInt();
            data.imuPitch = pitchRaw / 10.0;  // Convert x10 integer to degrees
            data.hasIMU = true;
        }

        // Field 14: Yaw Rate in degrees/sec (imuYawRate - INTEGER x10)
        if (fields.size() >= 15 && !fields[14].isEmpty()) {
            int yawRateRaw = fields[14].toInt();
            data.yawRate = yawRateRaw / 10.0;  // Convert x10 integer to degrees/sec
            data.hasIMU = true;
        }
    }

    // Phase 6.0.23: Always mark valid - FormGPS decides what to use
    data.isValid = true;
    data.sentenceType = "PANDA";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parseGSA(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 18) {
        return data;
    }

    // Field 16: HDOP
    if (!fields[16].isEmpty()) {
        data.hdop = fields[16].toDouble();
    }

    data.isValid = true;
    data.sentenceType = "GSA";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parseGSV(const QString& sentence) {
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 4) {
        return data;
    }

    // Field 3: Satellites in view
    data.satellites = fields[3].toInt();

    data.isValid = true;
    data.sentenceType = "GSV";
    data.sourceType = "NMEA";
    return data;
}

// PHASE 6.0.22.12: Additional NMEA parsers for ModSim compatibility

PGNParser::ParsedData PGNParser::parseHDT(const QString& sentence) {
    // HDT - Heading True
    // Format: $GPHDT,heading,T*CS
    // Example: $GPHDT,123.456,T*00
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 2) {
        return data;
    }

    // Field 1: True heading (0-360 degrees)
    if (!fields[1].isEmpty()) {
        data.headingHDT = fields[1].toDouble();
    }

    data.isValid = true;
    data.sentenceType = "HDT";
    data.sourceType = "NMEA";
    return data;
}

PGNParser::ParsedData PGNParser::parseAVR(const QString& sentence) {
    // AVR - Trimble Dual Antenna Attitude
    // Format: $PTNL,AVR,time,yaw,Yaw,tilt,Tilt,roll,Roll,baseline,q,pdop,sat*CS
    // Example: $PTNL,AVR,212405.20,+52.1531,Yaw,-0.0806,Tilt,2.3456,Roll,1.575,3,1.4,16*39
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 13) {
        return data;
    }

    // Field 3: Yaw/Heading (degrees)
    if (!fields[3].isEmpty()) {
        data.headingHDT = fields[3].toDouble();
    }

    // Field 5: Tilt/Pitch (degrees)
    if (!fields[5].isEmpty()) {
        data.imuPitch = fields[5].toDouble();
    }

    // Field 7: Roll (degrees)
    if (!fields[7].isEmpty()) {
        data.imuRoll = fields[7].toDouble();
    }

    // Field 9: Baseline (meters) - optional
    // Field 10: Quality (0-5, 3=RTK fixed)
    if (fields.size() >= 11 && !fields[10].isEmpty()) {
        data.quality = fields[10].toInt();
    }

    // Field 12: Satellites
    if (fields.size() >= 13 && !fields[12].isEmpty()) {
        data.satellites = fields[12].toInt();
    }

    data.isValid = true;
    data.sentenceType = "AVR";
    data.sourceType = "NMEA";
    data.hasIMU = true;  // AVR provides orientation data
    return data;
}

PGNParser::ParsedData PGNParser::parseKSXT(const QString& sentence) {
    // KSXT - Unicore Integrated GNSS/IMU
    // Format: $KSXT,time,lon,lat,alt,yaw,pitch,spd_ang,spd,roll,pos_stat,hdg_stat,hdg_sat,pos_sat,...
    // Example: $KSXT,20191219093115.00,112.87713062,28.23315515,65.5618,45.67,0.00,336.65,0.010,2.3,3,3,13,23,...
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    if (fields.size() < 14) {
        return data;
    }

    // Field 2: Longitude (decimal degrees)
    if (!fields[2].isEmpty()) {
        data.longitude = fields[2].toDouble();
    }

    // Field 3: Latitude (decimal degrees)
    if (!fields[3].isEmpty()) {
        data.latitude = fields[3].toDouble();
    }

    // Field 4: Altitude (meters)
    if (!fields[4].isEmpty()) {
        data.altitude = fields[4].toDouble();
    }

    // Field 5: Yaw/Heading (degrees true, 0-360)
    if (!fields[5].isEmpty()) {
        data.headingHDT = fields[5].toDouble();
        data.imuHeading = fields[5].toDouble();  // Also store as IMU heading
    }

    // Field 6: Pitch (degrees, -90 to 90)
    if (!fields[6].isEmpty()) {
        data.imuPitch = fields[6].toDouble();
    }

    // Field 7: Speed angle / Course over ground (degrees)
    if (!fields[7].isEmpty()) {
        data.heading = fields[7].toDouble();  // COG
    }

    // Field 8: Speed (km/h)
    if (!fields[8].isEmpty()) {
        data.speed = fields[8].toDouble();
    }

    // Field 9: Roll (degrees, -90 to 90)
    if (!fields[9].isEmpty()) {
        data.imuRoll = fields[9].toDouble();
    }

    // Field 10: Position status (0=invalid, 1=single, 2=float, 3=fixed)
    if (!fields[10].isEmpty()) {
        int posStatus = fields[10].toInt();
        data.quality = posStatus;  // Map to quality (3=RTK fixed)
    }

    // Field 13: Position satellites
    if (fields.size() >= 14 && !fields[13].isEmpty()) {
        data.satellites = fields[13].toInt();
    }

    // PHASE 6.0.23.1: Validate GPS fix quality before marking valid
    // Prevent zero coordinates from overwriting valid GPS data in FormGPS
    if (data.quality == 0 && data.latitude == 0.0 && data.longitude == 0.0) {
        // Throttled log (every 200 messages = 5 sec at 40Hz)
        static int noFixLogCounter = 0;
        if (++noFixLogCounter % 200 == 0) {
            qDebug() << "KSXT: GPS no fix (quality=0, lat=0, lon=0) - marking invalid";
        }
        data.isValid = false;
        data.sentenceType = "KSXT";
        data.sourceType = "NMEA";
        data.hasIMU = true;
        return data;
    }

    data.isValid = true;
    data.sentenceType = "KSXT";
    data.sourceType = "NMEA";
    data.hasIMU = true;  // KSXT provides integrated GNSS/INS data
    return data;
}

PGNParser::ParsedData PGNParser::parsePAOGI(const QString& sentence) {
    // PAOGI - AgOpenGPS Dual Antenna + IMU
    // Full format (19 fields): $PAOGI,time,lat,N/S,lon,E/W,q,ss,hdop,alt,M,geoid,M,spd,hdg,roll,pitch,yaw,T*CS
    // Short format (17 fields): $PAOGI,time,lat,N/S,lon,E/W,q,ss,hdop,alt,geoid,spd,hdg,roll,pitch,yaw,T*CS
    ParsedData data;
    QStringList fields = splitNMEA(sentence);

    // Minimum fields: position + quality + IMU data (at least 16 fields)
    if (fields.size() < 16) {
        qDebug() << "PAOGI sentence too short:" << fields.size() << "- need minimum 16 fields";
        return data;
    }

    // Fields 2-3: Latitude (NMEA DDMM.MMMMM format + N/S)
    if (!fields[2].isEmpty() && !fields[3].isEmpty()) {
        data.latitude = convertNmeaLatLon(fields[2], fields[3]);
    }

    // Fields 4-5: Longitude (NMEA DDDMM.MMMMM format + E/W)
    if (!fields[4].isEmpty() && !fields[5].isEmpty()) {
        data.longitude = convertNmeaLatLon(fields[4], fields[5]);
    }

    // Field 6: Quality
    if (!fields[6].isEmpty()) {
        data.quality = fields[6].toInt();
    }

    // Field 7: Satellites
    if (!fields[7].isEmpty()) {
        data.satellites = fields[7].toInt();
    }

    // Field 8: HDOP
    if (!fields[8].isEmpty()) {
        data.hdop = fields[8].toDouble();
    }

    // Field 9: Altitude
    if (!fields[9].isEmpty()) {
        data.altitude = fields[9].toDouble();
    }

    // Detect format: check if field 10 is "M" (full format) or numeric (short format)
    int offset = 0;
    if (fields.size() >= 11 && fields[10] == "M") {
        // Full format with M units: alt,M,geoid,M,spd,hdg,roll,pitch,yaw,T
        offset = 4;  // Fields shifted by 4 (M, geoid, M)
    } else {
        // Short format without M units: alt,geoid,spd,hdg,roll,pitch,yaw,T
        offset = 2;  // Fields shifted by 2 (geoid only)
    }

    // Speed (field 11+offset or 13 for full format, 11 for short)
    // Phase 6.0.34: Knots from firmware, convert to km/h
    // C# AgIO NMEA.Designer.cs:612-614 does: speed *= 1.852f (knots → km/h)
    int speedIdx = (offset == 4) ? 13 : 11;
    if (fields.size() > speedIdx && !fields[speedIdx].isEmpty()) {
        data.speed = fields[speedIdx].toDouble() * 1.852;  // Knots → km/h
    }

    // Heading - DUAL ANTENNA + IMU FUSED (field 12+offset or 14 for full, 12 for short)
    int headingIdx = speedIdx + 1;
    if (fields.size() > headingIdx && !fields[headingIdx].isEmpty()) {
        double fusedHeading = fields[headingIdx].toDouble();
        data.headingHDT = fusedHeading;  // True heading from dual antenna
        data.heading = fusedHeading;     // Also set general heading
    }

    // Roll - FROM DUAL ANTENNA BASELINE GEOMETRY
    int rollIdx = headingIdx + 1;
    if (fields.size() > rollIdx && !fields[rollIdx].isEmpty()) {
        data.imuRoll = fields[rollIdx].toDouble();
    }

    // Pitch - FROM IMU
    int pitchIdx = rollIdx + 1;
    if (fields.size() > pitchIdx && !fields[pitchIdx].isEmpty()) {
        data.imuPitch = fields[pitchIdx].toDouble();
    }

    // Yaw Rate - FROM IMU GYROSCOPE
    int yawIdx = pitchIdx + 1;
    if (fields.size() > yawIdx && !fields[yawIdx].isEmpty()) {
        data.yawRate = fields[yawIdx].toDouble();
    }

    // PHASE 6.0.23.1: Validate GPS fix quality before marking valid
    // Prevent zero coordinates from overwriting valid GPS data in FormGPS
    if (data.quality == 0 && data.latitude == 0.0 && data.longitude == 0.0) {
        // Throttled log (every 200 messages = 5 sec at 40Hz)
        static int noFixLogCounter = 0;
        if (++noFixLogCounter % 200 == 0) {
            qDebug() << "PAOGI: GPS no fix (quality=0, lat=0, lon=0) - marking invalid";
        }
        data.isValid = false;
        data.sentenceType = "PAOGI";
        data.sourceType = "NMEA";
        data.hasIMU = true;
        return data;
    }

    data.isValid = true;
    data.sentenceType = "PAOGI";
    data.sourceType = "NMEA";
    data.hasIMU = true;  // PAOGI provides integrated dual antenna + IMU data
    return data;
}

// ========== PGN BINARY PARSERS ==========

PGNParser::ParsedData PGNParser::parsePGN127(const QByteArray& data) {
    ParsedData result;

    if (data.size() < 10) {
        return result;
    }

    // PGN 127 format: Steer Data IN (WAS - Wheel Angle Sensor)
    // Bytes 0-4: Header (0x80, 0x81, 0x7F, length, CRC)
    // Bytes 5-6: WAS value (int16)

    qint16 wasRaw = (static_cast<unsigned char>(data[5])) |
                    (static_cast<unsigned char>(data[6]) << 8);

    result.wasValue = wasRaw;
    result.hasWAS = true;

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 127;

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN128(const QByteArray& data) {
    ParsedData result;

    if (data.size() < 21) {
        qDebug() << "PGN 128 too short:" << data.size();
        return result;
    }

    // PGN 128 format: GPS binary
    // Bytes 0-4: Header (0x80, 0x81, 0x80, length, CRC)
    // Bytes 5-8: Latitude (int32, 1e-7 degrees)
    // Bytes 9-12: Longitude (int32, 1e-7 degrees)
    // Bytes 13-14: Altitude (int16, cm)
    // Bytes 15-16: Speed (uint16, cm/s)
    // Bytes 17-18: Heading (uint16, 0.01 degrees)
    // Byte 19: Quality
    // Byte 20: Satellites

    qint32 latRaw = (static_cast<unsigned char>(data[5])) |
                    (static_cast<unsigned char>(data[6]) << 8) |
                    (static_cast<unsigned char>(data[7]) << 16) |
                    (static_cast<unsigned char>(data[8]) << 24);

    qint32 lonRaw = (static_cast<unsigned char>(data[9])) |
                    (static_cast<unsigned char>(data[10]) << 8) |
                    (static_cast<unsigned char>(data[11]) << 16) |
                    (static_cast<unsigned char>(data[12]) << 24);

    result.latitude = latRaw * 1e-7;
    result.longitude = lonRaw * 1e-7;

    qint16 altRaw = (static_cast<unsigned char>(data[13])) |
                    (static_cast<unsigned char>(data[14]) << 8);
    result.altitude = altRaw / 100.0;

    quint16 speedRaw = (static_cast<unsigned char>(data[15])) |
                       (static_cast<unsigned char>(data[16]) << 8);
    result.speed = speedRaw * 0.036;  // cm/s to km/h

    quint16 headingRaw = (static_cast<unsigned char>(data[17])) |
                         (static_cast<unsigned char>(data[18]) << 8);
    result.heading = headingRaw * 0.01;

    result.quality = static_cast<unsigned char>(data[19]);
    result.satellites = static_cast<unsigned char>(data[20]);

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 128;

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN129(const QByteArray& data) {
    ParsedData result;

    if (data.size() < 10) {
        qDebug() << "PGN 129 too short:" << data.size();
        return result;
    }

    // PGN 129 format: IMU binary
    // Bytes 0-4: Header (0x80, 0x81, 0x81, length, CRC)
    // Bytes 5-6: Roll (int16, 0.01 degrees)
    // Bytes 7-8: Pitch (int16, 0.01 degrees)
    // Bytes 9-10: Yaw (int16, 0.01 degrees) - optional

    qint16 rollRaw = (static_cast<unsigned char>(data[5])) |
                     (static_cast<unsigned char>(data[6]) << 8);

    qint16 pitchRaw = (static_cast<unsigned char>(data[7])) |
                      (static_cast<unsigned char>(data[8]) << 8);

    result.imuRoll = rollRaw * 0.01;
    result.imuPitch = pitchRaw * 0.01;

    if (data.size() > 10) {
        qint16 yawRaw = (static_cast<unsigned char>(data[9])) |
                        (static_cast<unsigned char>(data[10]) << 8);
        result.imuHeading = yawRaw * 0.01;
    }

    result.hasIMU = true;

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 129;

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN203(const QByteArray& data) {
    ParsedData result;

    if (data.size() < 10) {
        return result;
    }

    // PGN 203 format: Module discovery reply
    // Just validate for now, no specific data extraction

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 203;

    return result;
}

// Phase 6.0.21.1: REAL PGN parsers based on actual protocol specification

PGNParser::ParsedData PGNParser::parsePGN126(const QByteArray& data) {
    // PGN 126 (0x7E): Hello AutoSteer + WAS angle
    // Test validated: 80 81 7E 7E 05 F2 F2 A5 F1 07 47
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x7E = 126 AutoSteer)
    // Byte 3: PGN (0x7E = 126)
    // Byte 4: Length (5 bytes payload)
    // Byte 5-6: WAS angle int16 x 100 (0xF2F2 = -3374 = -33.74°)
    // Byte 7: Switch byte (0xA5)
    // Byte 8-9: Diagnostic (0xF107)
    // Byte 10: Checksum

    ParsedData result;

    if (data.size() < 11) {
        qDebug() << "PGN 126 too short:" << data.size();
        return result;
    }

    // Extract WAS angle (bytes 5-6, int16 x 100)
    qint16 wasAngleRaw = static_cast<qint16>(
        (static_cast<quint8>(data[6]) << 8) | static_cast<quint8>(data[5])
    );
    double wasAngle = wasAngleRaw / 100.0;

    result.wasValue = wasAngleRaw;
    result.hasWAS = true;
    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 126;

    // Log for validation
    static int logCounter = 0;
    if (++logCounter % 50 == 0) {
        qDebug() << "PGN 126 - WAS angle:" << wasAngle << "degrees";
    }

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN253(const QByteArray& data) {
    // PGN 253 (0xFD): AutoSteer Status (periodic, 40Hz)
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x7E = AutoSteer)
    // Byte 3: PGN (0xFD = 253)
    // Byte 4: Length (8 bytes)
    // Byte 5-6: Actual steer angle int16 x100 (degrees)
    // Byte 7-8: IMU heading int16 x10 (degrees)
    // Byte 9-10: IMU roll int16 x10 (degrees)
    // Byte 11: Switch byte (bit 0=work, bit 1=steer)
    // Byte 12: PWM display (0-255)
    // Byte 13: Checksum

    ParsedData result;

    if (data.size() < 14) {
        return result;
    }

    // Validate header and PGN
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }
    if ((unsigned char)data[3] != 0xFD) {
        return result;
    }

    // PHASE 6.0.23: Extract steer angle (bytes 5-6) - int16 x100
    qint16 steerAngleRaw = static_cast<qint16>(
        (static_cast<quint8>(data[6]) << 8) | static_cast<quint8>(data[5])
    );
    result.steerAngleActual = steerAngleRaw;
    result.hasSteerData = true;

    // Extract IMU heading (bytes 7-8) - int16 x10
    qint16 imuHeadingRaw = static_cast<qint16>(
        (static_cast<quint8>(data[8]) << 8) | static_cast<quint8>(data[7])
    );
    if (imuHeadingRaw != 9999) {  // 9999 = invalid/disconnected
        result.imuHeading = imuHeadingRaw * 0.1;
        result.hasIMU = true;
    }

    // Extract IMU roll (bytes 9-10) - int16 x10
    qint16 imuRollRaw = static_cast<qint16>(
        (static_cast<quint8>(data[10]) << 8) | static_cast<quint8>(data[9])
    );
    if (imuRollRaw != 8888) {  // 8888 = invalid/disconnected
        result.imuRoll = imuRollRaw * 0.1;
        result.hasIMU = true;
    }

    // PHASE 6.0.23: Extract switch status byte (byte 11)
    result.switchByte = static_cast<quint8>(data[11]);

    // PHASE 6.0.23: Extract PWM display (byte 12)
    result.pwmDisplay = static_cast<quint8>(data[12]);

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 253;
    result.sentenceType = "AUTOSTEER_STATUS";

    // Debug log (throttled to prevent spam at 40 Hz)
    static int logCounter = 0;
    if (++logCounter % 200 == 0) {  // Log every 200th message (5 sec at 40Hz)
        qDebug() << "PGN 253 - SteerAngle:" << (steerAngleRaw * 0.01)
                 << "deg, PWM:" << result.pwmDisplay
                 << ", Switch:" << QString::number(result.switchByte, 16);
    }

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN240(const QByteArray& data) {
    // PGN 240 (0xF0): RateControl Data
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x7b = Machine)
    // Byte 3: PGN (0xF0 = 240) RateControl
    // Byte 4: Length (4 bytes)
    // Byte 5: Module ID
    // Byte 6-7: RateApplied
    // Byte 8-9: AccQty
    // Byte 10: PWM
    // Byte 11: SensorStat
    // Byte 12: Checksum

    ParsedData result;

    if (data.size() != 13) {
        return result;
    }

    // Validate header and PGN
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }
    if ((unsigned char)data[3] != 0xF0) {
        return result;
    }

    qint16 RateApplied = static_cast<qint16>((static_cast<quint8>(data[7]) << 8) | static_cast<quint8>(data[6]));
    qint16 AccQty = static_cast<qint16>((static_cast<quint8>(data[9]) << 8) | static_cast<quint8>(data[8]));

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 240;
    result.sentenceType = "RateControl_Data_IN";
    result.moduleType = "Machine";
    result.rateControlInData[0] = static_cast<quint8>(data[5]); // ID
    result.rateControlInData[1] = RateApplied;
    result.rateControlInData[2] = AccQty;
    result.rateControlInData[3] = static_cast<quint8>(data[10]); // PWM
    result.rateControlInData[4] = static_cast<quint8>(data[11]); // Status

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN244(const QByteArray& data) {
    // PGN 244 (0xF4): Blockage Data
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x7b = Machine)
    // Byte 3: PGN (0xF4 = 244) Blockage Monitor
    // Byte 4: Length (4 bytes)
    // Byte 5: Module ID
    // Byte 6: Section Number
    // Byte 7-8: Section value
    // Byte 9: Checksum

    ParsedData result;

    if (data.size() != 10) {
        return result;
    }

    // Validate header and PGN
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }
    if ((unsigned char)data[3] != 0xF4) {
        return result;
    }

    int m_ID = static_cast<quint8>(data[5]);
    int sect_n = static_cast<quint8>(data[6]);
    qint16 sect_val = static_cast<qint16>((static_cast<quint8>(data[8]) << 8) | static_cast<quint8>(data[7]));

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 244;
    result.sentenceType = "Blockage_Data_IN";
    result.moduleType = "Machine";
    result.blockagesection[0] = m_ID;
    result.blockagesection[1] = sect_n;
    result.blockagesection[2] = sect_val;

    // // Debug log (throttled to prevent spam at 40 Hz)
    // static int logCounter = 0;
    // if (++logCounter % 200 == 0) {  // Log every 200th message (5 sec at 40Hz)
    //qDebug() << "PGN 244 - Blockage:" ;
    //              << "deg, PWM:" << result.pwmDisplay
    //              << ", Switch:" << QString::number(result.switchByte, 16);
    // }

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN211(const QByteArray& data) {
    // PGN 211 (0xD3): IMU Data
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x79 = 121 IMU)
    // Byte 3: PGN (0xD3 = 211)
    // Byte 4: Length (8 bytes)
    // Byte 5-6: Heading int16
    // Byte 7-8: Roll int16
    // Byte 9-10: Gyro int16
    // Byte 11-12: Reserved (0x00 0x00)
    // Byte 13: Checksum

    ParsedData result;

    if (data.size() < 14) {
        return result;
    }

    // Extract heading (bytes 5-6)
    qint16 headingRaw = static_cast<qint16>(
        (static_cast<quint8>(data[6]) << 8) | static_cast<quint8>(data[5])
    );
    result.imuHeading = headingRaw / 100.0;

    // Extract roll (bytes 7-8)
    qint16 rollRaw = static_cast<qint16>(
        (static_cast<quint8>(data[8]) << 8) | static_cast<quint8>(data[7])
    );
    result.imuRoll = rollRaw / 100.0;

    // Extract gyro (bytes 9-10)
    qint16 gyroRaw = static_cast<qint16>(
        (static_cast<quint8>(data[10]) << 8) | static_cast<quint8>(data[9])
    );
    result.yawRate = gyroRaw / 100.0;

    result.hasIMU = true;
    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 211;

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN212(const QByteArray& data) {
    // PGN 212 (0xD4): IMU Disconnect Status
    // Reference: NMEA_PGN_ARCHITECTURE_REFERENCE.md lines 178-201
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source ID (0x79 = 121 IMU)
    // Byte 3: PGN (0xD4 = 212)
    // Byte 4: Length (1 byte)
    // Byte 5: Disconnect flag (1 = disconnected)
    // Byte 6: Checksum

    ParsedData result;
    result.sourceType = "PGN";
    result.pgnNumber = 212;

    if (data.size() < 7) {
        qDebug() << "Invalid PGN 212 packet size:" << data.size();
        return result;
    }

    // Extract disconnect flag (byte 5)
    uint8_t disconnectFlag = static_cast<uint8_t>(data[5]);

    if (disconnectFlag == 1) {
        // IMU disconnected - set sentinel values
        result.hasIMU = true;
        result.imuHeading = 99999.0;  // Sentinel: IMU heading disconnected
        result.imuRoll = 88888.0;     // Sentinel: IMU roll disconnected
        result.yawRate = 0.0;
        result.isValid = true;
    }

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN214(const QByteArray& data) {
    // PGN 214 (0xD6): GPS Main Antenna
    // This is NMEA-to-PGN converted format (AgIO converts NMEA to PGN 214)
    // For now, just validate presence

    ParsedData result;

    if (data.size() < 10) {
        return result;
    }

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 214;

    // TODO: Extract GPS data from PGN 214 if needed
    // (Most GPS data comes via NMEA text, so this is lower priority)

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN121(const QByteArray& data) {
    // PGN 121 (0x79): Hello IMU OUT
    // Format: 0x80 0x81 0x79 0x79 0x05 [00 00 00 00 00] [CRC]
    // Purpose: Module identification heartbeat ONLY - NO IMU DATA
    // Real IMU data comes via NMEA ($PANDA/$PAOGI)

    ParsedData result;

    if (data.size() < 11) {
        return result;
    }

    // Validate header
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }

    // Validate source and PGN
    if ((unsigned char)data[2] != 0x79 || (unsigned char)data[3] != 0x79) {
        return result;
    }

    // Mark module detected (payload is always empty for PGN 121)
    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 121;
    result.sentenceType = "IMU_HELLO";

    qDebug() << "✅ PGN 121: IMU module detected (hello response)";

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN122(const QByteArray& data) {
    // PGN 122 (0x7A): Hello RC OUT
    // Format: 0x80 0x81 0x79 0x7A 0x05 [00 00 00 00 00] [CRC]
    // Purpose: Module identification heartbeat ONLY - NO IMU DATA
    // Real IMU data comes via NMEA ($PANDA/$PAOGI)

    ParsedData result;

    if (data.size() < 11) {
        return result;
    }

    // Validate header
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }

    // Validate source and PGN
    if ((unsigned char)data[2] != 0x7B || (unsigned char)data[3] != 0x7A) {
        return result;
    }

    // Mark module detected (payload is always empty for PGN 122)
    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 122;
    result.sentenceType = "RateControl_HELLO";

    //qDebug() << "✅ PGN 122: RC module detected (hello response)";

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN123(const QByteArray& data) {
    // PGN 123 (0x7B): Hello Machine OUT
    // Format: 0x80 0x81 0x7B 0x7B 0x05 [relayLo] [relayHi] [00 00 00] [CRC]
    // Purpose: Module identification + relay status

    ParsedData result;

    if (data.size() < 11) {
        return result;
    }

    // Validate header
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }

    // Validate source and PGN
    if ((unsigned char)data[2] != 0x7B || (unsigned char)data[3] != 0x7B) {
        return result;
    }

    // Extract relay status
    unsigned char relayLo = (unsigned char)data[5];  // Relay bits 1-8
    unsigned char relayHi = (unsigned char)data[6];  // Relay bits 9-16

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 123;
    result.sentenceType = "MACHINE_HELLO";

    qDebug() << "✅ PGN 123: Machine module detected (relays:"
             << QString::number(relayLo, 16).toUpper()
             << QString::number(relayHi, 16).toUpper() << ")";

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN124(const QByteArray& data) {
    // PGN 124 (0x7C): Hello Blockage OUT
    // Format: 0x80 0x81 0x7B 0x7C 0x05 [00 00] [00 00] [00 00 00] [CRC]
    // Purpose: Module identification

    ParsedData result;

    if (data.size() < 11) {
        return result;
    }

    // Validate header
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }

    // Validate source and PGN
    if ((unsigned char)data[2] != 0x7B || (unsigned char)data[3] != 0x7C) {
        return result;
    }


    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 124;
    result.sentenceType = "BLOCKAGE_HELLO";

    return result;
}

PGNParser::ParsedData PGNParser::parsePGN250(const QByteArray& data) {
    // PGN 250 (0xFA): AutoSteer Sensor OUT
    // Byte 0-1: Header (0x80 0x81)
    // Byte 2: Source (0x7E = AutoSteer)
    // Byte 3: PGN (0xFA = 250)
    // Byte 4: Length (8 bytes)
    // Byte 5: Sensor value (hydraulic pressure or motor current 0-255)
    // Byte 6-12: Reserved
    // Byte 13: Checksum

    ParsedData result;

    if (data.size() < 14) {
        return result;
    }

    // Validate header
    if ((unsigned char)data[0] != 0x80 || (unsigned char)data[1] != 0x81) {
        return result;
    }

    // Validate source (0x7E = AutoSteer Module) and PGN (0xFA = 250)
    if ((unsigned char)data[2] != 0x7E || (unsigned char)data[3] != 0xFA) {
        return result;
    }

    // PHASE 6.0.23: Extract sensor value (byte 5)
    result.sensorValue = static_cast<quint8>(data[5]);
    result.hasSteerData = true;

    result.isValid = true;
    result.sourceType = "PGN";
    result.pgnNumber = 250;
    result.sentenceType = "AUTOSTEER_SENSOR";

    // Debug log (throttled to prevent spam at 10 Hz)
    static int logCounter = 0;
    if (++logCounter % 50 == 0) {  // Log every 50th message (~5 sec at 10Hz)
        qDebug() << "PGN 250 - Sensor value:" << result.sensorValue;
    }

    return result;
}

// ========== VALIDATION ==========

bool PGNParser::isValidNMEA(const QString& sentence) {
    if (sentence.isEmpty() || !sentence.startsWith('$')) {
        return false;
    }

    // ✅ Problem 15 Fix: Validate checksum strictly (C# AgIO line 79)
    // C# rejects sentences with invalid checksum: while (!ValidateChecksum(sentence));
    // Qt must do the same to prevent corrupted data from being processed
    if (sentence.contains('*')) {
        if (!validateChecksum(sentence)) {
            // Checksum validation failed - reject sentence
            return false;
        }
    }

    return true;  // Accept only if checksum valid OR no checksum present
}

bool PGNParser::isValidPGN(const QByteArray& data) {
    if (data.size() < 5) {
        return false;
    }

    // Check PGN header: 0x80 0x81
    return (static_cast<unsigned char>(data[0]) == 0x80 &&
            static_cast<unsigned char>(data[1]) == 0x81);
}

bool PGNParser::validateChecksum(const QString& sentence) {
    int starPos = sentence.indexOf('*');
    if (starPos == -1) {
        return true;  // No checksum
    }

    QString checksumStr = sentence.mid(starPos + 1, 2);
    bool ok;
    quint8 expectedChecksum = checksumStr.toUInt(&ok, 16);

    if (!ok) {
        qDebug() << "⚠️ Invalid checksum format:" << checksumStr;
        return false;
    }

    quint8 calculatedChecksum = calculateChecksum(sentence);

    bool valid = (calculatedChecksum == expectedChecksum);

    // Debug checksum validation (reduced frequency)
    static int checksumCounter = 0;
    if (!valid || (++checksumCounter % 100 == 0)) {
        qDebug() << (valid ? "✅" : "❌") << "Checksum:"
                 << "Expected:" << QString("%1").arg(expectedChecksum, 2, 16, QChar('0')).toUpper()
                 << "Calculated:" << QString("%1").arg(calculatedChecksum, 2, 16, QChar('0')).toUpper()
                 << "Sentence:" << sentence;
    }

    return valid;
}

quint8 PGNParser::calculateChecksum(const QString& sentence) {
    quint8 checksum = 0;

    int startPos = sentence.indexOf('$') + 1;
    int endPos = sentence.indexOf('*');
    if (endPos == -1) {
        endPos = sentence.length();
    }

    for (int i = startPos; i < endPos; i++) {
        checksum ^= sentence[i].toLatin1();
    }

    return checksum;
}

// ========== UTILITIES ==========

QString PGNParser::extractSentenceType(const QString& sentence) {
    if (sentence.length() < 6 || !sentence.startsWith('$')) {
        return QString();
    }

    // Extract type (e.g., $GPGGA -> GPGGA, $PANDA -> PANDA)
    int commaPos = sentence.indexOf(',');
    if (commaPos == -1) {
        commaPos = sentence.indexOf('*');
    }
    if (commaPos == -1) {
        commaPos = sentence.length();
    }

    return sentence.mid(1, commaPos - 1);
}

int PGNParser::extractPGNNumber(const QByteArray& data) {
    // PGN Protocol: Byte 0=0x80, Byte 1=0x81, Byte 2=Source ID, Byte 3=PGN Number
    if (data.size() < 4) {
        return -1;
    }
    // Phase 6.0.21.1: FIXED - Use byte 3 (PGN number), NOT byte 2 (Source ID)
    return static_cast<unsigned char>(data[3]);
}

QStringList PGNParser::splitNMEA(const QString& sentence) {
    // Remove $ prefix and checksum
    QString clean = sentence;

    if (clean.startsWith('$')) {
        clean = clean.mid(1);
    }

    int starPos = clean.indexOf('*');
    if (starPos != -1) {
        clean = clean.left(starPos);
    }

    // ✅ PHASE 6.0.21.14: Split and trim each field to remove whitespace
    // Fixes parsing issues when fields contain trailing/leading spaces
    // Example: "N " becomes "N" for correct direction matching
    QStringList fields = clean.split(',');
    for (int i = 0; i < fields.size(); ++i) {
        fields[i] = fields[i].trimmed();
    }

    return fields;
}

double PGNParser::convertNmeaLatLon(const QString& value, const QString& direction) {
    if (value.isEmpty()) {
        return 0.0;
    }

    // NMEA format: DDMM.MMMM (latitude) or DDDMM.MMMM (longitude)
    double coord = value.toDouble();

    // Extract degrees and minutes
    int degrees = static_cast<int>(coord / 100.0);
    double minutes = coord - (degrees * 100.0);

    // Convert to decimal degrees
    double decimal = degrees + (minutes / 60.0);

    // Apply direction (S/W are negative)
    if (direction == "S" || direction == "W") {
        decimal = -decimal;
    }

    return decimal;
}

double PGNParser::convertKnotsToKmh(double knots) {
    return knots * 1.852;  // 1 knot = 1.852 km/h
}
