// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main class where everything is initialized
#include "formgps.h"
#include <QColor>
#include <QRgb>
#include "qmlutil.h"
#include "glm.h"
#include "cpgn.h"
#include <QLocale>
#include <QQuickWindow>
#include "classes/settingsmanager.h"
#include <cmath>
#include <QPixmapCache>        // Phase 6.0.45: Memory leak fixes - image cache management
#include <QCoreApplication>    // Phase 6.0.45: Memory leak fixes - sendPostedEvents for deferred deletion
#include <QEvent>              // Phase 6.0.45: Memory leak fixes - QEvent::DeferredDelete enum
#include "backend.h"
#include "mainwindowstate.h"
#include "flagsinterface.h"
#include "siminterface.h"
#include "recordedpath.h"
#include "backendaccess.h"
#include "modulecomm.h"
#include "camera.h"
#include "vehicleproperties.h"
#include "layerservice.h"
#include "boundaryinterface.h"

FormGPS::FormGPS(QWidget *parent) : QQmlApplicationEngine(parent)
{
    qDebug() << "FormGPS constructor START";

    // PHASE 6.0.33: Initialize raw GPS position (two-buffer pattern)
    m_rawGpsPosition = {0.0, 0.0};   // Will be updated when first NMEA packet arrives

    // PHASE 6.0.45: Set QPixmapCache limit to prevent memory leaks
    // Default Qt cache is 10 MB which is too small for QtAgOpenGPS UI
    // Heob analysis showed 94 MB QMovie leak + 15 MB QImage leaks
    // Setting 32 MB limit prevents unbounded growth while allowing adequate caching
    QPixmapCache::setCacheLimit(32768);  // 32 MB = 32768 KB
    qDebug() << "PHASE 6.0.45: QPixmapCache limit set to 32 MB";

    qDebug() << "Setting up basic connections...";
    connect(this, &FormGPS::do_processSectionLookahead, this, &FormGPS::processSectionLookahead, Qt::QueuedConnection);
    connect(this, &FormGPS::do_processOverlapCount, this, &FormGPS::processOverlapCount, Qt::QueuedConnection);
    
    qDebug() << "ðŸ”§ Setting up AgIO service FIRST...";
    setupAgIOService();
    // Phase 6.0.27: DISABLED legacy parsedDataReady connection
    // Using separated signals (nmeaDataReady, imuDataReady, steerDataReady) instead
    // Old connection caused double updates: both onParsedDataReady() and onNmeaDataReady()
    // processed the same $PANDA sentence → race condition → satellite count fluctuation
    // Phase 6.0.21: Connect to AgIOService broadcast signal for GPS/IMU data
    // Phase 6.0.23.4: DirectConnection for real-time 40 Hz (same thread, no queue)
    // Phase 6.0.24: QueuedConnection tested but caused 100% CPU → reverted
    // connect(m_agioService, &AgIOService::parsedDataReady,
    //         this, &FormGPS::onParsedDataReady, Qt::DirectConnection);
    // qDebug() << "Phase 6.0.21: Connected to AgIOService::parsedDataReady signal";

    // Phase 6.0.25: Connect separated data signals for optimal routing
    connect(m_agioService, &AgIOService::nmeaDataReady,
            this, &FormGPS::onNmeaDataReady, Qt::DirectConnection);

    connect(m_agioService, &AgIOService::imuDataReady,
            this, &FormGPS::onImuDataReady, Qt::DirectConnection);

    connect(m_agioService, &AgIOService::steerDataReady,
            this, &FormGPS::onSteerDataReady, Qt::DirectConnection);

    connect(m_agioService, &AgIOService::machineDataReady,
            this, &FormGPS::onMachineDataReady, Qt::DirectConnection);

    qDebug() << "Phase 6.0.25: Separated NMEA/IMU/Steer signal connections established";

    qDebug() << "ðŸŽ¯ Initializing singletons...";
    // ===== CRITIQUE : Initialiser les singletons AVANT connect_classes() =====
    // CTrack will be auto-initialized via QML singleton pattern
    qDebug() << "  âœ… CTrack singleton will be auto-created by Qt";
    
    // Qt 6.8: Constructor ready for QML loading
    qDebug() << "âœ… FormGPS constructor core completed - ready for QML loading";

    qDebug() << "ðŸŽ¨ Now loading QML interface (AFTER constructor completion)...";
    setupGui();

    // ===== PHASE 6.3.1: PropertyWrapper initialization moved to initializeQMLInterfaces() =====
    // PropertyWrapper must be initialized AFTER QML objects are fully loaded and accessible
    qDebug() << "ðŸ”§ Phase 6.3.1: PropertyWrapper initialization will happen in initializeQMLInterfaces()";

    // Initialize AgIO singleton AFTER FormLoop is ready
    // Old QMLSettings removed - now using AgIOService singleton
    // Pure Qt 6.8 approach - factory function should be called automatically

    qDebug() << "  âœ… AgIO service initialized in main thread";
    //loadSettings(;

    // Initialize language translation system
    m_translator = new QTranslator(this);
    on_language_changed(); // Load initial translation and set up QML retranslation

    // === CRITICAL: applicationClosing connection for save_everything fix ===
    // When applicationClosing changes → automatically save with vehicle
    // Note: Using connect() instead of setBinding() to avoid recursive binding loops
    connect(Backend::instance(), &Backend::applicationClosingChanged, this, [this]() {
        if (Backend::instance()->applicationClosing()) {
            qDebug() << "🚨 applicationClosing detected - scheduling vehicle save";
            // Defer save to avoid conflicts and allow property propagation
            QTimer::singleShot(100, this, [this]() {
                qDebug() << "💾 Executing applicationClosing save with vehicle";
                FileSaveEverythingBeforeClosingField(true);  // Save vehicle on app exit
                qDebug() << "✅ applicationClosing save completed";
            });
        }
    });
    qDebug() << "🔗 applicationClosing connection established for save_everything replacement";

    qDebug() << "âœ… FormGPS full initialization completed";

    // Note: QML Component.onCompleted will trigger after setupGui() completes
    // The initialization will happen via one of our three protection mechanisms

}

FormGPS::~FormGPS()
{
    qDebug() << "FormGPS destructor START - cleaning up resources";

    // Phase 6.0.45: Memory leak fixes - 5-step QML cleanup sequence

    // Step 1: Clear QML engine component cache to free QQmlObjectCreator instances
    // Addresses: 90,513 leaked QQmlObjectCreator::createInstance() allocations
    clearComponentCache();
    qDebug() << "QML component cache cleared";

    // Step 2: Trigger JavaScript garbage collection to free QML objects
    // Addresses: 62,426 leaked QQmlObjectCreator::populateInstance() allocations
    collectGarbage();
    qDebug() << "QML garbage collection completed";

    // Step 3: Clear Qt image caches to free QPixmap/QImage allocations
    // Addresses: 94 MB QMovie leak + 15 MB QImage leaks
    QPixmapCache::clear();
    qDebug() << "QPixmapCache cleared";

    // Step 4: Disconnect all signal/slot connections to prevent dangling references
    // Addresses: 62,194 leaked QQmlObjectCreator::setPropertyBinding() allocations
    disconnect();
    qDebug() << "All signals disconnected";

    // Step 5: Clean up AgIO service (existing cleanup)
    cleanupAgIOService();
    qDebug() << "AgIO service cleaned up";

    // Step 6: Process pending deleteLater() calls to ensure deferred deletions complete
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    qDebug() << "Pending deletions processed";

    // Clean up translator (automatically cleaned by parent)
    // translator will be deleted automatically by parent object

    qDebug() << "FormGPS destructor COMPLETE";
}

void FormGPS::processOverlapCount()
{
    if (Backend::instance()->isJobStarted())
    {
        int once = 0;
        int twice = 0;
        int more = 0;
        int level = 0;
        double total = 0;
        double total2 = 0;

        //50, 96, 112
        for (int i = 0; i < 400 * 400; i++)
        {

            if (overPixels[i].red > 105)
            {
                more++;
                level = overPixels[i].red;
            }
            else if (overPixels[i].red > 85)
            {
                twice++;
                level = overPixels[i].red;
            }
            else if (overPixels[i].red > 50)
            {
                once++;
            }
        }
        total = once + twice + more;
        total2 = total + twice + more + more;

        if (total2 > 0)
        {
            Backend::instance()->currentField_setActualAreaCovered( (total / total2 * Backend::instance()->m_currentField.workedAreaTotal));
            fd.overlapPercent = ((1 - total / total2) * 100);
        }
        else
        {
            Backend::instance()->currentField_setActualAreaCovered( 0);
            fd.overlapPercent = 0;
        }
    }
}

// PHASE 6.0.40: Reset GPS state when switching between sim/real modes
// Prevents gray screen bug when toggling between simulation and real GPS
void FormGPS::ResetGPSState(bool toSimMode)
{
    CNMEA &pn = *Backend::instance()->pn();
    // PHASE 6.0.42.7: Save and close field before mode switch
    // Field data is tied to current coordinate system (latStart/lonStart)
    // Switching modes changes coordinate reference → must save field before switch
    // Same logic as GPS jump detection (handleGPSJump)
    if (Backend::instance()->isJobStarted()) {
        qDebug() << "Field open during mode switch - saving and closing";
        FileSaveEverythingBeforeClosingField(false);  // Save all field data
        JobClose();  // Close field properly
        qDebug() << "Field closed successfully before mode switch";
    }

    // Reset initialization flags
    isGPSPositionInitialized = false;
    isFirstFixPositionSet = false;
    CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(false);
    //isFirstHeadingSet = false;
    startCounter = 0;

    // PHASE 6.0.42: Reset guidance line offset when switching modes
    // Old offset from previous mode is invalid in new coordinate system
    CVehicle::instance()->set_guidanceLineDistanceOff(32000);
    CVehicle::instance()->guidanceLineSteerAngle = 0;

    // PHASE 6.0.42: Reset stepFixPts[] for heading calculation
    // Old position history from previous coordinate system is invalid
    // Must accumulate 3 new points to calculate heading automatically
    for (int i = 0; i < totalFixSteps; i++) {
        stepFixPts[i].isSet = 0;
    }

    // Reset sentence counter to prevent "No GPS" false alarm
    Backend::instance()->m_fixFrame.sentenceCounter = 0;

    if (toSimMode) {
        // Initialize with simulation coordinates
        pn.latitude = SimInterface::instance()->latitude;
        pn.longitude = SimInterface::instance()->longitude;
        pn.headingTrue = 0;

        // CRITICAL: Initialize latStart/lonStart for sim mode
        // Without this, ConvertWGS84ToLocal uses wrong reference point
        pn.setLatStart(pn.latitude);
        pn.setLonStart(pn.longitude);
        pn.SetLocalMetersPerDegree();

        // Convert sim position to local coords
        pn.ConvertWGS84ToLocal(SimInterface::instance()->latitude, SimInterface::instance()->longitude, pn.fix.northing, pn.fix.easting);

        // Initialize raw GPS position for sim (used for heading calculation)
        {
            QMutexLocker lock(&m_rawGpsPositionMutex);
            m_rawGpsPosition.easting = pn.fix.easting;
            m_rawGpsPosition.northing = pn.fix.northing;
        }

        // PHASE 6.0.42: Update last known position for jump detection
        // Prevents false jump detection when switching to simulation
        m_lastKnownLatitude = pn.latitude;
        m_lastKnownLongitude = pn.longitude;

        // PHASE 6.0.42.6: Reset IMU values for simulation mode
        // Simulation = perfect flat terrain, no roll/pitch/yaw
        // Prevents old real-mode IMU values from corrupting simulation position calculations
        // Old IMU values would cause incorrect roll corrections and sidehill compensation
        ahrs.imuRoll = 0.0;     // Flat terrain (no roll)
        ahrs.imuPitch = 0.0;    // No slope (no pitch)
        ahrs.imuYawRate = 0.0;  // No rotation (no yaw rate)
        // Note: ahrs.imuHeading updated by onSimNewPosition() = pn.headingTrue (line 69)

        // PHASE 6.0.42.1: Mark position as initialized since we just set latStart/lonStart above
        // Allows startCounter to increment immediately instead of wasting 1 cycle in InitializeFirstFewGPSPositions()
        isFirstFixPositionSet = true;
    } else {
        // PHASE 6.0.42.3: Real mode - INVALIDATE position to prevent premature initialization
        // Problem: If we keep SIM coordinates, InitializeFirstFewGPSPositions() initializes
        // with stale SIM coords BEFORE real GPS arrives → gray screen when GPS finally arrives
        // Solution: Reset pn.latitude/longitude to 0 to force waiting for real GPS
        // This makes behavior identical to application startup (REAL + UDP ON)

        // PHASE 6.0.42.4: ALSO reset m_lastKnownLatitude/longitude to prevent false GPS jump detection
        // Problem: If we keep SIM coords in m_lastKnown*, detectGPSJump() triggers when real GPS arrives
        // → handleGPSJump() resets everything → gray screen even with UDP ON
        // Solution: Reset m_lastKnown* to 0 → first real GPS treated as "first fix", not a "jump"
        m_lastKnownLatitude = 0;
        m_lastKnownLongitude = 0;

        // Invalidate current position - forces waiting for real GPS data
        pn.latitude = 0;
        pn.longitude = 0;
        pn.fix.easting = 0;
        pn.fix.northing = 0;

        // Reset raw GPS position - will be updated when real GPS data arrives
        {
            QMutexLocker lock(&m_rawGpsPositionMutex);
            m_rawGpsPosition.easting = 0;
            m_rawGpsPosition.northing = 0;
        }

        // PHASE 6.0.41: Force latStart/lonStart reinitialization even if field is open
        // Prevents gray screen when GPS arrives after mode switch with open field
        m_forceGPSReinitialization = true;

        // PHASE 6.0.42.2: DON'T set isFirstFixPositionSet = true in REAL mode
        // Because we don't have valid GPS coordinates yet (UDP OFF scenario)
        // InitializeFirstFewGPSPositions() will wait for real GPS to arrive
    }

    // Reset previous positions for heading calculation
    prevFix.easting = pn.fix.easting;
    prevFix.northing = pn.fix.northing;
    prevDistFix = pn.fix;
}

// PHASE 6.0.42: Detect if GPS position jumped drastically
// Prevents gray screen and field corruption when GPS coordinates change significantly
// Use cases: SIM<->REAL mode switch, GPS module change, position correction after signal loss
bool FormGPS::detectGPSJump(double newLat, double newLon)
{
    // First GPS fix - not a jump
    if (m_lastKnownLatitude == 0 && m_lastKnownLongitude == 0) {
        m_lastKnownLatitude = newLat;
        m_lastKnownLongitude = newLon;
        return false;
    }

    // Calculate distance in kilometers using Haversine approximation
    // dLat: Latitude difference in km (111.32 km per degree latitude)
    // dLon: Longitude difference in km (adjusted by cosine of latitude for Earth curvature)
    double dLat = (newLat - m_lastKnownLatitude) * 111.32;
    double dLon = (newLon - m_lastKnownLongitude) * 111.32 * cos(newLat * 0.01745329);
    double distanceKm = sqrt(dLat*dLat + dLon*dLon);

    qDebug() << "GPS position check: distance from last known =" << distanceKm << "km";

    return (distanceKm > GPS_JUMP_THRESHOLD_KM);
}

// PHASE 6.0.42: Handle GPS jump - save field, regenerate OpenGL center, update position
// This function ensures clean transition when GPS coordinates change significantly:
// 1. Save and close current field (if open) to prevent coordinate corruption
// 2. Update latStart/lonStart with new GPS position (OpenGL reference point)
// 3. Reset GPS initialization flags for proper reinitialization
// 4. Update last known position for future jump detection
void FormGPS::handleGPSJump(double newLat, double newLon)
{
    CNMEA &pn = *Backend::instance()->pn();

    qDebug() << "GPS JUMP DETECTED - regenerating OpenGL center";
    qDebug() << "Old position: lat=" << m_lastKnownLatitude << "lon=" << m_lastKnownLongitude;
    qDebug() << "New position: lat=" << newLat << "lon=" << newLon;

    // If field is open, save and close it to prevent coordinate corruption
    // Field data is tied to specific GPS coordinates (latStart/lonStart)
    // When GPS jumps, field coordinates no longer match real-world positions
    if (Backend::instance()->isJobStarted()) {
        qDebug() << "Field open during GPS jump - saving and closing";
        FileSaveEverythingBeforeClosingField(false);  // Save all field data (sections, boundary, contour, flags, tracks)
        JobClose();  // Close field properly (clears boundaries, sections, resets flags)
        qDebug() << "Field closed successfully";
    }

    // Update latStart/lonStart with new GPS position
    // These are the reference coordinates for WGS84->Local conversion
    // OpenGL rendering uses local meters (northing/easting) calculated from these
    pn.setLatStart(newLat);
    pn.setLonStart(newLon);
    pn.SetLocalMetersPerDegree();  // Recalculate meters per degree for new latitude

    // PHASE 6.0.42.1: Reset GPS initialization for reinitialization cycle
    // BUT set isFirstFixPositionSet = true because we just initialized latStart/lonStart above
    // This allows startCounter to increment immediately instead of wasting 1 cycle
    isGPSPositionInitialized = false;
    isFirstFixPositionSet = true;  // Position reference initialized above
    CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(false);
    startCounter = 0;

    // PHASE 6.0.42.1: Update pn structure with new GPS position
    // Ensures pn.latitude/longitude match the new reference point
    pn.latitude = newLat;
    pn.longitude = newLon;

    // CRITICAL: Convert new GPS position to local coordinates using new reference point
    // This ensures pn.fix.northing/easting are valid for the first UpdateFixPosition() call
    pn.ConvertWGS84ToLocal(newLat, newLon, pn.fix.northing, pn.fix.easting);

    // PHASE 6.0.42.1: Initialize raw GPS position for heading calculation
    // Symmetric to simulation mode initialization (formgps_sim.cpp:82-85)
    {
        QMutexLocker lock(&m_rawGpsPositionMutex);
        m_rawGpsPosition.easting = pn.fix.easting;
        m_rawGpsPosition.northing = pn.fix.northing;
    }

    // PHASE 6.0.42.1: Initialize prevFix for position tracking
    // Ensures first position update after jump has valid reference
    prevFix.easting = pn.fix.easting;
    prevFix.northing = pn.fix.northing;

    // PHASE 6.0.42: Reset guidance line distance offset
    // Old offset based on previous coordinate system is now invalid
    // Set to 32000 = "no guidance active" until new guidance line is calculated
    CVehicle::instance()->set_guidanceLineDistanceOff(32000);
    CVehicle::instance()->guidanceLineSteerAngle = 0;

    // PHASE 6.0.42: Reset stepFixPts[] for heading calculation
    // Old position history from previous coordinate system is invalid
    // System will accumulate 3 new points and auto-calculate heading when speed > 1.5 km/h
    for (int i = 0; i < totalFixSteps; i++) {
        stepFixPts[i].isSet = 0;
    }

    // Update last known position for future jump detection
    m_lastKnownLatitude = newLat;
    m_lastKnownLongitude = newLon;
}

void FormGPS::tmrWatchdog_timeout()
{
    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_TRACK(track);
    BACKEND_YT(yt);

    //TODO: replace all this with individual timers for cleaner

    // PHASE 6.0.40: Detect mode change and reset GPS state
    // Prevents gray screen when switching between sim and real modes
    static bool wasSimulatorOn = SettingsManager::instance()->menu_isSimulatorOn();
    bool isSimulatorOn = SettingsManager::instance()->menu_isSimulatorOn();

    if (wasSimulatorOn != isSimulatorOn) {
        // Mode changed - reset GPS state to prevent gray screen bug
        qDebug() << "Mode switch detected:" << (wasSimulatorOn ? "SIM to REAL" : "REAL to SIM");
        //TODO: redundant code with ResetGPSState()
        ResetGPSState(isSimulatorOn);
        wasSimulatorOn = isSimulatorOn;
    }

    if (! isSimulatorOn && SimInterface::instance()->isRunning()) {
        qDebug() << "Shutting down simulator.";
        SimInterface::instance()->shutDown();
    } else if (isSimulatorOn && ! SimInterface::instance()->isRunning() ) {
        qDebug() << "Starting up simulator.";
        // Old initialization removed - now done in ResetGPSState()
        pn.latitude = SimInterface::instance()->latitude;
        pn.longitude = SimInterface::instance()->longitude;
        pn.headingTrue = 0;

        // PHASE 6.0.35 FIX: Initialize latStart/lonStart BEFORE first conversion
        // Problem: onSimNewPosition() calls ConvertWGS84ToLocal() BEFORE UpdateFixPosition() initializes latStart/lonStart
        // Solution: Initialize latStart/lonStart here when simulation starts (similar to real GPS mode)
        // This ensures ConvertWGS84ToLocal() uses correct reference point from first conversion
        pn.setLatStart(pn.latitude);
        pn.setLonStart(pn.longitude);
        pn.SetLocalMetersPerDegree();

        gpsHz = 10;
        SimInterface::instance()->startUp();
    }

    Backend::instance()->m_fixFrame.sentenceCounter += 1;
    //notify QML here since UpdateFixPosition() only runs with a new fix
    emit Backend::instance()->fixFrameChanged();

    if (tenSecondCounter++ >= 40)
    {
        tenSecondCounter = 0;
        tenSeconds++;
    }
    if (threeSecondCounter++ >= 12)
    {
        threeSecondCounter = 0;
        threeSeconds++;
    }
    if (oneSecondCounter++ >= 4)
    {
        oneSecondCounter = 0;
        oneSecond++;
    }
    if (oneHalfSecondCounter++ >= 2)
    {
        oneHalfSecondCounter = 0;
        oneHalfSecond++;
    }
    if (oneFifthSecondCounter++ >= 0)
    {
        oneFifthSecondCounter = 0;
        oneFifthSecond++;
    }

    ////////////////////////////////////////////// 10 second ///////////////////////////////////////////////////////
    //every 10 second update status
    if (tenSeconds != 0)
    {
        //reset the counter
        tenSeconds = 0;
    }
    /////////////////////////////////////////////////////////   333333333333333  ////////////////////////////////////////
    //every 3 second update status
    if (displayUpdateThreeSecondCounter != threeSeconds)
    {
        //reset the counter
        displayUpdateThreeSecondCounter = threeSeconds;

        //check to make sure the grid is big enough
        //worldGrid.checkZoomWorldGrid(pn.fix.northing, pn.fix.easting;

        //hide the NAv panel in 6  secs
        // TODO:

        //save nmea log file
        //TODO: if (isLogNMEA) FileSaveNMEA(;

        //update button lines numbers
        //TODO: UpdateGuidanceLineButtonNumbers(;

    }//end every 3 seconds

    //every second update all status ///////////////////////////   1 1 1 1 1 1 ////////////////////////////
    if (displayUpdateOneSecondCounter != oneSecond)
    {
        //reset the counter
        displayUpdateOneSecondCounter = oneSecond;

        //counter used for saving field in background
        minuteCounter++;
        tenMinuteCounter++;

        // PHASE 6.0.42.9: Auto-track timer increment (C# GUI.Designer.cs:275)
        // Enables automatic switching to closest track as tractor moves
        // Timer prevents rapid switching (max 1 switch/second when >= 1)
        track.autoTrack3SecTimer++;
    }

    //every half of a second update all status  ////////////////    0.5  0.5   0.5    0.5    /////////////////
    if (displayUpdateHalfSecondCounter != oneHalfSecond)
    {
        //reset the counter
        displayUpdateHalfSecondCounter = oneHalfSecond;

        isFlashOnOff = !isFlashOnOff;

        //Make sure it is off when it should
        if (!MainWindowState::instance()->isContourBtnOn() &&
            track.idx() == -1 &&
            MainWindowState::instance()->isBtnAutoSteerOn() )
        {

            MainWindowState::instance()->set_isBtnAutoSteerOn(false);
        }

    } //end every 1/2 second

    //every fourth second update  ///////////////////////////   Fourth  ////////////////////////////
    {
        //reset the counter
        oneHalfSecondCounter++;
        oneSecondCounter++;
        yt.makeUTurnCounter++;

        secondsSinceStart = stopwatch.elapsed() / 1000.0;
    }
}

void FormGPS::SwapDirection() {
    BACKEND_YT(yt);

    if (!yt.isYouTurnTriggered)
    {
        yt.isYouTurnRight = ! yt.isYouTurnRight;
        yt.ResetCreatedYouTurn();
    }
    else if (MainWindowState::instance()->isYouTurnBtnOn())
    {
        MainWindowState::instance()->set_isYouTurnBtnOn(false);
    }
}


void FormGPS::JobClose()
{
    BACKEND_TRACK(track);
    BACKEND_YT(yt);

    lock.lockForWrite();
    recPath.resumeState = 0;
    recPath.currentPositonIndex = 0;

    sbGrid.clear();

    //reset field offsets

    Backend::instance()->pn()->set_fixOffset(Vec2(0,0));

    //turn off headland
    MainWindowState::instance()->set_isHeadlandOn(false); //this turns off the button

    recPath.recList.clear();
    recPath.StopDrivingRecordedPath();

    //make sure hydraulic lift is off
    ModuleComm::instance()->p_239.pgn[CPGN_EF::hydLift] = 0;
    emit ModuleComm::instance()->p_239_changed();

    CVehicle::instance()->setIsHydLiftOn(false); //this turns off the button also - Qt 6.8

    //oglZoom.SendToBack(;

    //clean all the lines
    bnd.bndList.clear();
    //TODO: bnd.shpList.clear(;


    Backend::instance()->set_isJobStarted(false);

    //fix ManualOffOnAuto buttons
    MainWindowState::instance()->set_manualBtnState(SectionState::Off);

    //fix auto button
    MainWindowState::instance()->set_autoBtnState(SectionState::Off);

    // ⚡ PHASE 6.0.20: Disable AutoSteer when job closes (safety + clean state)
    MainWindowState::instance()->set_isBtnAutoSteerOn(false);

    /*
    btnZone1.BackColor = Color.Silver;
    btnZone2.BackColor = Color.Silver;
    btnZone3.BackColor = Color.Silver;
    btnZone4.BackColor = Color.Silver;
    btnZone5.BackColor = Color.Silver;
    btnZone6.BackColor = Color.Silver;
    btnZone7.BackColor = Color.Silver;
    btnZone8.BackColor = Color.Silver;

    btnZone1.Enabled = false;
    btnZone2.Enabled = false;
    btnZone3.Enabled = false;
    btnZone4.Enabled = false;
    btnZone5.Enabled = false;
    btnZone6.Enabled = false;
    btnZone7.Enabled = false;
    btnZone8.Enabled = false;

    btnSection1Man.Enabled = false;
    btnSection2Man.Enabled = false;
    btnSection3Man.Enabled = false;
    btnSection4Man.Enabled = false;
    btnSection5Man.Enabled = false;
    btnSection6Man.Enabled = false;
    btnSection7Man.Enabled = false;
    btnSection8Man.Enabled = false;
    btnSection9Man.Enabled = false;
    btnSection10Man.Enabled = false;
    btnSection11Man.Enabled = false;
    btnSection12Man.Enabled = false;
    btnSection13Man.Enabled = false;
    btnSection14Man.Enabled = false;
    btnSection15Man.Enabled = false;
    btnSection16Man.Enabled = false;

    btnSection1Man.BackColor = Color.Silver;
    btnSection2Man.BackColor = Color.Silver;
    btnSection3Man.BackColor = Color.Silver;
    btnSection4Man.BackColor = Color.Silver;
    btnSection5Man.BackColor = Color.Silver;
    btnSection6Man.BackColor = Color.Silver;
    btnSection7Man.BackColor = Color.Silver;
    btnSection8Man.BackColor = Color.Silver;
    btnSection9Man.BackColor = Color.Silver;
    btnSection10Man.BackColor = Color.Silver;
    btnSection11Man.BackColor = Color.Silver;
    btnSection12Man.BackColor = Color.Silver;
    btnSection13Man.BackColor = Color.Silver;
    btnSection14Man.BackColor = Color.Silver;
    btnSection15Man.BackColor = Color.Silver;
    btnSection16Man.BackColor = Color.Silver;
    */

    //clear the section lists
    for (int j = 0; j < tool.triStrip.count(); j++)
    {
        //clean out the lists
        tool.triStrip[j].patchList.clear();
        tool.triStrip[j].triangleList.clear();
    }

    tool.triStrip.clear();
    tool.triStrip.append(CPatches());

    //clear coverage layers
    LayerService::instance()->clearAllLayers();

    //turn off all boundaries.
    BoundaryInterface::instance()->properties()->clearAll();

    //invalidate all GPU patch list buffers. Must be destroyed
    //in the OpenGL context, so deferred to the next drawing
    //pass.
    tool.patchesBufferDirty = true;

    //clear the flags
    FlagsInterface::instance()->clearFlags();

    //ABLine
    tram.tramList.clear();

    track.ResetCurveLine();

    //tracks
    track.gArr.clear();
    track.setIdx(-1);

    //clean up tram
    tram.displayMode = 0;
    tram.generateMode = 0;
    tram.tramBndInnerArr.clear();
    tram.tramBndOuterArr.clear();

    //clear out contour and Lists
    ct.ResetContour();
    MainWindowState::instance()->set_isContourBtnOn(false); //turns off button in gui
    ct.isContourOn = false;

    //btnABDraw.Enabled = false;
    //btnCycleLines.Image = Properties.Resources.ABLineCycle;
    //btnCycleLines.Enabled = false;
    //btnCycleLinesBk.Image = Properties.Resources.ABLineCycleBk;
    //btnCycleLinesBk.Enabled = false;

    //AutoSteer
    //btnAutoSteer.Enabled = false;
    MainWindowState::instance()->set_isBtnAutoSteerOn(false);

    //auto YouTurn shutdown
    MainWindowState::instance()->set_isYouTurnBtnOn(false);

    yt.ResetYouTurn();

    //reset acre and distance counters
    Backend::instance()->currentField_setWorkedAreaTotal(0);

    //reset GUI areas
    bnd.UpdateFieldBoundaryGUIAreas();

    displayFieldName = tr("None");

    recPath.recList.clear();
    recPath.shortestDubinsList.clear();
    recPath.shuttleDubinsList.clear();

    //FixPanelsAndMenus();
    Camera::instance()->SetZoom();

    //release Bing texture
    lock.unlock();
}

void FormGPS::JobNew()
{
    BACKEND_TRACK(track);


    JobClose();

    startCounter = 0;

    //btnSectionMasterManual.Enabled = true;
    MainWindowState::instance()->set_manualBtnState(SectionState::Off);
    //btnSectionMasterManual.Image = Properties.Resources.ManualOff;

    //btnSectionMasterAuto.Enabled = true;
    MainWindowState::instance()->set_autoBtnState(SectionState::Off);
    //btnSectionMasterAuto.Image = Properties.Resources.SectionMasterOff;

    lock.lockForWrite();
    track.ABLine.abHeading = 0.00;

    Camera::instance()->SetZoom();
    fileSaveCounter = 25;
    track.setIsAutoTrack(false);
    Backend::instance()->set_isJobStarted(true);

    // PHASE 6.0.29: Reset recorded path flags when opening field
    // Prevents steer from activating due to garbage flag values (formgps_position.cpp:800)
    RecordedPath::instance()->set_isDrivingRecordedPath(false);
    recPath.isFollowingDubinsToPath = false;
    recPath.isFollowingRecPath = false;
    recPath.isFollowingDubinsHome = false;

    // PHASE 6.0.30: Auto-enable recording when job starts
    // Vehicle trail (yellow line) needs isRecordOn=true to populate recList (formgps_position.cpp:1821)
    // Phase 6.0.29 initialized isRecordOn=false in constructor, which emptied the trail after JobNew()
    recPath.isRecordOn = true;
    tool.patchesBufferDirty = true;

    lock.unlock();

}

void FormGPS::FileSaveEverythingBeforeClosingField(bool saveVehicle)
{
    qDebug() << "shutting down, saving field items.";

    if (! Backend::instance()->isJobStarted()) return;

    qDebug() << "Test3";
    lock.lockForWrite();
    //turn off contour line if on
    if (ct.isContourOn) ct.StopContourLine(contourSaveList);

    //turn off all the sections
    for (int j = 0; j < tool.numOfSections; j++)
    {
        tool.section[j].sectionOnOffCycle = false;
        tool.section[j].sectionOffRequest = false;
    }

    //turn off patching
    for (int j = 0; j < tool.triStrip.count(); j++)
    {
        if (tool.triStrip[j].isDrawing)
            tool.triStrip[j].TurnMappingOff(tool.secColors[j],
                                       tool.section[tool.triStrip[j].currentStartSectionNum].leftPoint,
                                       tool.section[tool.triStrip[j].currentEndSectionNum].rightPoint,
                                       tool.patchSaveList);
    }
    lock.unlock();
    qDebug() << "Test4";

    //FileSaveHeadland(;
    qDebug() << "Starting FileSaveBoundary()";
    FileSaveBoundary();
    qDebug() << "Starting FileSaveSections()";
    FileSaveSections();
    qDebug() << "Starting FileSaveContour()";
    FileSaveContour();
    qDebug() << "Starting FileSaveTracks()";
    FileSaveTracks();
    qDebug() << "Starting FileSaveFlags()";
    FileSaveFlags();
    qDebug() << "Starting ExportFieldAs_KML()";
    ExportFieldAs_KML();
    qDebug() << "All file operations completed";
    //ExportFieldAs_ISOXMLv3()
    //ExportFieldAs_ISOXMLv4()

    // Save vehicle settings AFTER all field operations complete (conditional)
    // Include applicationClosing property in save decision (Qt 6.8 Rectangle Pattern)
    bool shouldSaveVehicle = saveVehicle || Backend::instance()->applicationClosing();
    qDebug() << "Before vehicle_saveas check, saveVehicle=" << saveVehicle << "applicationClosing=" << Backend::instance()->applicationClosing() << "shouldSaveVehicle=" << shouldSaveVehicle;
    if(shouldSaveVehicle && SettingsManager::instance()->vehicle_vehicleName() != "Default Vehicle") {
        QString vehicleName = SettingsManager::instance()->vehicle_vehicleName();
        qDebug() << "Scheduling async vehicle_saveas():" << vehicleName;

        // ASYNC SOLUTION: Defer vehicle_saveas to avoid mutex deadlock during field close
        QTimer::singleShot(100, this, [this, vehicleName]() {
            qDebug() << "Executing async vehicle_saveas():" << vehicleName;
            vehicle_saveas(vehicleName);
            qDebug() << "Async vehicle_saveas() completed";
        });
    } else {
        qDebug() << "Skipping vehicle_saveas (saveVehicle=" << saveVehicle << "applicationClosing=" << Backend::instance()->applicationClosing() << "shouldSaveVehicle=" << shouldSaveVehicle << ")";
    }

    qDebug() << "Before field cleanup";
    //property_setF_CurrentDir = tr("None";
    //currentFieldDirectory = (QString)property_setF_CurrentDir;
    displayFieldName = tr("None");

    qDebug() << "Before JobClose()";
    JobClose();
    qDebug() << "JobClose() completed";
    //Text = "AgOpenGPS";
    qDebug() << "Test5";
}

// AgIO Service Setup Methods
void FormGPS::setupAgIOService()
{
    qDebug() << "Setting up AgIO service (main thread)...";

    // Phase 6.0.21: Get AgIOService singleton instance for signal connection
    m_agioService = AgIOService::instance();

    qDebug() << "AgIOService singleton accessed - ready for parsedDataReady signal connection";
}

void FormGPS::connectToAgIOFactoryInstance()
{
    qDebug() << "ðŸ”— Connecting to AgIOService factory instance...";
    
    // Get the factory-created singleton instance
    m_agioService = AgIOService::instance();
    
    if (m_agioService) {
        qDebug() << "âœ… Connected to AgIOService singleton instance";
        
        // Now connect the Phase 4.2 pipeline: AgIOService â†’ pn â†’ vehicle â†’ OpenGL
        connectFormLoopToAgIOService();
        
        qDebug() << "ðŸ”— Phase 4.2: AgIOService â†’ pn â†’ vehicle pipeline established";
    } else {
        qDebug() << "âŒ ERROR: AgIOService singleton not found";
    }
}

void FormGPS::testAgIOConfiguration()
{
    qDebug() << "\n=================================";
    qDebug() << "ðŸ“‹ AgIO Configuration Test";
    qDebug() << "=================================";
    
    QSettings settings("QtAgOpenGPS", "QtAgOpenGPS");
    qDebug() << "ðŸ“ Settings file:" << settings.fileName();
    
    // Display NTRIP settings
    qDebug() << "\nðŸŒ NTRIP Configuration:";
    qDebug() << "  URL:" << settings.value("comm/ntripURL", "").toString();
    qDebug() << "  Mount:" << settings.value("comm/ntripMount", "").toString();
    qDebug() << "  Port:" << settings.value("comm/ntripCasterPort", 2101).toInt();
    qDebug() << "  Enabled:" << settings.value("comm/ntripIsOn", false).toBool();
    qDebug() << "  User:" << (settings.value("comm/ntripUserName", "").toString().isEmpty() ? "none" : "configured");
    
    // Display UDP settings
    qDebug() << "\nðŸ“¡ UDP Configuration:";
    int ip1 = settings.value("comm/udpIP1", 192).toInt();
    int ip2 = settings.value("comm/udpIP2", 168).toInt();
    int ip3 = settings.value("comm/udpIP3", 5).toInt();
    qDebug() << "  Subnet:" << QString("%1.%2.%3.xxx").arg(ip1).arg(ip2).arg(ip3);
    qDebug() << "  Broadcast:" << QString("%1.%2.%3.255").arg(ip1).arg(ip2).arg(ip3);
    qDebug() << "  Listen Port:" << settings.value("comm/udpListenPort", 9999).toInt();
    qDebug() << "  Send Port:" << settings.value("comm/udpSendPort", 8888).toInt();
    
    qDebug() << "\nðŸ” Expected Data Sources:";
    qDebug() << "  1. GPS via UDP on port 9999 (NMEA sentences)";
    qDebug() << "  2. GPS via Serial port (if configured)";
    qDebug() << "  3. NTRIP corrections from" << settings.value("comm/ntripURL", "").toString();
    qDebug() << "  4. AgOpenGPS modules on" << QString("%1.%2.%3.255").arg(ip1).arg(ip2).arg(ip3);
    qDebug() << "=================================\n";
}

void FormGPS::connectFormLoopToAgIOService()
{
    qDebug() << "ðŸ”— Phase 4.2: Connecting AgIOService â†’ pn â†’ vehicle ...";
    
    if (!m_agioService) {
        qDebug() << "âŒ Cannot connect: AgIOService is null";
        return;
    }
    
    // PHASE 4.2: Direct connection AgIOService â†’ pn â†’ vehicle
    // This replaces FormLoop progressively as per architecture document
    
    // ✅ RECTANGLE PATTERN PURE: Direct access via updateGPSData() method
    // GPS data automatically synchronized via property bindings and main timer
    // No connect() needed - updateGPSData() called directly when needed
    
    // Phase 6.0.21: IMU initialization removed - GPS/IMU data now comes via gpsDataReceived signal
    // Data flow: AgIOService broadcasts via signal -> FormGPS stores -> ahrs updated in UpdateFixPosition()
    // ahrs.imuRoll, ahrs.imuPitch, ahrs.imuHeading initialized from signal data

    qDebug() << "OK AgIOService -> FormGPS signal pipeline established";
    qDebug() << "  Data flow: AgIOService (broadcast) -> FormGPS (storage) -> vehicle -> OpenGL";
    qDebug() << "  Phase 6.0.21: GPS/IMU data via gpsDataReceived signal";
}

void FormGPS::cleanupAgIOService()
{
    qDebug() << "ðŸ”§ Cleaning up AgIO service...";

    if (m_agioService) {
        m_agioService->shutdown();
        // Note: Don't delete m_agioService as it's managed by QML singleton system
        m_agioService = nullptr;
        qDebug() << "âœ… AgIO service cleaned up";
    }
}

// Qt 6.8: All complex property binding methods removed
// Using simple objectCreated signal instead

