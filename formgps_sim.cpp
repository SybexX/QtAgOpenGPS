// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main event sim comms
#include "formgps.h"
#include "qmlutil.h"
#include "classes/settingsmanager.h"
#include <QTime>
#include <QRandomGenerator>
#include "mainwindowstate.h"
#include "siminterface.h"
#include "backend.h"
#include "modulecomm.h"

/* Callback for Simulator new position */
void FormGPS::simConnectSlots()
{
}

void FormGPS::onSimNewPosition(double vtgSpeed,
                     double headingTrue,
                     double latitude,
                     double longitude, double hdop,
                     double altitude,
                     double satellitesTracked)
{
    CNMEA &pn = *Backend::instance()->pn();

    // ✅ PHASE 6.0.21.13: Ignore simulation data when simulation is OFF
    // Prevents conflict: simulation timer still running briefly after disabling simulation
    // Symmetric to Phase 6.0.21.12 which blocks UDP when simulation ON
    // Ensures only ONE data source writes to Q_PROPERTY at a time
    if (!SettingsManager::instance()->menu_isSimulatorOn()) {
        return;  // Simulation mode disabled - ignore simulation data
    }

    // PHASE 6.0.42: Check for GPS jump in simulation mode
    // Handles REAL→SIM mode switch with field open
    // If jump detected: closes field (if open), updates latStart/lonStart, resets flags
    //if (detectGPSJump(latitude, longitude)) {
    //    handleGPSJump(latitude, longitude);
    //}

    pn.vtgSpeed = vtgSpeed;

    CVehicle::instance()->AverageTheSpeed(vtgSpeed);

    pn.headingTrue = pn.headingTrueDual = headingTrue;
    Backend::instance()->m_fixFrame.dualHeading = pn.headingTrueDual;

    // PHASE 6.0.35 FIX: Simulation IMU heading = GPS heading (as in original C# code)
    // CSim.cs:80-81: mf.ahrs.imuHeading = mf.pn.headingTrue
    ahrs.imuHeading = pn.headingTrue;
    if (ahrs.imuHeading >= 360) ahrs.imuHeading -= 360;

    // Phase 6.3.1: Use PropertyWrapper for safe QObject access
    pn.ConvertWGS84ToLocal(latitude,longitude,pn.fix.northing,pn.fix.easting);
    pn.latitude = latitude;
    pn.longitude = longitude;

    // PHASE 6.0.35 FIX: Store RAW GPS position (simulation mode)
    // Symmetric to real GPS mode (formgps_position.cpp:2194-2196)
    // Prevents pn.fix reset to {0,0} when UpdateFixPosition() copies m_rawGpsPosition
    // Speed >= 1.5 km/h bypass ends → UpdateFixPosition() resets pn.fix to m_rawGpsPosition
    {
        QMutexLocker lock(&m_rawGpsPositionMutex);
        m_rawGpsPosition.easting = pn.fix.easting;
        m_rawGpsPosition.northing = pn.fix.northing;
    }

    pn.hdop = hdop;
    pn.altitude = altitude;
    pn.satellitesTracked = satellitesTracked;
    pn.fixQuality = 8;  // Simulation mode (NMEA standard value for simulation)

    //using a random number tests to make sure our Q_GADGET properties
    //are notifying QML properly.
    pn.age = QRandomGenerator::global()->generateDouble();

    Backend::instance()->m_fixFrame.sentenceCounter = 0;
    Backend::instance()->m_fixFrame.droppedSentences = 0;

    ModuleComm::instance()->set_actualSteerAngleDegrees(SimInterface::instance()->steerAngleActual());

    UpdateFixPosition();
}
