// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// This runs every time we get a new GPS fix, or sim position
#include "formgps.h"
#include "cnmea.h"
#include "modulecomm.h"
#include "ccontour.h"
#include "cvehicle.h"
#include "csection.h"
#include "cboundary.h"
#include "ctrack.h"
#include "settingsmanager.h"
#include <QQuickView>
#include <QOpenGLContext>
#include <QPair>
#include <QElapsedTimer>
#include <QLabel>
#include <QPainter>
#include "glm.h"
#include "aogrenderer.h"
#include "cpgn.h"
#include "qmlutil.h"
#include "glutils.h"
#include "rendering.h"
#include "backend.h"
#include "mainwindowstate.h"
#include "boundaryinterface.h"
#include "recordedpath.h"
#include "siminterface.h"
#include "modulecomm.h"
#include "cpgn.h"
#include "blockage.h"
#include "ratecontrol.h"
#include "tools.h"
#include "steerconfig.h"
#include "backendaccess.h"
#include "camera.h"
#include "vehicleproperties.h"
#include "sectionproperties.h"
#include "layerservice.h"
#include <QtConcurrent/QtConcurrentRun>


Q_LOGGING_CATEGORY (qpos, "formgps_position.qtagopengps")

extern QLabel *grnPixelsWindow;
extern QLabel *overlapPixelsWindow;

inline QColor QColorWithAlpha(const QColor &c, int a) {
    return QColor(c.red(), c.green(), c.blue(), a);
}

//called for every new GPS or simulator position
void FormGPS::UpdateFixPosition()
{
    QLocale locale;

    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_TRACK(track);  //bring in a reference "track"
    BACKEND_YT(yt); //bring in a reference "yt"
    Camera &camera = *Camera::instance();

    // PHASE 6.0.33: Declare rawGpsPosition at function start (before goto labels)
    // Used to separate RAW GPS positions (for heading calc) from CORRECTED positions (for display)
    // Now copies from m_rawGpsPosition member (set by onNmeaDataReady at 8 Hz)
    Vec2 rawGpsPosition;

    CPGN_FE &p_254 = ModuleComm::instance()->p_254;

    //swFrame.Stop();
    //Measure the frequency of the GPS updates
    //timeSliceOfLastFix = (double)(swFrame.elapsed()) / 1000;
    qDebug(qpos) << "swFrame time at new frame: " << swFrame.elapsed();
    //lock.lockForWrite(); //stop GL from updating while we calculate a new position

    // Phase 6.0.21: Calculate Hz from CPU timer (AgIOService.nowHz/gpsHz removed)
    // GPS frequency is calculated from frame timing
    nowHz = 1000.0 / swFrame.elapsed(); //convert ms into hz

    // Phase 6.0.20 Task 24 Step 5.6: Remove artificial 20 Hz ceiling in simulation mode
    // - Simulation mode: Show real CPU performance (can be 50+ Hz)
    // - Real mode fallback: Apply limits only if GPS disconnected
    // if (!SettingsManager::instance()->menu_isSimulatorOn()) {
    //     // Real mode fallback protection (GPS disconnected scenario)
    //     if (nowHz > 50) nowHz = 50;
    //     if (nowHz < 3) nowHz = 3;
    // }
    // Simulation mode: No limits, show true CPU performance

    double mc_actualSteerAngleDegrees = ModuleComm::instance()->actualSteerAngleDegrees();

    //simple comp filter
    gpsHz = 0.98 * gpsHz + 0.02 * nowHz;

    //Initialization counter
    startCounter++;

    if (!isGPSPositionInitialized)
    {
        InitializeFirstFewGPSPositions();
        //lock.unlock();
        return;
    }

    //qDebug(qpos) << "Easting " <<  pn.fix.easting << "Northing" <<  pn.fix.northing << "Time " << swFrame.elapsed() << nowHz;

    swFrame.restart();

    pn.speed = pn.vtgSpeed;
    CVehicle::instance()->AverageTheSpeed(pn.speed);

    /*
    //GPS is valid, let's bootstrap the demo field if needed
    if(bootstrap_field)
    {
        fileCreateField();
        fileSaveABLines();
        bootstrap_field = false;
    }
    */

    //#region Heading
    //calculate current heading only when moving, otherwise use last
    if (headingFromSource == "Fix")
    {
        //#region Start

        distanceCurrentStepFixDisplay = glm::Distance(prevDistFix, pn.fix);
        double newDistance = Backend::instance()->m_currentField.distanceUser + distanceCurrentStepFixDisplay;
        if (newDistance > 999) newDistance = 0;
        Backend::instance()->currentField_setDistanceUser(newDistance);
        distanceCurrentStepFixDisplay *= 100;

        prevDistFix = pn.fix;

        if (fabs(CVehicle::instance()->avgSpeed()) < 1.5 && !CVehicle::instance()->vehicleProperties()->firstHeadingSet())
            goto byPass;

        if (!CVehicle::instance()->vehicleProperties()->firstHeadingSet()) //set in steer settings, Stanley
        {
            prevFix.easting = stepFixPts[0].easting; prevFix.northing = stepFixPts[0].northing;

            if (stepFixPts[2].isSet == 0)
            {
                //this is the first position no roll or offset correction
                if (stepFixPts[0].isSet == 0)
                {
                    stepFixPts[0].easting = pn.fix.easting;
                    stepFixPts[0].northing = pn.fix.northing;
                    stepFixPts[0].isSet = 1;
                    //lock.unlock();
                    return;
                }

                //and the second
                if (stepFixPts[1].isSet == 0)
                {
                    for (int i = totalFixSteps - 1; i > 0; i--) stepFixPts[i] = stepFixPts[i - 1];
                    stepFixPts[0].easting = pn.fix.easting;
                    stepFixPts[0].northing = pn.fix.northing;
                    stepFixPts[0].isSet = 1;
                    //lock.unlock();
                    return;
                }

                //the critcal moment for checking initial direction/heading.
                for (int i = totalFixSteps - 1; i > 0; i--) stepFixPts[i] = stepFixPts[i - 1];
                stepFixPts[0].easting = pn.fix.easting;
                stepFixPts[0].northing = pn.fix.northing;
                stepFixPts[0].isSet = 1;

                Backend::instance()->m_fixFrame.gpsHeading = atan2(pn.fix.easting - stepFixPts[2].easting,
                                    pn.fix.northing - stepFixPts[2].northing);

                if (Backend::instance()->m_fixFrame.gpsHeading < 0) Backend::instance()->m_fixFrame.gpsHeading = Backend::instance()->m_fixFrame.gpsHeading + glm::twoPI;
                else if (Backend::instance()->m_fixFrame.gpsHeading > glm::twoPI) Backend::instance()->m_fixFrame.gpsHeading = Backend::instance()->m_fixFrame.gpsHeading - glm::twoPI;

                CVehicle::instance()->set_fixHeading ( Backend::instance()->m_fixFrame.gpsHeading );

                //set the imu to gps heading offset
                if (ahrs.imuHeading != 99999)
                {
                    double imuHeading = (glm::toRadians(ahrs.imuHeading));
                    imuGPS_Offset = 0;

                    //Difference between the IMU heading and the GPS heading
                    double gyroDelta = (imuHeading + imuGPS_Offset) - Backend::instance()->m_fixFrame.gpsHeading;

                    if (gyroDelta < 0) gyroDelta += glm::twoPI;
                    else if (gyroDelta > glm::twoPI) gyroDelta -= glm::twoPI;

                    //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
                    if (gyroDelta >= -glm::PIBy2 && gyroDelta <= glm::PIBy2) gyroDelta *= -1.0;
                    else
                    {
                        if (gyroDelta > glm::PIBy2) { gyroDelta = glm::twoPI - gyroDelta; }
                        else { gyroDelta = (glm::twoPI + gyroDelta) * -1.0; }
                    }
                    if (gyroDelta > glm::twoPI) gyroDelta -= glm::twoPI;
                    else if (gyroDelta < -glm::twoPI) gyroDelta += glm::twoPI;

                    //moe the offset to line up imu with gps
                    imuGPS_Offset = (gyroDelta);
                    //rounding a floating point number doesn't make sense.
                    //imuGPS_Offset = Math.Round(imuGPS_Offset, 6);

                    if (imuGPS_Offset >= glm::twoPI) imuGPS_Offset -= glm::twoPI;
                    else if (imuGPS_Offset <= 0) imuGPS_Offset += glm::twoPI;

                    //determine the Corrected heading based on gyro and GPS
                    _imuCorrected = imuHeading + imuGPS_Offset;
                    if (_imuCorrected > glm::twoPI) _imuCorrected -= glm::twoPI;
                    else if (_imuCorrected < 0) _imuCorrected += glm::twoPI;

                    // Phase 6.0.24 Problem 18: Validate _imuCorrected before assigning to fixHeading
                    if (std::isfinite(_imuCorrected) && fabs(_imuCorrected) < 100.0) {
                        CVehicle::instance()->set_fixHeading ( _imuCorrected );
                    } else {
                        qWarning() << "Invalid _imuCorrected value:" << _imuCorrected << "- not assigned to fixHeading";
                    }
                }

                //set the camera
                camera.set_camHeading(glm::toDegrees(Backend::instance()->m_fixFrame.gpsHeading));

                //now we have a heading, fix the first 3
                if (CVehicle::instance()->antennaOffset != 0)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        stepFixPts[i].easting = (cos(-Backend::instance()->m_fixFrame.gpsHeading) * CVehicle::instance()->antennaOffset) + stepFixPts[i].easting;
                        stepFixPts[i].northing = (sin(-Backend::instance()->m_fixFrame.gpsHeading) * CVehicle::instance()->antennaOffset) + stepFixPts[i].northing;
                    }
                }

                if (ahrs.imuRoll != 88888)
                {
                    // PHASE 6.0.31: Fixed tan() → sin() for geometric correctness
                    // Roll correction is horizontal displacement = height × sin(roll), NOT tan(roll)
                    // tan() causes exponential error for large angles (15.5% error at 30°)
                    rollCorrectionDistance = sin(glm::toRadians((ahrs.imuRoll))) * -CVehicle::instance()->antennaHeight;

                    // roll to left is positive  **** important!!
                    // not any more - April 30, 2019 - roll to right is positive Now! Still Important
                    for (int i = 0; i < 3; i++)
                    {
                        stepFixPts[i].easting = (cos(-Backend::instance()->m_fixFrame.gpsHeading) * rollCorrectionDistance) + stepFixPts[i].easting;
                        stepFixPts[i].northing = (sin(-Backend::instance()->m_fixFrame.gpsHeading) * rollCorrectionDistance) + stepFixPts[i].northing;
                    }
                }

                //get the distance from first to 2nd point, update fix with new offset/roll
                stepFixPts[0].distance = glm::Distance(stepFixPts[1], stepFixPts[0]);
                pn.fix.easting = stepFixPts[0].easting;
                pn.fix.northing = stepFixPts[0].northing;

                CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(true);
                TimedMessageBox(2000, "Direction Reset", "Forward is Set");

                lastGPS = pn.fix;

                //lock.unlock();
                return;
            }
        }
        //#endregion

        //#region Offset Roll

        // PHASE 6.0.33 FIX: Copy RAW GPS position from member FIRST (prevents cascade corrections)
        // m_rawGpsPosition set by onNmeaDataReady() at 8 Hz (never modified by corrections)
        // UpdateFixPosition() called at 50 Hz → always starts from same raw position
        {
            QMutexLocker lock(&m_rawGpsPositionMutex);
            rawGpsPosition = m_rawGpsPosition;
        }

        // PHASE 6.0.33 FIX: RESET pn.fix to RAW position BEFORE applying corrections
        // This ensures corrections are applied to FRESH base position, not accumulated
        pn.fix.easting = rawGpsPosition.easting;
        pn.fix.northing = rawGpsPosition.northing;

        // Apply antenna offset correction
        if (CVehicle::instance()->antennaOffset != 0)
        {
            pn.fix.easting = (cos(-Backend::instance()->m_fixFrame.gpsHeading) * CVehicle::instance()->antennaOffset) + pn.fix.easting;
            pn.fix.northing = (sin(-Backend::instance()->m_fixFrame.gpsHeading) * CVehicle::instance()->antennaOffset) + pn.fix.northing;
        }

        uncorrectedEastingGraph = pn.fix.easting;

        // Apply roll correction
        if (ahrs.imuRoll != 88888)
        {
            //change for roll to the right is positive times -1
            rollCorrectionDistance = sin(glm::toRadians((ahrs.imuRoll))) * -CVehicle::instance()->antennaHeight;
            correctionDistanceGraph = rollCorrectionDistance;

            pn.fix.easting = (cos(-Backend::instance()->m_fixFrame.gpsHeading) * rollCorrectionDistance) + pn.fix.easting;
            pn.fix.northing = (sin(-Backend::instance()->m_fixFrame.gpsHeading) * rollCorrectionDistance) + pn.fix.northing;
        }

        //#endregion

        //#region Fix Heading

        double minFixHeadingDistSquared;
        double newGPSHeading;
        double imuHeading;
        double camDelta;
        double gyroDelta;

        //imu on board
        if (ahrs.imuHeading != 99999)
        {
            //check for out-of bounds fusion weights in case config
            //file was edited and changed inappropriately.
            //TODO move this sort of thing to FormGPS::load_settings
            if (ahrs.fusionWeight > 0.4) ahrs.fusionWeight = 0.4;
            if (ahrs.fusionWeight < 0.2) ahrs.fusionWeight = 0.2;

            // PHASE 6.0.35: Always calculate IMU heading (even during GPS bypass)
            // This ensures fixHeading is updated continuously via IMU, preventing wheel shake
            imuHeading = (glm::toRadians(ahrs.imuHeading));

            //how far since last fix
            distanceCurrentStepFix = glm::Distance(stepFixPts[0], pn.fix);

            // PHASE 6.0.35: Recalculate GPS heading ONLY if distance sufficient
            // C# original: GPS heading update is conditional, but IMU fusion ALWAYS runs
            if (distanceCurrentStepFix >= gpsMinimumStepDistance)
            {
                //userDistance can be reset

                minFixHeadingDistSquared = minHeadingStepDist * minHeadingStepDist;
                fixToFixHeadingDistance = 0;

                for (int i = 0; i < totalFixSteps; i++)
                {
                    fixToFixHeadingDistance = glm::DistanceSquared(stepFixPts[i], pn.fix);
                    currentStepFix = i;

                    if (fixToFixHeadingDistance > minFixHeadingDistSquared)
                    {
                        break;
                    }
                }

                if (fixToFixHeadingDistance >= (minFixHeadingDistSquared * 0.5))
                {
                    newGPSHeading = atan2(pn.fix.easting - stepFixPts[currentStepFix].easting,
                                          pn.fix.northing - stepFixPts[currentStepFix].northing);
                    if (newGPSHeading < 0) newGPSHeading += glm::twoPI;

                    if (ahrs.isReverseOn)
                    {
                        ////what is angle between the last valid heading before stopping and one just now
                        delta = fabs(M_PI - fabs(fabs(newGPSHeading - _imuCorrected) - M_PI));

                        //ie change in direction
                        if (delta > 1.57) //
                        {
                            CVehicle::instance()->setIsReverse(true);
                            newGPSHeading += M_PI;
                            if (newGPSHeading < 0) newGPSHeading += glm::twoPI;
                            else if (newGPSHeading >= glm::twoPI) newGPSHeading -= glm::twoPI;
                            Backend::instance()->set_isReverseWithIMU(true);
                        }
                        else
                        {
                            CVehicle::instance()->setIsReverse(false);
                            Backend::instance()->set_isReverseWithIMU(false);
                        }
                    }
                    else
                    {
                        CVehicle::instance()->setIsReverse(false);
                    }

                    // PHASE 6.0.35: Wheel angle compensation (forwardComp/reverseComp already implemented)
                    if (CVehicle::instance()->isReverse())
                        newGPSHeading -= glm::toRadians(CVehicle::instance()->antennaPivot / 1
                                                        * mc_actualSteerAngleDegrees * ahrs.reverseComp);
                    else
                        newGPSHeading -= glm::toRadians(CVehicle::instance()->antennaPivot / 1
                                                        * mc_actualSteerAngleDegrees * ahrs.forwardComp);

                    if (newGPSHeading < 0) newGPSHeading += glm::twoPI;
                    else if (newGPSHeading >= glm::twoPI) newGPSHeading -= glm::twoPI;

                    Backend::instance()->m_fixFrame.gpsHeading = newGPSHeading;

                    // PHASE 6.0.35 FIX: Update stepFixPts ONLY when GPS heading recalculated
                    // This ensures stepFixPts[0] and pn.fix remain spaced apart (critical for low speed)
                    // C# original: stepFixPts updated BEFORE byPass label, not after
                    for (int i = totalFixSteps - 1; i > 0; i--) stepFixPts[i] = stepFixPts[i - 1];
                    stepFixPts[0].easting = rawGpsPosition.easting;
                    stepFixPts[0].northing = rawGpsPosition.northing;
                    stepFixPts[0].isSet = 1;

                    //#region IMU Fusion - Update GPS->IMU offset

                    // IMU Fusion with heading correction, add the correction
                    //Difference between the IMU heading and the GPS heading
                    gyroDelta = 0;

                    //if (!Backend::instance()->isReverseWithIMU)
                    gyroDelta = (imuHeading + imuGPS_Offset) - Backend::instance()->m_fixFrame.gpsHeading;
                    //else
                    //{
                    //    gyroDelta = 0;
                    //}

                    if (gyroDelta < 0) gyroDelta += glm::twoPI;
                    else if (gyroDelta > glm::twoPI) gyroDelta -= glm::twoPI;

                    //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
                    if (gyroDelta >= -glm::PIBy2 && gyroDelta <= glm::PIBy2) gyroDelta *= -1.0;
                    else
                    {
                        if (gyroDelta > glm::PIBy2) { gyroDelta = glm::twoPI - gyroDelta; }
                        else { gyroDelta = (glm::twoPI + gyroDelta) * -1.0; }
                    }
                    if (gyroDelta > glm::twoPI) gyroDelta -= glm::twoPI;
                    else if (gyroDelta < -glm::twoPI) gyroDelta += glm::twoPI;

                    //move the offset to line up imu with gps
                    if(!Backend::instance()->isReverseWithIMU())
                        imuGPS_Offset += (gyroDelta * (ahrs.fusionWeight));
                    else
                        imuGPS_Offset += (gyroDelta * (0.02));

                    if (imuGPS_Offset > glm::twoPI) imuGPS_Offset -= glm::twoPI;
                    else if (imuGPS_Offset < 0) imuGPS_Offset += glm::twoPI;

                    //#endregion
                }
                // ELSE: fixToFixHeadingDistance too small -> keep existing gpsHeading and imuGPS_Offset
            }
            // ELSE: distanceCurrentStepFix too small -> keep existing gpsHeading and imuGPS_Offset

            // PHASE 6.0.35: ALWAYS calculate imuCorrected and update fixHeading (even during GPS bypass)
            // This is THE FIX: fixHeading updated at IMU rate (10 Hz) even when GPS heading stale
            // Result: No wheel shake at startup, no rotation jump when movement begins
            //determine the Corrected heading based on gyro and GPS
            _imuCorrected = imuHeading + imuGPS_Offset;
            if (_imuCorrected > glm::twoPI) _imuCorrected -= glm::twoPI;
            else if (_imuCorrected < 0) _imuCorrected += glm::twoPI;

            //use imu as heading when going slow
            // Phase 6.0.24 Problem 18: Validate _imuCorrected before assigning to fixHeading
            if (std::isfinite(_imuCorrected) && fabs(_imuCorrected) < 100.0) {
                CVehicle::instance()->set_fixHeading ( _imuCorrected );
            } else {
                qWarning() << "Invalid _imuCorrected value:" << _imuCorrected << "- not assigned to fixHeading";
            }

            //#endregion
        }
        else
        {
            //how far since last fix
            distanceCurrentStepFix = glm::Distance(stepFixPts[0], pn.fix);

            // PHASE 6.0.35: Recalculate GPS heading ONLY if distance sufficient
            // No IMU available, so fixHeading = gpsHeading (updated only when GPS moves enough)
            if (distanceCurrentStepFix >= gpsMinimumStepDistance)
            {
                minFixHeadingDistSquared = minHeadingStepDist * minHeadingStepDist;
                fixToFixHeadingDistance = 0;

                for (int i = 0; i < totalFixSteps; i++)
                {
                    // PHASE 6.0.32: Use RAW position for distance check (consistent with heading calc)
                    fixToFixHeadingDistance = glm::DistanceSquared(stepFixPts[i], rawGpsPosition);
                    currentStepFix = i;

                    if (fixToFixHeadingDistance > minFixHeadingDistSquared)
                    {
                        break;
                    }
                }

                if (fixToFixHeadingDistance >= minFixHeadingDistSquared * 0.5)
                {
                    // PHASE 6.0.32: Calculate heading from RAW GPS positions (not corrected)
                    // Old code used pn.fix which contains CORRECTED position → heading affected by roll
                    newGPSHeading = atan2(rawGpsPosition.easting - stepFixPts[currentStepFix].easting,
                                          rawGpsPosition.northing - stepFixPts[currentStepFix].northing);
                    if (newGPSHeading < 0) newGPSHeading += glm::twoPI;

                    if (ahrs.isReverseOn)
                    {

                        ////what is angle between the last valid heading before stopping and one just now
                        delta = fabs(M_PI - fabs(fabs(newGPSHeading - Backend::instance()->m_fixFrame.gpsHeading) - M_PI));

                        filteredDelta = delta * 0.2 + filteredDelta * 0.8;

                        //filtered delta different then delta
                        if (fabs(filteredDelta - delta) > 0.5)
                        {
                            CVehicle::instance()->setIsChangingDirection(true);
                        }
                        else
                        {
                            CVehicle::instance()->setIsChangingDirection(false);
                        }

                        //we can't be sure if changing direction so do nothing
                        if (CVehicle::instance()->isChangingDirection())
                        {
                            // Skip heading update when changing direction (unstable)
                        }
                        else
                        {
                            //ie change in direction
                            if (filteredDelta > 1.57) //
                            {
                                CVehicle::instance()->setIsReverse(true);
                                newGPSHeading += M_PI;
                                if (newGPSHeading < 0) newGPSHeading += glm::twoPI;
                                else if (newGPSHeading >= glm::twoPI) newGPSHeading -= glm::twoPI;
                            }
                            else
                                CVehicle::instance()->setIsReverse(false);

                            // PHASE 6.0.35: Wheel angle compensation (forwardComp/reverseComp already implemented)
                            if (CVehicle::instance()->isReverse())
                                newGPSHeading -= glm::toRadians(CVehicle::instance()->antennaPivot / 1
                                                                * mc_actualSteerAngleDegrees * ahrs.reverseComp);
                            else
                                newGPSHeading -= glm::toRadians(CVehicle::instance()->antennaPivot / 1
                                                                * mc_actualSteerAngleDegrees * ahrs.forwardComp);

                            if (newGPSHeading < 0) newGPSHeading += glm::twoPI;
                            else if (newGPSHeading >= glm::twoPI) newGPSHeading -= glm::twoPI;

                            //set the headings
                            Backend::instance()->m_fixFrame.gpsHeading = newGPSHeading;
                            CVehicle::instance()->set_fixHeading ( Backend::instance()->m_fixFrame.gpsHeading );

                            // PHASE 6.0.35 FIX: Update stepFixPts when heading recalculated (No IMU reverse path)
                            for (int i = totalFixSteps - 1; i > 0; i--) stepFixPts[i] = stepFixPts[i - 1];
                            stepFixPts[0].easting = rawGpsPosition.easting;
                            stepFixPts[0].northing = rawGpsPosition.northing;
                            stepFixPts[0].isSet = 1;
                        }
                    }

                    else
                    {
                        CVehicle::instance()->setIsReverse(false);

                        // PHASE 6.0.35 FIX: Apply wheel angle compensation in forward mode too!
                        // Bug: compensation was only applied in reverse detection branch
                        newGPSHeading -= glm::toRadians(CVehicle::instance()->antennaPivot / 1
                                                        * mc_actualSteerAngleDegrees * ahrs.forwardComp);

                        if (newGPSHeading < 0) newGPSHeading += glm::twoPI;
                        else if (newGPSHeading >= glm::twoPI) newGPSHeading -= glm::twoPI;

                        //set the headings
                        Backend::instance()->m_fixFrame.gpsHeading = newGPSHeading;
                        CVehicle::instance()->set_fixHeading ( Backend::instance()->m_fixFrame.gpsHeading );

                        // PHASE 6.0.35 FIX: Update stepFixPts when heading recalculated (No IMU forward path)
                        for (int i = totalFixSteps - 1; i > 0; i--) stepFixPts[i] = stepFixPts[i - 1];
                        stepFixPts[0].easting = rawGpsPosition.easting;
                        stepFixPts[0].northing = rawGpsPosition.northing;
                        stepFixPts[0].isSet = 1;
                    }
                }
                // ELSE: fixToFixHeadingDistance too small -> keep existing gpsHeading and fixHeading
            }
            // ELSE: distanceCurrentStepFix too small -> keep existing gpsHeading and fixHeading
        }

        // PHASE 6.0.35 FIX: stepFixPts update moved BEFORE afterByPass label (see lines 355-361, 505-509, 523-527)
        // This ensures stepFixPts[0] only updated when heading recalculated → fixes low-speed heading update bug

        //#endregion

        //#region Camera

        camDelta = CVehicle::instance()->fixHeading() - smoothCamHeading;

        if (camDelta < 0) camDelta += glm::twoPI;
        else if (camDelta > glm::twoPI) camDelta -= glm::twoPI;

        //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
        if (camDelta >= -glm::PIBy2 && camDelta <= glm::PIBy2) camDelta *= -1.0;
        else
        {
            if (camDelta > glm::PIBy2) { camDelta = glm::twoPI - camDelta; }
            else { camDelta = (glm::twoPI + camDelta) * -1.0; }
        }
        if (camDelta > glm::twoPI) camDelta -= glm::twoPI;
        else if (camDelta < -glm::twoPI) camDelta += glm::twoPI;

        smoothCamHeading -= camDelta * Camera::camSmoothFactor();

        if (smoothCamHeading > glm::twoPI) smoothCamHeading -= glm::twoPI;
        else if (smoothCamHeading < -glm::twoPI) smoothCamHeading += glm::twoPI;

        camera.set_camHeading(glm::toDegrees(smoothCamHeading));

        // PHASE 6.0.35 FIX: Skip byPass in normal flow (heading with wheel compensation already calculated)
        // byPass should only execute when jumped to from line 98 (slow speed / no initial heading)
        // Without this goto, byPass overwrites fixHeading (with wheel comp) → causes crab motion!
        goto afterByPass;

        //#endregion


        //Calculate a million other things
    byPass:
        if (ahrs.imuHeading != 99999)
        {
            _imuCorrected = (glm::toRadians(ahrs.imuHeading)) + imuGPS_Offset;
            if (_imuCorrected > glm::twoPI) _imuCorrected -= glm::twoPI;
            else if (_imuCorrected < 0) _imuCorrected += glm::twoPI;

            //use imu as heading when going slow
            // Phase 6.0.24 Problem 18: Validate _imuCorrected before assigning to fixHeading
            if (std::isfinite(_imuCorrected) && fabs(_imuCorrected) < 100.0) {
                CVehicle::instance()->set_fixHeading ( _imuCorrected );
            } else {
                qWarning() << "Invalid _imuCorrected value:" << _imuCorrected << "- not assigned to fixHeading";
            }
        }

        camDelta = CVehicle::instance()->fixHeading() - smoothCamHeading;

        if (camDelta < 0) camDelta += glm::twoPI;
        else if (camDelta > glm::twoPI) camDelta -= glm::twoPI;

        //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
        if (camDelta >= -glm::PIBy2 && camDelta <= glm::PIBy2) camDelta *= -1.0;
        else
        {
            if (camDelta > glm::PIBy2) { camDelta = glm::twoPI - camDelta; }
            else { camDelta = (glm::twoPI + camDelta) * -1.0; }
        }
        if (camDelta > glm::twoPI) camDelta -= glm::twoPI;
        else if (camDelta < -glm::twoPI) camDelta += glm::twoPI;

        smoothCamHeading -= camDelta * Camera::camSmoothFactor();

        if (smoothCamHeading > glm::twoPI) smoothCamHeading -= glm::twoPI;
        else if (smoothCamHeading < -glm::twoPI) smoothCamHeading += glm::twoPI;

        camera.set_camHeading(glm::toDegrees(smoothCamHeading));

        afterByPass:
        TheRest();
    } else if (headingFromSource == "VTG")
    {
        CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(true);
        if (CVehicle::instance()->avgSpeed() > 1)
        {
            //use NMEA headings for camera and tractor graphic
            CVehicle::instance()->set_fixHeading ( glm::toRadians(pn.headingTrue) );
            camera.set_camHeading(pn.headingTrue);
            Backend::instance()->m_fixFrame.gpsHeading = CVehicle::instance()->fixHeading();
        }

        //grab the most current fix to last fix distance
        distanceCurrentStepFix = glm::Distance(pn.fix, prevFix);

        //#region Antenna Offset

        if (CVehicle::instance()->antennaOffset != 0)
        {
            pn.fix.easting = (cos(-CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaOffset) + pn.fix.easting;
            pn.fix.northing = (sin(-CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaOffset) + pn.fix.northing;
        }
        //#endregion

        uncorrectedEastingGraph = pn.fix.easting;

        //an IMU with heading correction, add the correction
        if (ahrs.imuHeading != 99999)
        {
            //current gyro angle in radians
            double correctionHeading = (glm::toRadians(ahrs.imuHeading));

            //Difference between the IMU heading and the GPS heading
            double gyroDelta = (correctionHeading + imuGPS_Offset) - Backend::instance()->m_fixFrame.gpsHeading;
            if (gyroDelta < 0) gyroDelta += glm::twoPI;

            //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
            if (gyroDelta >= -glm::PIBy2 && gyroDelta <= glm::PIBy2) gyroDelta *= -1.0;
            else
            {
                if (gyroDelta > glm::PIBy2) { gyroDelta = glm::twoPI - gyroDelta; }
                else { gyroDelta = (glm::twoPI + gyroDelta) * -1.0; }
            }
            if (gyroDelta > glm::twoPI) gyroDelta -= glm::twoPI;
            if (gyroDelta < -glm::twoPI) gyroDelta += glm::twoPI;

            //if the gyro and last corrected fix is < 10 degrees, super low pass for gps
            if (fabs(gyroDelta) < 0.18)
            {
                //a bit of delta and add to correction to current gyro
                imuGPS_Offset += (gyroDelta * (0.1));
                if (imuGPS_Offset > glm::twoPI) imuGPS_Offset -= glm::twoPI;
                if (imuGPS_Offset < -glm::twoPI) imuGPS_Offset += glm::twoPI;
            }
            else
            {
                //a bit of delta and add to correction to current gyro
                imuGPS_Offset += (gyroDelta * (0.2));
                if (imuGPS_Offset > glm::twoPI) imuGPS_Offset -= glm::twoPI;
                if (imuGPS_Offset < -glm::twoPI) imuGPS_Offset += glm::twoPI;
            }

            //determine the Corrected heading based on gyro and GPS
            _imuCorrected = correctionHeading + imuGPS_Offset;
            if (_imuCorrected > glm::twoPI) _imuCorrected -= glm::twoPI;
            if (_imuCorrected < 0) _imuCorrected += glm::twoPI;

            // Phase 6.0.24 Problem 18: Validate _imuCorrected before assigning to fixHeading
            if (std::isfinite(_imuCorrected) && fabs(_imuCorrected) < 100.0) {
                CVehicle::instance()->set_fixHeading ( _imuCorrected);
            } else {
                qWarning() << "Invalid _imuCorrected value:" << _imuCorrected << "- not assigned to fixHeading";
            }

            double new_camHeading = CVehicle::instance()->fixHeading();
            if (new_camHeading > glm::twoPI) new_camHeading -= glm::twoPI;
            camera.set_camHeading(glm::toDegrees(new_camHeading));
        }


        //#region Roll

        if (ahrs.imuRoll != 88888)
        {
            //change for roll to the right is positive times -1
            rollCorrectionDistance = sin(glm::toRadians((ahrs.imuRoll))) * -CVehicle::instance()->antennaHeight;
            correctionDistanceGraph = rollCorrectionDistance;

            // roll to left is positive  **** important!!
            // not any more - April 30, 2019 - roll to right is positive Now! Still Important
            pn.fix.easting = (cos(-CVehicle::instance()->fixHeading()) * rollCorrectionDistance) + pn.fix.easting;
            pn.fix.northing = (sin(-CVehicle::instance()->fixHeading()) * rollCorrectionDistance) + pn.fix.northing;
        }

        //#endregion Roll

        TheRest();

        //most recent fixes are now the prev ones
        prevFix.easting = pn.fix.easting; prevFix.northing = pn.fix.northing;

    } else if (headingFromSource == "Dual")
    {
        CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(true);
        //use Dual Antenna heading for camera and tractor graphic
        CVehicle::instance()->set_fixHeading ( glm::toRadians(pn.headingTrueDual) );
        Backend::instance()->m_fixFrame.gpsHeading = CVehicle::instance()->fixHeading();

        uncorrectedEastingGraph = pn.fix.easting;

        if (CVehicle::instance()->antennaOffset != 0)
        {
            pn.fix.easting = (cos(-CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaOffset) + pn.fix.easting;
            pn.fix.northing = (sin(-CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaOffset) + pn.fix.northing;
        }

        if (ahrs.imuRoll != 88888 && CVehicle::instance()->antennaHeight != 0)
        {

            //change for roll to the right is positive times -1
            rollCorrectionDistance = sin(glm::toRadians((ahrs.imuRoll))) * -CVehicle::instance()->antennaHeight;
            correctionDistanceGraph = rollCorrectionDistance;

            // PHASE 6.0.35 FIX: Use fixHeading (not gpsHeading) for geometric consistency
            pn.fix.easting = (cos(-CVehicle::instance()->fixHeading()) * rollCorrectionDistance) + pn.fix.easting;
            pn.fix.northing = (sin(-CVehicle::instance()->fixHeading()) * rollCorrectionDistance) + pn.fix.northing;
        }

        //grab the most current fix and save the distance from the last fix
        distanceCurrentStepFix = glm::Distance(pn.fix, prevDistFix);

        //userDistance can be reset
        double userDistance = Backend::instance()->m_currentField.distanceUser + distanceCurrentStepFix;
        if (userDistance > 999) userDistance = 0;
        Backend::instance()->currentField_setDistanceUser(userDistance);

        distanceCurrentStepFixDisplay = distanceCurrentStepFix * 100;
        prevDistFix = pn.fix;

        if (glm::DistanceSquared(lastReverseFix, pn.fix) > 0.20)
        {
            //most recent heading
            double newHeading = atan2(pn.fix.easting - lastReverseFix.easting,
                                      pn.fix.northing - lastReverseFix.northing);

            if (newHeading < 0) newHeading += glm::twoPI;


            //what is angle between the last reverse heading and current dual heading
            double delta = fabs(M_PI - fabs(fabs(newHeading - CVehicle::instance()->fixHeading()) - M_PI));

            //are we going backwards
            CVehicle::instance()->setIsReverse(delta > 2 ? true : false);

            //save for next meter check
            lastReverseFix = pn.fix;
        }

        double camDelta = CVehicle::instance()->fixHeading() - smoothCamHeading;

        if (camDelta < 0) camDelta += glm::twoPI;
        else if (camDelta > glm::twoPI) camDelta -= glm::twoPI;

        //calculate delta based on circular data problem 0 to 360 to 0, clamp to +- 2 Pi
        if (camDelta >= -glm::PIBy2 && camDelta <= glm::PIBy2) camDelta *= -1.0;
        else
        {
            if (camDelta > glm::PIBy2) { camDelta = glm::twoPI - camDelta; }
            else { camDelta = (glm::twoPI + camDelta) * -1.0; }
        }
        if (camDelta > glm::twoPI) camDelta -= glm::twoPI;
        else if (camDelta < -glm::twoPI) camDelta += glm::twoPI;

        smoothCamHeading -= camDelta * Camera::camSmoothFactor();

        if (smoothCamHeading > glm::twoPI) smoothCamHeading -= glm::twoPI;
        else if (smoothCamHeading < -glm::twoPI) smoothCamHeading += glm::twoPI;

        camera.set_camHeading(glm::toDegrees(smoothCamHeading));

        TheRest();
    }
    //else {
    //}

    if (CVehicle::instance()->fixHeading() >= glm::twoPI)
        CVehicle::instance()->set_fixHeading( CVehicle::instance()->fixHeading()  - glm::twoPI );

    //#endregion
//
    //#region Corrected Position for GPS_OUT
    //NOTE: Michael, I'm not sure about this entire region

    double rollCorrectedLat;
    double rollCorrectedLon;
    // Phase 6.3.1: Use PropertyWrapper for safe QObject access
    pn.ConvertLocalToWGS84(pn.fix.northing, pn.fix.easting, rollCorrectedLat, rollCorrectedLon);

    QByteArray pgnRollCorrectedLatLon(22, 0);

    pgnRollCorrectedLatLon[0] = 0x80;
    pgnRollCorrectedLatLon[1] = 0x81;
    pgnRollCorrectedLatLon[2] = 0x7F;
    pgnRollCorrectedLatLon[3] = 0x64;
    pgnRollCorrectedLatLon[4] = 16;

    std::memcpy(pgnRollCorrectedLatLon.data() + 5, &rollCorrectedLon, 8);
    std::memcpy(pgnRollCorrectedLatLon.data() + 13, &rollCorrectedLat, 8);

    // SendPgnToLoop(pgnRollCorrectedLatLon); // ❌ REMOVED - Phase 4.6: AgIOService Workers handle PGN
    // GPS position data now flows through: AgIOService → FormGPS → pn/vehicle → OpenGL

    //#endregion

    //#region AutoSteer

    //preset the values
    CVehicle::instance()->set_guidanceLineDistanceOff (32000);

    if (MainWindowState::instance()->isContourBtnOn())
    {
        ct.DistanceFromContourLine(MainWindowState::instance()->isBtnAutoSteerOn(), *CVehicle::instance(), yt, ahrs, pn, CVehicle::instance()->pivotAxlePos, CVehicle::instance()->steerAxlePos);
    }
    else
    {
        //auto track routine
        // PHASE 6.0.42.9: Fix auto-track condition (C# Position.designer.cs:826)
        // Added timer check to prevent rapid switching (max 1 switch/second)
        if (track.isAutoTrack() && !MainWindowState::instance()->isBtnAutoSteerOn() && track.autoTrack3SecTimer >= 1)
        {
            track.autoTrack3SecTimer = 0;  // Reset timer after switch

            track.SwitchToClosestRefTrack(CVehicle::instance()->steerAxlePos, *CVehicle::instance());
        }

        bool autoSteerState = MainWindowState::instance()->isBtnAutoSteerOn();
        track.BuildCurrentLine(CVehicle::instance()->pivotAxlePos,secondsSinceStart,autoSteerState,yt,*CVehicle::instance(),bnd,ahrs,gyd,pn);
    }

    // autosteer at full speed of updates

    //if the whole path driving driving process is green
    if (RecordedPath::instance()->isDrivingRecordedPath()) recPath.UpdatePosition(yt, MainWindowState::instance()->isBtnAutoSteerOn());

    // If Drive button off - normal autosteer
    if (!CVehicle::instance()->isInFreeDriveMode())
    {
        //fill up0 the appropriate arrays with new values
        p_254.pgn[CPGN_FE::speedHi] = (char)((int)(fabs(CVehicle::instance()->avgSpeed()) * 10.0) >> 8);
        p_254.pgn[CPGN_FE::speedLo] = (char)((int)(fabs(CVehicle::instance()->avgSpeed()) * 10.0));
        //mc.machineControlData[mc.cnSpeed] = mc.autoSteerData[mc.sdSpeed];

        //save distance for display
        lightbarDistance = CVehicle::instance()->guidanceLineDistanceOff();

        if (!MainWindowState::instance()->isBtnAutoSteerOn()) //32020 means auto steer is off
        {
            //NOTE: Is this supposed to be commented out?
            //CVehicle::instance()->set_guidanceLineDistanceOff (32020);
            p_254.pgn[CPGN_FE::status] = 0;  // PHASE 6.0.29: OFF → send 0 (match C# original)
        }

        else p_254.pgn[CPGN_FE::status] = 1;  // PHASE 6.0.29: ON → send 1 (match C# original)

        if (RecordedPath::instance()->isDrivingRecordedPath() || recPath.isFollowingDubinsToPath) p_254.pgn[CPGN_FE::status] = 1;  // PHASE 6.0.29: Force ON (match C# original)

        // PHASE 6.0.42.8: Auto-snap track to pivot when autosteer turns ON
        // C# original: OpenGL.Designer.cs:1858-1876
        // Behavior: When autosteer activates, automatically center track to current tractor position
        // This is a ONE-TIME snap (not continuous tracking) controlled by isAutoSnapped flag
        if (ModuleComm::instance()->steerSwitchHigh())
        {
            // Manual steer override active (switch on handlebar)
            // Reset auto-snap flag so it can snap again when autosteer re-enabled
            track.setIsAutoSnapped(false);
        }
        else if (MainWindowState::instance()->isBtnAutoSteerOn())
        {
            // Autosteer is ON → perform auto-snap if enabled and not already snapped
            if (track.isAutoSnapToPivot() && !track.isAutoSnapped())
            {
                track.SnapToPivot();           // Nudge track to align with current pivot position
                track.setIsAutoSnapped(true);  // Mark as snapped (prevents re-snap until reset)
            }
        }
        else
        {
            // Autosteer is OFF → reset auto-snap flag for next activation cycle
            track.setIsAutoSnapped(false);
        }

        //mc.autoSteerData[7] = unchecked((byte)(CVehicle::instance()->guidanceLineDistanceOff() >> 8));
        //mc.autoSteerData[8] = unchecked((byte)(CVehicle::instance()->guidanceLineDistanceOff()));

        //convert to cm from mm and divide by 2 - lightbar
        int distanceX2;
        //if (CVehicle::instance()->set_guidanceLineDistanceOff() == 32020 || CVehicle::instance()->guidanceLineDistanceOff() == 32000)
        if (!MainWindowState::instance()->isBtnAutoSteerOn() || CVehicle::instance()->guidanceLineDistanceOff() == 32000)
            distanceX2 = 255;

        else
        {
            distanceX2 = (int)(CVehicle::instance()->guidanceLineDistanceOff() * 0.05);

            if (distanceX2 < -127) distanceX2 = -127;
            else if (distanceX2 > 127) distanceX2 = 127;
            distanceX2 += 127;
        }

        p_254.pgn[CPGN_FE::lineDistance] = (char)distanceX2;

        if (!SimInterface::instance()->isRunning())
        {
            if (MainWindowState::instance()->isBtnAutoSteerOn() && CVehicle::instance()->avgSpeed() > SettingsManager::instance()->as_maxSteerSpeed())
            {
                MainWindowState::instance()->set_isBtnAutoSteerOn(false);
                if (isMetric)
                    TimedMessageBox(3000, tr("AutoSteer Disabled"), tr("Above Maximum Safe Steering Speed: ") + locale.toString(CVehicle::instance()->maxSteerSpeed, 'g', 1) + tr(" Kmh"));
                else
                    TimedMessageBox(3000, tr("AutoSteer Disabled"), tr("Above Maximum Safe Steering Speed: ") + locale.toString(CVehicle::instance()->maxSteerSpeed * 0.621371, 'g', 1) + tr(" MPH"));
            }

            if (MainWindowState::instance()->isBtnAutoSteerOn() && CVehicle::instance()->avgSpeed() < SettingsManager::instance()->as_minSteerSpeed())
            {
                minSteerSpeedTimer++;
                if (minSteerSpeedTimer > 80)
                {
                    MainWindowState::instance()->set_isBtnAutoSteerOn(false);
                    if (isMetric)
                        TimedMessageBox(3000, tr("AutoSteer Disabled"), tr("Below Minimum Safe Steering Speed: ") + locale.toString(CVehicle::instance()->minSteerSpeed, 'g', 1) + tr(" Kmh"));
                    else
                        TimedMessageBox(3000, tr("AutoSteer Disabled"), tr("Below Minimum Safe Steering Speed: ") + locale.toString(CVehicle::instance()->minSteerSpeed * 0.621371, 'g', 1) + tr(" MPH"));
                }
            }
            else
            {
                minSteerSpeedTimer = 0;
            }
        }

        double tanSteerAngle = tan(glm::toRadians(((double)(CVehicle::instance()->guidanceLineSteerAngle)) * 0.01));
        double tanActSteerAngle = tan(glm::toRadians(mc_actualSteerAngleDegrees));

        setAngVel = 0.277777 * CVehicle::instance()->avgSpeed() * tanSteerAngle / CVehicle::instance()->wheelbase;
        actAngVel = glm::toDegrees(0.277777 * CVehicle::instance()->avgSpeed() * tanActSteerAngle / CVehicle::instance()->wheelbase);


        isMaxAngularVelocity = false;
        //greater then settings rads/sec limit steer angle
        if (fabs(setAngVel) > CVehicle::instance()->maxAngularVelocity)
        {
            setAngVel = CVehicle::instance()->maxAngularVelocity;
            tanSteerAngle = 3.6 * setAngVel * CVehicle::instance()->wheelbase / CVehicle::instance()->avgSpeed();
            if (CVehicle::instance()->guidanceLineSteerAngle < 0)
                CVehicle::instance()->guidanceLineSteerAngle = (short)(glm::toDegrees(atan(tanSteerAngle)) * -100);
            else
                CVehicle::instance()->guidanceLineSteerAngle = (short)(glm::toDegrees(atan(tanSteerAngle)) * 100);
            isMaxAngularVelocity = true;
        }

        setAngVel = glm::toDegrees(setAngVel);

        p_254.pgn[CPGN_FE::steerAngleHi] = (char)(CVehicle::instance()->guidanceLineSteerAngle >> 8);
        p_254.pgn[CPGN_FE::steerAngleLo] = (char)(CVehicle::instance()->guidanceLineSteerAngle);

        if (CVehicle::instance()->isChangingDirection() && ahrs.imuHeading == 99999)
            p_254.pgn[CPGN_FE::status] = 0;  // PHASE 6.0.29: Changing direction → OFF (match C# original)

        //for now if backing up, turn off autosteer
        if (!isSteerInReverse)
        {
            if (CVehicle::instance()->isReverse()) p_254.pgn[CPGN_FE::status] = 0;  // PHASE 6.0.29: Reverse → OFF (match C# original)
        }
    }

    else //Drive button is on
    {
        //fill up the auto steer array with free drive values
        p_254.pgn[CPGN_FE::speedHi] = (char)((int)(80) >> 8);
        p_254.pgn[CPGN_FE::speedLo] = (char)((int)(80));

        //turn on status to operate
        p_254.pgn[CPGN_FE::status] = 1;  // PHASE 6.0.29: Free Drive ON (match C# original)

        //send the steer angle
        CVehicle::instance()->guidanceLineSteerAngle = (qint16)(CVehicle::instance()->driveFreeSteerAngle() * 100);

        p_254.pgn[CPGN_FE::steerAngleHi] = (char)(CVehicle::instance()->guidanceLineSteerAngle >> 8);
        p_254.pgn[CPGN_FE::steerAngleLo] = (char)(CVehicle::instance()->guidanceLineSteerAngle);


    }

    //out serial to autosteer module  //indivdual classes load the distance and heading deltas
    // SendPgnToLoop(p_254.pgn); // ❌ REMOVED - Phase 4.6: AgIOService Workers handle PGN
    // Phase 6.0.33: Send PGN 254 at 50 Hz (synchronized with timer frequency)
    // Status byte (line 795) controls module behavior: 0=OFF (no steering), 1=ON (steering)
    // Module always responds with PGN 253 feedback → enables wheel display in real-time
    // SAFE: isBtnAutoSteerOn = false at startup (formgps.cpp:27), never saved in settings
    if (m_agioService) {
        m_agioService->sendPgn(p_254.pgn);
    }

    // Smart WAS Calibration data collection
    if (SteerConfig::instance()->isCollectingData && abs(CVehicle::instance()->guidanceLineDistanceOff()) < 500) // Within 50cm of guidance line
    {
        // Convert guidanceLineSteerAngle from centidegrees to degrees and collect data
        SteerConfig::instance()->AddSteerAngleSample(CVehicle::instance()->guidanceLineSteerAngle * 0.01, abs(CVehicle::instance()->avgSpeed()));
    }

    //for average cross track error
    if (CVehicle::instance()->guidanceLineDistanceOff() < 29000)
    {
        crossTrackError = (int)((double)crossTrackError * 0.90 + fabs((double)CVehicle::instance()->guidanceLineDistanceOff()) * 0.1);
    }
    else
    {
        crossTrackError = 0;
    }

    //#region Youturn

    //if an outer boundary is set, then apply critical stop logic
    if (bnd.bndList.count() > 0)
    {
        //check if inside all fence
        if (!MainWindowState::instance()->isYouTurnBtnOn())
        {
            BoundaryInterface::instance()->set_isOutOfBounds(!bnd.IsPointInsideFenceArea(CVehicle::instance()->pivotAxlePos));
            // Qt 6.8 FIX: Removed redundant self-assignment that could cause binding loop
        }
        else //Youturn is on
        {
            bool isInTurnBounds = bnd.IsPointInsideTurnArea(CVehicle::instance()->pivotAxlePos) != -1;
            //Are we inside outer and outside inner all turn boundaries, no turn creation problems
            //if we are too much off track > 1.3m, kill the diagnostic creation, start again
            //if (!yt.isYouTurnTriggered)
            if (isInTurnBounds)
            {
                BoundaryInterface::instance()->set_isOutOfBounds(false);
                //now check to make sure we are not in an inner turn boundary - drive thru is ok
                if (yt.youTurnPhase != 10)
                {
                    if (crossTrackError > 1000)
                    {
                        yt.ResetCreatedYouTurn();
                    }
                    else
                    {
                        if (track.getMode() == TrackMode::AB)
                        {
                            yt.BuildABLineDubinsYouTurn(yt.isYouTurnRight,bnd,
                                                        track,secondsSinceStart);
                        }
                        else
                        {
                            yt.BuildCurveDubinsYouTurn(yt.isYouTurnRight, bnd,track,secondsSinceStart);
                        }
                    }

                    if (yt.uTurnStyle == 0 && yt.youTurnPhase == 10)
                    {
                        yt.SmoothYouTurn(6);
                    }
                    if (yt.isTurnCreationTooClose && !yt.turnTooCloseTrigger)
                    {
                        yt.turnTooCloseTrigger = true;
                        //if (sounds.isTurnSoundOn) sounds.sndUTurnTooClose.Play(); Implemented in QML
                    }
                }
                else if (yt.ytList.count() > 5)//wait to trigger the actual turn since its made and waiting
                {
                    //distance from current pivot to first point of youturn pattern
                    _distancePivotToTurnLine = glm::Distance(yt.ytList[5], CVehicle::instance()->pivotAxlePos);

                    //if ((_distancePivotToTurnLine <= 20.0) && (_distancePivotToTurnLine >= 18.0) && !yt.isYouTurnTriggered)

                    /* moved to QML
                    if (!sounds.isBoundAlarming)
                    {
                        if (sounds.isTurnSoundOn) sounds.sndBoundaryAlarm.Play();
                        sounds.isBoundAlarming = true;
                    }*/
                    //yt.YouTurnTrigger(track, *CVehicle::instance());
                    //if we are close enough to pattern, trigger.
                    if ((_distancePivotToTurnLine <= 1.0) && (_distancePivotToTurnLine >= 0) && !yt.isYouTurnTriggered)
                    {
                        yt.YouTurnTrigger(track);
                        //moved to QML
                        //sounds.isBoundAlarming = false;
                    }

                    //if (Backend::instance()->mainWindow->isBtnAutoSteerOn() && CVehicle::instance()->guidanceLineDistanceOff() > 300 && !yt.isYouTurnTriggered)
                    //{
                    //    yt.ResetCreatedYouTurn();
                    //}
                }
            }
            else
            {
                if (!yt.isYouTurnTriggered)
                {
                    yt.ResetCreatedYouTurn();
                    BoundaryInterface::instance()->set_isOutOfBounds(!bnd.IsPointInsideFenceArea(CVehicle::instance()->pivotAxlePos));
                    // Qt 6.8 FIX: Removed redundant self-assignment that could cause binding loop
                }

            }

            //}
            //// here is stop logic for out of bounds - in an inner or out the outer turn border.
            //else
            //{
            //    //this->setIsOutOfBounds(true);
            //    if (Backend::instance()->mainWindow->isBtnAutoSteerOn())
            //    {
            //        if (MainWindowState::instance()->isYouTurnBtnOn())
            //        {
            //            yt.ResetCreatedYouTurn();
            //            //sim.stepDistance = 0 / 17.86;
            //        }
            //    }
            //    else
            //    {
            //        yt.isTurnCreationTooClose = false;
            //    }

            //}
        }
    }
    else
    {
        BoundaryInterface::instance()->set_isOutOfBounds(false);
    }

    //#endregion

    //update main window
    //oglMain.MakeCurrent();
    //oglMain.Refresh();

    if (Backend::instance()->isJobStarted()) {
        processSectionLookahead();


        //oglZoom_Paint();
        //processOverlapCount();
    }

    qDebug(qpos) << "Time before painting field: " << (float)swFrame.nsecsElapsed() / 1000000;
#ifdef USE_INDIRECT_RENDERING
    oglMain_Paint();
#endif

    //Both the framebuffer and the qquickitem renderer share the same interface here.
    QQuickItem *renderer = qobject_cast<QQuickItem *>(Backend::instance()->aogRenderer);
    // CRITICAL: Force OpenGL update in GUI thread to prevent threading violation
    //if (renderer) {
    //    renderer->update();
    //}
    //qDebug(qpos) << "Time after painting field: " << (float)swFrame.nsecsElapsed() / 1000000;

    Backend::instance()->m_fixFrame.setFrameTime(swFrame.elapsed());

    // Calculate tool position once

    //double tool_lat, tool_lon; //not used currently
    //pn.ConvertLocalToWGS84(CVehicle::instance()->pivotAxlePos.northing, CVehicle::instance()->pivotAxlePos.easting, tool_lat, tool_lon);

    CVehicle::instance()->set_avgPivDistance(CVehicle::instance()->avgPivDistance() * 0.5 + CVehicle::instance()->guidanceLineDistanceOff() * 0.5);

    // Steer module counter logic - Phase 6.0.20 Task 24 Step 3.2
    if (!SimInterface::instance()->isRunning()) {
        int counter = ModuleComm::instance()->steerModuleConnectedCounter();
        if (counter++ > 30)
            counter = 31;
        ModuleComm::instance()->set_steerModuleConnectedCounter(counter);
    }

    // === Position GPS Updates (6 properties) - Qt 6.8 QProperty (Phase 6.0.9.06) ===
    Backend::instance()->m_fixFrame.latitude = pn.latitude;
    Backend::instance()->m_fixFrame.longitude = pn.longitude;
    Backend::instance()->m_fixFrame.altitude = pn.altitude;
    Backend::instance()->m_fixFrame.easting = CVehicle::instance()->pivotAxlePos.easting;
    Backend::instance()->m_fixFrame.northing = CVehicle::instance()->pivotAxlePos.northing;
    Backend::instance()->m_fixFrame.heading = CVehicle::instance()->pivotAxlePos.heading;

    Blockage::instance()->current_speed = pn.speed;

    RateControl::instance()->speed = pn.speed;
    RateControl::instance()->width = tool.applyWidth;
    RateControl::instance()->swidth = SettingsManager::instance()->vehicle_toolWidth();
    RateControl::instance()->aBtnState = (MainWindowState::instance()->autoBtnState() == SectionState::Auto);
    RateControl::instance()->mBtnState = (MainWindowState::instance()->manualBtnState() == SectionState::On);

    // === IMU Data Updates (5 properties) ===
    Backend::instance()->m_fixFrame.imuRoll = ahrs.imuRoll;
    Backend::instance()->m_fixFrame.imuPitch = ahrs.imuPitch;
    Backend::instance()->m_fixFrame.imuHeading = ahrs.imuHeading;
    Backend::instance()->m_fixFrame.imuRollDegrees = ahrs.imuRoll;
    Backend::instance()->m_fixFrame.imuAngVel = ahrs.angVel;

    // === GPS Status Updates (8 properties) ===
    Backend::instance()->m_fixFrame.hdop = pn.hdop;
    Backend::instance()->m_fixFrame.age = pn.age;
    Backend::instance()->m_fixFrame.fixQuality = (int)pn.fixQuality;
    Backend::instance()->m_fixFrame.satellitesTracked = pn.satellitesTracked;
    Backend::instance()->m_fixFrame.hz = gpsHz;
    Backend::instance()->m_fixFrame.rawHz = nowHz;
    // Phase 6.0.20 Task 24 Step 5.6: droppedSentences - TODO implement real GPS frame drop counter
    // For now, set to 0 (old udpWatchCounts removed in Phase 4.6 AgIOService migration)
    if (Backend::instance()->m_fixFrame.droppedSentences !=0) {
        Backend::instance()->m_fixFrame.droppedSentences = 0;
    }

    //TODO: limit this to update qml at only 10hz
    emit Backend::instance()->fixFrameChanged();

    // === Navigation Updates (6 properties) ===
    Backend::instance()->set_distancePivotToTurnLine(_distancePivotToTurnLine);
    Backend::instance()->set_isYouTurnRight (yt.isYouTurnRight);
    Backend::instance()->set_isYouTurnTriggered ( yt.isYouTurnTriggered);
    Backend::instance()->set_imuCorrected( _imuCorrected);

    // === Tool Position Updates (2 properties) ===
    int whichTool = 0;

    if (tool.isToolTBT) {
        //qDebug(qpos) << tool.tankPos.easting << tool.tankPos.northing << tool.tankPos.heading;
        Tools::instance()->toolsProperties()->tools()[0]->set_easting(tool.tankPos.easting);
        Tools::instance()->toolsProperties()->tools()[0]->set_northing(tool.tankPos.northing);
        Tools::instance()->toolsProperties()->tools()[0]->set_heading(glm::toDegrees(tool.tankPos.heading));
        whichTool = 1;
    }

    //qDebug(qpos) << tool.toolPos.easting << tool.toolPos.northing << tool.tankPos.heading;
    Tools::instance()->toolsProperties()->tools()[whichTool]->set_easting(tool.toolPos.easting);
    Tools::instance()->toolsProperties()->tools()[whichTool]->set_northing(tool.toolPos.northing);
    Tools::instance()->toolsProperties()->tools()[whichTool]->set_heading(glm::toDegrees(tool.toolPos.heading));

    QList<SectionProperties *> &sectionProperties = Tools::instance()->toolsProperties()->tools()[whichTool]->sections();

    for (int i=0; i < tool.numOfSections; i++) {
        sectionProperties[i]->set_state(tool.sectionButtonState[i]);
        sectionProperties[i]->set_mapping(tool.section[i].isMappingOn);
        sectionProperties[i]->set_on(tool.section[i].isSectionOn);
    }

    //update BoundaryInterface with latest boundary lines from bnd
    bnd.updateInterface();

    //update TracksProperties with latest track data
    track.updateInterface();

    //qDebug(qpos) << CVehicle::instance()->pivotAxlePos.easting << CVehicle::instance()->pivotAxlePos.northing << CVehicle::instance()->pivotAxlePos.heading;
}

void FormGPS::TheRest()
{
    CNMEA &pn = *Backend::instance()->pn();

    //positions and headings
    CalculatePositionHeading();

    //calculate lookahead at full speed, no sentence misses
    CalculateSectionLookAhead(tool.toolPos.northing, tool.toolPos.easting, CVehicle::instance()->cosSectionHeading, CVehicle::instance()->sinSectionHeading);

    //To prevent drawing high numbers of triangles, determine and test before drawing vertex
    sectionTriggerDistance = glm::Distance(pn.fix, prevSectionPos);
    contourTriggerDistance = glm::Distance(pn.fix, prevContourPos);
    gridTriggerDistance = glm::DistanceSquared(pn.fix, prevGridPos);

    //NOTE: Michael, maybe verify this is all good
    if ( isLogElevation && gridTriggerDistance > 2.9 && patchCounter !=0 && Backend::instance()->isJobStarted())
    {
        //grab fix and elevation
        sbGrid.append(
            QString::number(pn.latitude, 'f', 7).toUtf8() + ","
            + QString::number(pn.longitude, 'f', 7).toUtf8() + ","
            + QString::number(pn.altitude - CVehicle::instance()->antennaHeight, 'f', 3).toUtf8() + ","
            + QString::number(pn.fixQuality).toUtf8() + ","
            + QString::number(pn.fix.easting, 'f', 2).toUtf8() + ","
            + QString::number(pn.fix.northing, 'f', 2).toUtf8() + ","
            + QString::number(CVehicle::instance()->pivotAxlePos.heading, 'f', 3).toUtf8() + ","
            + QString::number(ahrs.imuRoll, 'f', 3).toUtf8()
            + "\r\n");

        prevGridPos.easting = CVehicle::instance()->pivotAxlePos.easting;
        prevGridPos.northing = CVehicle::instance()->pivotAxlePos.northing;
    }

    //contour points
    if (Backend::instance()->isJobStarted() &&(contourTriggerDistance > tool.contourWidth
                         || contourTriggerDistance > sectionTriggerStepDistance))
    {
        AddContourPoints();
    }

    //section on off and points
    if (sectionTriggerDistance > sectionTriggerStepDistance && Backend::instance()->isJobStarted())
    {
        AddSectionOrPathPoints();
    }

    //test if travelled far enough for new boundary point
    if (bnd.isOkToAddPoints)
    {
        //if at least 1 metre distance from last point, add a new one
        bnd.AddCurrentPoint(1);
    }

    //calc distance travelled since last GPS fix
    //distance = glm::distance(pn.fix, prevFix);
    //if (CVehicle::instance()->avgSpeed > 1)

    if ((CVehicle::instance()->avgSpeed() - previousSpeed  ) < -CVehicle::instance()->panicStopSpeed && CVehicle::instance()->panicStopSpeed != 0)
    {
        if (MainWindowState::instance()->isBtnAutoSteerOn())
            MainWindowState::instance()->set_isBtnAutoSteerOn(false);
    }

    previousSpeed = CVehicle::instance()->avgSpeed();
}

void FormGPS::processSectionLookahead() {
    //qDebug(qpos) << "frame time before doing section lookahead " << swFrame.elapsed(;
    //lock.lockForWrite(;
    //qDebug(qpos) << "frame time after getting lock  " << swFrame.elapsed(;

#define USE_QPAINTER_BACKBUFFER

    qDebug(qpos) << "Main callback thread is" << QThread::currentThread();
    QImage back_buffer;

#ifdef USE_QPAINTER_BACKBUFFER
    auto result = QtConcurrent::run( [this, &back_buffer]() {
        BACKEND_TRACK(track);

        //Draw the coverage in the back buffer
        tool.DrawPatchesBackQP(tram,bnd, CVehicle::instance()->pivotAxlePos, MainWindowState::instance()->isHeadlandOn(), track.idx() > -1);

        QMetaObject::invokeMethod(QApplication::instance(), [this, &back_buffer]() {
#else
    oglBack_Paint();
#endif

    CPGN_EF &p_239 = ModuleComm::instance()->p_239;
    CPGN_E5 &p_229 = ModuleComm::instance()->p_229;

    if (SettingsManager::instance()->display_showBack()) {
#if QT_VERSION < QT_VERSION_CHECK(6,9,0)
        grnPixelsWindow->setPixmap(QPixmap::fromImage(tool.grnPixWindow.mirrored(false, true)));
#else
        grnPixelsWindow->setPixmap(QPixmap::fromImage(tool.grnPixWindow.flipped()));
#endif
        //overlapPixelsWindow->setPixmap(QPixmap::fromImage(overPix.mirrored()));
    }

    //calculate sections on and off
    tool.ProcessLookAhead(Backend::instance()->m_fixFrame.hz,
                          MainWindowState::instance()->autoBtnState(),
                          bnd, tram);

    //send the byte out to section machines
    tool.BuildMachineByte(tram);

    //if a minute has elapsed save the field in case of crash and to be able to resume
    if (minuteCounter > 30 && Backend::instance()->m_fixFrame.sentenceCounter < 20)
    {
        // Phase 2.4: No longer need to stop timer - saves are now fast (< 50ms)
        // tmrWatchdog->stop();  // REMOVED - buffered saves don't block GPS

        //don't save if no gps
        if (Backend::instance()->isJobStarted())
        {
            //auto save the field patches, contours accumulated so far
            FileSaveSections();  // Now < 50ms with buffering
            FileSaveContour();   // Now < 50ms with buffering

            //NMEA log file
            //TODO: if (isLogElevation) FileSaveElevation(;
            //ExportFieldAs_KML(;
        }

        //if its the next day, calc sunrise sunset for next day
        minuteCounter = 0;

        //set saving flag off
        isSavingFile = false;

        // Phase 2.4: No longer need to restart timer
        // tmrWatchdog->start();  // REMOVED - timer never stopped

        //calc overlap
        //oglZoom.Refresh(;

    }

    if (Backend::instance()->isJobStarted())
    {
        p_239.pgn[CPGN_EF::geoStop] = BoundaryInterface::instance()->isOutOfBounds() ? 1 : 0;

        // SendPgnToLoop(p_239.pgn;  // âŒ REMOVED - Phase 4.6: AgIOService Workers handle PGN

        // SendPgnToLoop(p_229.pgn;  // âŒ REMOVED - Phase 4.6: Use AgIOService.sendPgn() instead
        if (m_agioService) {
            m_agioService->sendPgn(p_239.pgn);
            m_agioService->sendPgn(p_229.pgn);
        }
    }

#ifdef USE_QPAINTER_BACKBUFFER
    qDebug(qpos) << "After threaded back buffer drawing, section lookahead finished at " << swFrame.elapsed();
    }, Qt::QueuedConnection);
    });
#endif

    //lock.unlock(;

    //this is the end of the "frame". Now we wait for next NMEA sentence with a valid fix.
}


void FormGPS::CalculatePositionHeading()
{
    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_TRACK(track);
    // #region pivot hitch trail
    //Probably move this into CVehicle

    //translate from pivot position to steer axle and pivot axle position
    //translate world to the pivot axle
    CVehicle::instance()->pivotAxlePos.easting = pn.fix.easting - (sin(CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaPivot);
    CVehicle::instance()->pivotAxlePos.northing = pn.fix.northing - (cos(CVehicle::instance()->fixHeading()) * CVehicle::instance()->antennaPivot);
    CVehicle::instance()->pivotAxlePos.heading = CVehicle::instance()->fixHeading();

    CVehicle::instance()->steerAxlePos.easting = CVehicle::instance()->pivotAxlePos.easting + (sin(CVehicle::instance()->fixHeading()) * CVehicle::instance()->wheelbase);
    CVehicle::instance()->steerAxlePos.northing = CVehicle::instance()->pivotAxlePos.northing + (cos(CVehicle::instance()->fixHeading()) * CVehicle::instance()->wheelbase);
    CVehicle::instance()->steerAxlePos.heading = CVehicle::instance()->fixHeading();
    
    // PHASE 4.3: Measure execution latency for vehicle position update
    // This measures the TIME BETWEEN calls (which gives us frequency)
    static QElapsedTimer intervalTimer;
    static bool intervalTimerStarted = false;
    if (!intervalTimerStarted) {
        intervalTimer.start();
        intervalTimerStarted = true;
    }
    
    // Measure interval between calls (for frequency calculation)
    qint64 intervalBetweenCalls = intervalTimer.nsecsElapsed();
    intervalTimer.restart();
    
    // For actual execution latency, we need a different approach
    // The execution of this function should be < 1ms
    // The interval between calls is ~100ms at 10Hz which is normal for GPS
    
    static int latencyLogCounter = 0;
    if (++latencyLogCounter % 500 == 0) { // Log every 500 updates (~10s at 50Hz actual frequency)
        double actualHz = 1000000000.0 / intervalBetweenCalls; // Convert ns to Hz
        qDebug(qpos) << "📊 UpdateFixPosition - Interval:" << intervalBetweenCalls/1000000 << "ms"
                 << "Actual Hz:" << actualHz << "GPS Hz:" << gpsHz;
    }

    //guidance look ahead distance based on time or tool width at least

    if (!track.ABLine.isLateralTriggered && !track.curve.isLateralTriggered)
    {
        double guidanceLookDist = (std::max(tool.width * 0.5, CVehicle::instance()->avgSpeed() * 0.277777 * Backend::instance()->guidanceLookAheadTime()));
        CVehicle::instance()->guidanceLookPos.easting = CVehicle::instance()->pivotAxlePos.easting + (sin(CVehicle::instance()->fixHeading()) * guidanceLookDist);
        CVehicle::instance()->guidanceLookPos.northing = CVehicle::instance()->pivotAxlePos.northing + (cos(CVehicle::instance()->fixHeading()) * guidanceLookDist);
    }

    //determine where the rigid vehicle hitch ends
    CVehicle::instance()->hitchPos.easting = pn.fix.easting + (sin(CVehicle::instance()->fixHeading()) * (tool.hitchLength - CVehicle::instance()->antennaPivot));
    CVehicle::instance()->hitchPos.northing = pn.fix.northing + (cos(CVehicle::instance()->fixHeading()) * (tool.hitchLength - CVehicle::instance()->antennaPivot));

    //tool attached via a trailing hitch
    if (tool.isToolTrailing)
    {
        double over;
        if (tool.isToolTBT)
        {
            //Torriem rules!!!!! Oh yes, this is all his. Thank-you
            if (distanceCurrentStepFix != 0)
            {
                tool.tankPos.heading = atan2(CVehicle::instance()->hitchPos.easting - tool.tankPos.easting, CVehicle::instance()->hitchPos.northing - tool.tankPos.northing);
                if (tool.tankPos.heading < 0) tool.tankPos.heading += glm::twoPI;
            }

            ////the tool is seriously jacknifed or just starting out so just spring it back.
            over = fabs(M_PI - fabs(fabs(tool.tankPos.heading - CVehicle::instance()->fixHeading()) - M_PI));

            if ((over < 2.0) && (startCounter > 50))
            {
                tool.tankPos.easting = CVehicle::instance()->hitchPos.easting + (sin(tool.tankPos.heading) * (tool.tankTrailingHitchLength));
                tool.tankPos.northing = CVehicle::instance()->hitchPos.northing + (cos(tool.tankPos.heading) * (tool.tankTrailingHitchLength));
            }

            //criteria for a forced reset to put tool directly behind vehicle
            if (over > 2.0 || startCounter < 51 )
            {
                tool.tankPos.heading = CVehicle::instance()->fixHeading();
                tool.tankPos.easting = CVehicle::instance()->hitchPos.easting + (sin(tool.tankPos.heading) * (tool.tankTrailingHitchLength));
                tool.tankPos.northing = CVehicle::instance()->hitchPos.northing + (cos(tool.tankPos.heading) * (tool.tankTrailingHitchLength));
            }

        }

        else
        {
            tool.tankPos.heading = CVehicle::instance()->fixHeading();
            tool.tankPos.easting = CVehicle::instance()->hitchPos.easting;
            tool.tankPos.northing = CVehicle::instance()->hitchPos.northing;
        }

        //Torriem rules!!!!! Oh yes, this is all his. Thank-you
        if (distanceCurrentStepFix != 0)
        {
            tool.toolPivotPos.heading = atan2(tool.tankPos.easting - tool.toolPivotPos.easting, tool.tankPos.northing - tool.toolPivotPos.northing);
            if (tool.toolPivotPos.heading < 0) tool.toolPivotPos.heading += glm::twoPI;
        }

        ////the tool is seriously jacknifed or just starting out so just spring it back.
        over = fabs(M_PI - fabs(fabs(tool.toolPivotPos.heading - tool.tankPos.heading) - M_PI));

        if ((over < 1.9) && (startCounter > 50))
        {
            tool.toolPivotPos.easting = tool.tankPos.easting + (sin(tool.toolPivotPos.heading) * (tool.trailingHitchLength));
            tool.toolPivotPos.northing = tool.tankPos.northing + (cos(tool.toolPivotPos.heading) * (tool.trailingHitchLength));
        }

        //criteria for a forced reset to put tool directly behind vehicle
        if (over > 1.9 || startCounter < 51 )
        {
            tool.toolPivotPos.heading = tool.tankPos.heading;
            tool.toolPivotPos.easting = tool.tankPos.easting + (sin(tool.toolPivotPos.heading) * (tool.trailingHitchLength));
            tool.toolPivotPos.northing = tool.tankPos.northing + (cos(tool.toolPivotPos.heading) * (tool.trailingHitchLength));
        }

        tool.toolPos.heading = tool.toolPivotPos.heading;
        tool.toolPos.easting = tool.tankPos.easting +
                                  (sin(tool.toolPivotPos.heading) * tool.trailingHitchLength);
        tool.toolPos.northing = tool.tankPos.northing +
                                   (cos(tool.toolPivotPos.heading) * tool.trailingHitchLength);

    }

    //rigidly connected to vehicle
    else
    {
        tool.toolPivotPos.heading = CVehicle::instance()->fixHeading();
        tool.toolPivotPos.easting = CVehicle::instance()->hitchPos.easting;
        tool.toolPivotPos.northing = CVehicle::instance()->hitchPos.northing;

        tool.toolPos.heading = CVehicle::instance()->fixHeading();
        tool.toolPos.easting = CVehicle::instance()->hitchPos.easting;
        tool.toolPos.northing = CVehicle::instance()->hitchPos.northing;
    }

    //#endregion

    //used to increase triangle count when going around corners, less on straight
    //pick the slow moving side edge of tool
    double distance = tool.width * 0.5;
    if (distance > 5) distance = 5;

    //whichever is less
    if (tool.farLeftSpeed < tool.farRightSpeed)
    {
        double twist = tool.farLeftSpeed / tool.farRightSpeed;
        twist *= twist;
        if (twist < 0.2) twist = 0.2;
        CVehicle::instance()->sectionTriggerStepDistance = distance * twist * twist;
    }
    else
    {
        double twist = tool.farRightSpeed / tool.farLeftSpeed;
        //twist *= twist;
        if (twist < 0.2) twist = 0.2;

        CVehicle::instance()->sectionTriggerStepDistance = distance * twist * twist;
    }

    //finally fixed distance for making a curve line
    if (!track.curve.isMakingCurve) CVehicle::instance()->sectionTriggerStepDistance = CVehicle::instance()->sectionTriggerStepDistance + 0.5;
    //if (MainWindowState::instance()->isContourBtnOn()) CVehicle::instance()->sectionTriggerStepDistance *=0.5;

    //precalc the sin and cos of heading * -1
    CVehicle::instance()->sinSectionHeading = sin(-tool.toolPivotPos.heading);
    CVehicle::instance()->cosSectionHeading = cos(-tool.toolPivotPos.heading);

}

//calculate the extreme tool left, right velocities, each section lookahead, and whether or not its going backwards
void FormGPS::CalculateSectionLookAhead(double northing, double easting, double cosHeading, double sinHeading)
{
    //calculate left side of section 1
    Vec2 left;
    Vec2 right = left;
    double leftSpeed = 0, rightSpeed = 0;

    //speed max for section kmh*0.277 to m/s * 10 cm per pixel * 1.7 max speed
    double meterPerSecPerPixel = fabs(CVehicle::instance()->avgSpeed()) * 4.5;
    //qDebug(qpos) << pn.speed << ", m/s per pixel is " << meterPerSecPerPixel;

    //now loop all the section rights and the one extreme left
    for (int j = 0; j < tool.numOfSections; j++)
    {
        if (j == 0)
        {
            //only one first left point, the rest are all rights moved over to left
            tool.section[j].leftPoint = Vec2(cosHeading * (tool.section[j].positionLeft) + easting,
                                             sinHeading * (tool.section[j].positionLeft) + northing);

            left = tool.section[j].leftPoint - tool.section[j].lastLeftPoint;

            //save a copy for next time
            tool.section[j].lastLeftPoint = tool.section[j].leftPoint;

            //get the speed for left side only once

            leftSpeed = left.getLength() * gpsHz * 10;
            //qDebug(qpos) << leftSpeed << " - left speed";
            if (leftSpeed > meterPerSecPerPixel) leftSpeed = meterPerSecPerPixel;
        }
        else
        {
            //right point from last section becomes this left one
            tool.section[j].leftPoint = tool.section[j - 1].rightPoint;
            left = tool.section[j].leftPoint - tool.section[j].lastLeftPoint;

            //save a copy for next time
            tool.section[j].lastLeftPoint = tool.section[j].leftPoint;

            //save the slower of the 2
            if (leftSpeed > rightSpeed) leftSpeed = rightSpeed;
        }

        tool.section[j].rightPoint = Vec2(cosHeading * (tool.section[j].positionRight) + easting,
                                          sinHeading * (tool.section[j].positionRight) + northing);
        /*
        qDebug(qpos) << j << ": " << tool.section[j].leftPoint.easting << "," <<
                                 tool.section[j].leftPoint.northing <<" " <<
                                 tool.section[j].rightPoint.easting << ", " <<
                                 tool.section[j].rightPoint.northing;
                                 */


        //now we have left and right for this section
        right = tool.section[j].rightPoint - tool.section[j].lastRightPoint;

        //save a copy for next time
        tool.section[j].lastRightPoint = tool.section[j].rightPoint;

        //grab vector length and convert to meters/sec/10 pixels per meter
        rightSpeed = right.getLength() * gpsHz * 10;
        if (rightSpeed > meterPerSecPerPixel) rightSpeed = meterPerSecPerPixel;

        //Is section outer going forward or backward
        double head = left.headingXZ();

        if (head < 0) head += glm::twoPI;

        if (M_PI - fabs(fabs(head - tool.toolPos.heading) - M_PI) > glm::PIBy2)
        {
            if (leftSpeed > 0) leftSpeed *= -1;
        }

        head = right.headingXZ();
        if (head < 0) head += glm::twoPI;
        if (M_PI - fabs(fabs(head - tool.toolPos.heading) - M_PI) > glm::PIBy2)
        {
            if (rightSpeed > 0) rightSpeed *= -1;
        }

        double sped = 0;
        //save the far left and right speed in m/sec averaged over 20%
        if (j==0)
        {
            sped = (leftSpeed * 0.1);
            if (sped < 0.1) sped = 0.1;
            tool.farLeftSpeed = tool.farLeftSpeed * 0.7 + sped * 0.3;
            //qWarning() << sped << tool.farLeftSpeed << CVehicle::instance()->avgSpeed();
        }

        if (j == tool.numOfSections - 1)
        {
            sped = (rightSpeed * 0.1);
            if(sped < 0.1) sped = 0.1;
            tool.farRightSpeed = tool.farRightSpeed * 0.7 + sped * 0.3;
        }
        //choose fastest speed
        if (leftSpeed > rightSpeed)
        {
            sped = leftSpeed;
            leftSpeed = rightSpeed;
        }
        else sped = rightSpeed;
        tool.section[j].speedPixels = tool.section[j].speedPixels * 0.7 + sped * 0.3;
    }
}

void FormGPS::AddContourPoints()
{
    //if (isConstantContourOn)
    {
        //record contour all the time
        //Contour Base Track.... At least One section on, turn on if not
        if (patchCounter != 0)
        {
            //keep the line going, everything is on for recording path
            if (ct.isContourOn) ct.AddPoint(CVehicle::instance()->pivotAxlePos);
            else
            {
                ct.StartContourLine();
                ct.AddPoint(CVehicle::instance()->pivotAxlePos);
            }
        }

        //All sections OFF so if on, turn off
        else
        {
            if (ct.isContourOn)
            { ct.StopContourLine(contourSaveList); }
        }

        //Build contour line if close enough to a patch
        if (MainWindowState::instance()->isContourBtnOn()) ct.BuildContourGuidanceLine(secondsSinceStart, *CVehicle::instance(), CVehicle::instance()->pivotAxlePos);
    }
    //save the north & east as previous
    prevContourPos.northing = CVehicle::instance()->pivotAxlePos.northing;
    prevContourPos.easting = CVehicle::instance()->pivotAxlePos.easting;
}

//add the points for section, contour line points, Area Calc feature
void FormGPS::AddSectionOrPathPoints()
{
    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_TRACK(track);
    int alpha;

    if (SettingsManager::instance()->display_isDayMode())
        alpha=152;
    else
        alpha=76;

    if (recPath.isRecordOn)
    {
        //keep minimum speed of 1.0
        double speed = CVehicle::instance()->avgSpeed();
        if (CVehicle::instance()->avgSpeed() < 1.0) speed = 1.0;
        bool autoBtn = (MainWindowState::instance()->autoBtnState() == SectionState::Auto);

        recPath.recList.append(CRecPathPt(CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing, CVehicle::instance()->pivotAxlePos.heading, speed, autoBtn));
    }

    track.AddPathPoint(CVehicle::instance()->pivotAxlePos);

    //save the north & east as previous
    prevSectionPos.northing = pn.fix.northing;
    prevSectionPos.easting = pn.fix.easting;

    // if non zero, at least one section is on.
    patchCounter = 0;

    for (int j=0; j < tool.numOfSections; j++) {
        if (tool.section[j].isSectionOn) {
            if(SettingsManager::instance()->color_isMultiColorSections())
                LayerService::instance()->addSectionVertices(
                        j,
                        tool.section[j].leftPoint,
                        tool.section[j].rightPoint,QColorWithAlpha(tool.secColors[j], alpha));
            else
                LayerService::instance()->addSectionVertices(
                    j,
                    tool.section[j].leftPoint,
                    tool.section[j].rightPoint,
                    QColorWithAlpha(SettingsManager::instance()->display_colorSectionsDay(), alpha));
        }
    }

    //send the current and previous GPS fore/aft corrected fix to each section
    for (int j = 0; j < tool.triStrip.count(); j++)
    {
        if (tool.triStrip[j].isDrawing)
        {
            if (Backend::instance()->isPatchesChangingColor())
            {
                tool.triStrip[j].numTriangles = 64;
                Backend::instance()->set_isPatchesChangingColor(false);
            }

            tool.triStrip[j].AddMappingPoint(tool.secColors[j],
                                        tool.section[tool.triStrip[j].currentStartSectionNum].leftPoint,
                                        tool.section[tool.triStrip[j].currentEndSectionNum].rightPoint,
                                        tool.patchSaveList);
            patchCounter++;
        }
    }
}

//the start of first few frames to initialize entire program
void FormGPS::InitializeFirstFewGPSPositions()
{
    CNMEA &pn = *Backend::instance()->pn();
    Camera &camera = *Camera::instance();

    if (!isFirstFixPositionSet)
    {
        // PHASE 6.0.41: Force latStart/lonStart update when switching modes, even if field open
        // Prevents gray screen when GPS arrives after SIM->REAL switch with open field
        if (!Backend::instance()->isJobStarted() || m_forceGPSReinitialization)
        {
            // PHASE 6.0.42.5: Validate GPS coordinates before initialization
            // Race condition fix: Timer (40 Hz) can trigger BEFORE GPS data arrives after mode switch
            // Scenario: SIM→REAL→UDP ON → timer tick at T+25ms, GPS arrives at T+50-200ms
            // If pn.latitude/longitude == 0 → wait for next cycle (25ms) instead of corrupting latStart/lonStart
            // Prevents initializing coordinate reference with invalid (0,0) which causes gray screen
            if (pn.latitude == 0 || pn.longitude == 0) {
                // Invalid coordinates - wait for real GPS data
                // Do NOT clear m_forceGPSReinitialization flag
                // Do NOT set isFirstFixPositionSet = true
                return;  // Retry next cycle (25ms later)
            }

            // Valid coordinates → initialize normally
            pn.setLatStart(pn.latitude);
            pn.setLonStart(pn.longitude);
            pn.SetLocalMetersPerDegree();

            // PHASE 6.0.41: Clear flag after successful reinitialization
            if (m_forceGPSReinitialization) {
                m_forceGPSReinitialization = false;
            }
        }

        // Phase 6.3.1: Use PropertyWrapper for safe QObject access
        pn.ConvertWGS84ToLocal(pn.latitude, pn.longitude, pn.fix.northing, pn.fix.easting);

        //Draw a grid once we know where in the world we are.
        isFirstFixPositionSet = true;

        //most recent fixes
        prevFix.easting = pn.fix.easting;
        prevFix.northing = pn.fix.northing;

        //run once and return
        isFirstFixPositionSet = true;

        return;
    }

    else
    {

        //most recent fixes
        prevFix.easting = pn.fix.easting; prevFix.northing = pn.fix.northing;

        //keep here till valid data
        if (startCounter > (20))
        {
            isGPSPositionInitialized = true;
            lastReverseFix = pn.fix;
        }

        //in radians
        CVehicle::instance()->set_fixHeading( 0 );
        tool.toolPos.heading = CVehicle::instance()->fixHeading();

        //send out initial zero settings
        if (isGPSPositionInitialized)
        {
            //TODO determine if it is day from wall clock and date
            isDayTime = true;

            camera.SetZoom();
        }
        return;
    }
}

// Phase 6.0.21: Receive parsed data from AgIOService broadcast signal
void FormGPS::onParsedDataReady(const PGNParser::ParsedData& data)
{
    CNMEA &pn = *Backend::instance()->pn();
    if (!data.isValid) return;

    // Phase 6.0.21.12: Ignore UDP GPS data when simulation is ON
    // Prevents conflict: simulation data vs real UDP data fighting for same Q_PROPERTY
    // When simulation ON + AgIO ON + UDP ON → simulation has priority, ignore UDP
    if (SettingsManager::instance()->menu_isSimulatorOn()) {
        return;  // Simulation mode active - ignore real GPS data from UDP
    }

    // ===== Phase 6.0.23.4: Update INTERNAL structures at 40 Hz (real-time calculations) =====
    // Phase 6.0.21.11: Update pn structure before UpdateFixPosition() (like simulation mode)
    // CRITICAL: UpdateFixPosition() line 1246 checks "if (m_latitude != pn.latitude)"
    // If pn.latitude is not updated, it overwrites m_latitude with stale value (0) → binding conflict

    // ✅ Problem 14 Fix (Enhanced): Validate GPS FIX QUALITY before assigning ANY data
    // NMEA Standard:
    // - quality=0 → "Invalid" (no GPS fix)
    // - satellites=0 → No satellites tracked (impossible to have position)
    // Strategy: Reject entire fix if invalid, don't overwrite last valid position

    // Always update quality/satellites (metadata about GPS state)
    pn.fixQuality = data.quality;
    pn.satellitesTracked = data.satellites;

    // Check if GPS fix is VALID (quality > 0 AND satellites > 0)
    bool validGpsFix = (data.quality > 0 && data.satellites > 0);

    if (validGpsFix) {
        // GPS fix valid → update coordinates IF non-zero (empty NMEA field protection)
        if (data.latitude != 0.0 && data.longitude != 0.0) {
            // PHASE 6.0.42: Check for GPS jump before updating position
            // Handles SIM→REAL mode switch, GPS module change, position corrections
            // If jump detected: closes field (if open), updates latStart/lonStart, resets flags
            if (detectGPSJump(data.latitude, data.longitude)) {
                handleGPSJump(data.latitude, data.longitude);
            }

            pn.latitude = data.latitude;
            pn.longitude = data.longitude;
        }

        // Update other GPS data (only if fix valid)
        // Note: altitude=0 is VALID (sea level), age=0 is VALID (no differential correction)
        pn.altitude = data.altitude;
        pn.hdop = data.hdop;
        pn.age = data.age;
    } else {
        // GPS fix INVALID (quality=0 or satellites=0)
        // Don't update position/altitude/hdop/age → preserve last valid values
        // This prevents display from jumping to 0 when GPS signal lost momentarily
        qDebug(qpos) << "❌ GPS fix INVALID - quality:" << data.quality
                 << "satellites:" << data.satellites
                 << "→ preserving last valid position";
    }

    // Update heading in pn structure
    if (data.headingDual > 0) {
        pn.headingTrue = pn.headingTrueDual = data.headingDual;
    } else if (data.heading > 0) {
        pn.headingTrue = data.heading;
        pn.headingTrueDual = 0;  // No dual antenna
    }

    // Speed
    pn.vtgSpeed = data.speed;

    // ✅ CRITICAL FIX: Convert lat/lon to northing/easting at EVERY packet (like simulation)
    // BUG: Without this, northing/easting were only calculated ONCE (first position)
    // → Lat/lon updated every packet but northing/easting stayed at old values
    // → Roll corrections applied to stale northing/easting → tracteur pivote!
    // Simulation does this correctly (formgps_sim.cpp:55) - real mode must match
    if (validGpsFix) {
        pn.ConvertWGS84ToLocal(pn.latitude, pn.longitude, pn.fix.northing, pn.fix.easting);
    }

    // ✅ Problem 14 Fix (Final): Store IMU data in ahrs structure (40 Hz)
    // Update ahrs structure every time IMU data arrives
    if (data.hasIMU) {
        // Store in ahrs structure (used by calculations)
        ahrs.imuHeading = data.imuHeading;
        ahrs.imuRoll = data.imuRoll;
        ahrs.imuPitch = data.imuPitch;
        ahrs.imuYawRate = data.yawRate;

        // ✅ NO THROTTLING: Assign directly to Q_PROPERTY (40 Hz)
        // Qt optimizes: only triggers QML update if value actually changed
        // Simpler architecture: no intermediate storage, no sync bugs
        Backend::instance()->m_fixFrame.imuHeading = data.imuHeading;  // 0° = north (VALID)
        Backend::instance()->m_fixFrame.imuRoll = data.imuRoll;        // 0° = horizontal (VALID)
        Backend::instance()->m_fixFrame.imuPitch = data.imuPitch;      // 0° = no slope (VALID)
        Backend::instance()->m_fixFrame.yawRate = data.yawRate;        // 0°/s = no rotation (VALID)
    }

    // PHASE 6.0.23: Store AutoSteer control data if present (PGN 253/250) - 40 Hz
    if (data.hasSteerData) {
        // Steer Angle Actual (from PGN 253 byte 5-6)
        if (data.steerAngleActual != 0) {
            ModuleComm::instance()->set_actualSteerAngleDegrees(data.steerAngleActual * 0.01);
        }

        // Switch Status (from PGN 253 byte 11)
        if (data.switchByte != 0) {
            ModuleComm::instance()->set_workSwitchHigh((data.switchByte & 0x01) == 0x01);
            ModuleComm::instance()->set_steerSwitchHigh((data.switchByte & 0x02) == 0x02);
            ModuleComm::instance()->CheckWorkAndSteerSwitch(MainWindowState::instance()->isBtnAutoSteerOn());
        }

        // PWM Display (from PGN 253 byte 12)
        if (data.pwmDisplay != 0) {
            ModuleComm::instance()->set_pwmDisplay(data.pwmDisplay);
        }

        // Sensor Value (from PGN 250 byte 5)
        if (data.sensorValue != 0) {
            ModuleComm::instance()->set_sensorData(data.sensorValue);
        }
    }

    Backend::instance()->m_fixFrame.sentenceCounter = 0;

    // Phase 6.0.24: UpdateFixPosition() moved to timerGPS callback (40 Hz fixed rate)
    // onParsedDataReady() now ONLY stores data in pn/ahrs structures
    // timerGPS (25ms = 40 Hz) calls onGPSTimerTimeout() → UpdateFixPosition()
    // This prevents UpdateFixPosition() from being called at UDP packet rate (700 Hz bug!)
    // Architecture now matches simulation mode: data storage separated from position update

    // ===== Phase 6.0.23.4: Update Q_PROPERTY for QML at 10 Hz (display only) =====
    static int qmlUpdateCounter = 0;
    if (++qmlUpdateCounter % 4 == 0) {  // 40 Hz / 4 = 10 Hz for QML display
        // Position data (10 Hz → QML)
        // ✅ Problem 14 Fix: pn.latitude/longitude now validated above (not overwritten with 0)
        // Safe to assign - will use last valid position if current data was 0

        // Speed (10 Hz → QML)
        // ✅ IMU data is now assigned directly at 40 Hz (see above, no throttling)
        // Simpler: no need to duplicate assignment here

        // AutoSteer display data (10 Hz → QML)
    }
}

// ========== Phase 6.0.25: Separated Data Handlers ==========

void FormGPS::onNmeaDataReady(const PGNParser::ParsedData& data)
{
    CNMEA &pn = *Backend::instance()->pn();
    // NMEA GPS data handler (~8 Hz)
    // Updates internal structures only - UpdateFixPosition() called by timerGPS at 40 Hz

    if (!data.isValid) return;

    // Phase 6.0.21.12: Ignore UDP GPS data when simulation is ON
    if (SettingsManager::instance()->menu_isSimulatorOn()) {
        return;  // Simulation mode active
    }

    // Update pn structure (internal GPS data)
    pn.fixQuality = data.quality;
    pn.satellitesTracked = data.satellites;

    // Validate GPS fix before updating position
    bool validGpsFix = (data.quality > 0 && data.satellites > 0);

    if (validGpsFix) {
        if (data.latitude != 0.0 && data.longitude != 0.0) {
            pn.latitude = data.latitude;
            pn.longitude = data.longitude;
            pn.ConvertWGS84ToLocal(pn.latitude, pn.longitude, pn.fix.northing, pn.fix.easting);

            // PHASE 6.0.33: Store RAW GPS position (8 Hz updates, immutable)
            // This position is NEVER modified by corrections (antenna offset, roll)
            // UpdateFixPosition() always starts from this raw position
            // Prevents cascade corrections when timer calls UpdateFixPosition() between GPS packets
            QMutexLocker lock(&m_rawGpsPositionMutex);
            m_rawGpsPosition.easting = pn.fix.easting;
            m_rawGpsPosition.northing = pn.fix.northing;
        }
        pn.altitude = data.altitude;
        pn.hdop = data.hdop;
        pn.age = data.age;
    }

    // Update speed (always, even if fix invalid)
    pn.vtgSpeed = data.speed;

    // Phase 6.0.27: Update heading in pn structure (FIXED to match legacy behavior)
    if (data.headingDual > 0) {
        // Dual antenna heading - update BOTH headingTrue and headingTrueDual
        pn.headingTrue = pn.headingTrueDual = data.headingDual;
    } else if (data.heading > 0) {
        // Single antenna heading - update headingTrue, clear headingTrueDual
        pn.headingTrue = data.heading;
        pn.headingTrueDual = 0;  // No dual antenna
    }

    // Phase 6.0.27: IMU data from NMEA (FIXED to match legacy behavior)
    // Update ahrs structure FIRST (used by calculations), then Q_PROPERTY (used by QML)
    if (data.hasIMU) {
        // Store in ahrs structure (used by calculations)
        ahrs.imuHeading = data.imuHeading;
        ahrs.imuRoll = data.imuRoll;
        ahrs.imuPitch = data.imuPitch;
        ahrs.imuYawRate = data.yawRate;

        // Update Q_PROPERTY (used by QML display)
        Backend::instance()->m_fixFrame.imuHeading = data.imuHeading; // 0° = north (VALID)
        Backend::instance()->m_fixFrame.imuRoll = data.imuRoll;        // 0° = horizontal (VALID)
        Backend::instance()->m_fixFrame.imuPitch = data.imuPitch;      // 0° = no slope (VALID)
        Backend::instance()->m_fixFrame.yawRate = data.yawRate;        // 0°/s = no rotation (VALID)
    }

    // Watchdog timer (tmrWatchdog_timeout) increments sentenceCounter every 250ms
    // MainWindow.qml shows "No GPS" warning when sentenceCounter > 29 (~7.25 seconds)
    // Must reset counter to 0 when NMEA data arrives to indicate GPS is working
    Backend::instance()->m_fixFrame.sentenceCounter = 0;

    // NO UpdateFixPosition() here - called by timerGPS at 40 Hz fixed rate
}

void FormGPS::onImuDataReady(const PGNParser::ParsedData& data)
{
    // External IMU module data handler (~10 Hz)
    // Updates IMU variables only - NO GPS position update

    if (!data.isValid) return;

    // Phase 6.0.21.12: Ignore UDP IMU data when simulation is ON
    if (SettingsManager::instance()->menu_isSimulatorOn()) {
        return;
    }

    // PGN 212: IMU disconnect - set sentinel values
    if (data.pgnNumber == 212) {
        Backend::instance()->m_fixFrame.imuHeading = 99999.0;  // Sentinel: IMU disconnected
        Backend::instance()->m_fixFrame.imuRoll = 88888.0;     // Sentinel: IMU disconnected
        Backend::instance()->m_fixFrame.yawRate = 0.0;
        return;
    }

    // PGN 211: External IMU data
    if (data.hasIMU) {
        Backend::instance()->m_fixFrame.imuHeading = data.imuHeading;

        // Roll with filtering and inversion
        double rollK = data.imuRoll;
        if (ahrs.isRollInvert) rollK *= -1.0;
        rollK -= ahrs.rollZero;

        // Apply exponential filter
        double currentRoll = Backend::instance()->m_fixFrame.imuRoll;
        double filteredRoll = currentRoll * ahrs.rollFilter + rollK * (1.0 - ahrs.rollFilter);
        Backend::instance()->m_fixFrame.imuRoll = filteredRoll;

        // Yaw rate
        if (data.yawRate != 0.0) {
            Backend::instance()->m_fixFrame.yawRate = data.yawRate;
        }
    }

    // NO UpdateFixPosition() - IMU updates only
}

void FormGPS::onSteerDataReady(const PGNParser::ParsedData& data)
{
    // AutoSteer module feedback handler (~40 Hz throttled by timer)
    // Updates mc.* variables and AutoSteer IMU fallback
    // NO GPS position update

    if (!data.isValid || !data.hasSteerData) return;

    // PGN 253: AutoSteer status
    if (data.pgnNumber == 253) {
        // Actual steer angle from module
        ModuleComm::instance()->set_actualSteerAngleDegrees(data.steerAngleActual * 0.01);

        // IMU data from AutoSteer module (fallback if no external IMU)
        if (data.hasIMU) {
            // Heading from AutoSteer BNO085 (if valid)
            if (data.imuHeading != 9999.0) {
                Backend::instance()->m_fixFrame.imuHeading = data.imuHeading;
            }

            // Roll from AutoSteer BNO085 (if valid, with filtering)
            if (data.imuRoll != 8888.0) {
                double rollK = data.imuRoll;
                if (ahrs.isRollInvert) rollK *= -1.0;
                rollK -= ahrs.rollZero;

                double currentRoll = Backend::instance()->m_fixFrame.imuRoll;
                double filteredRoll = currentRoll * ahrs.rollFilter + rollK * (1.0 - ahrs.rollFilter);
                Backend::instance()->m_fixFrame.imuRoll = filteredRoll;
            }
        }

        // Switch status (work switch, steer switch)
        ModuleComm::instance()->set_workSwitchHigh((data.switchByte & 0x01) != 0);
        ModuleComm::instance()->set_steerSwitchHigh((data.switchByte & 0x02) != 0);

        // PWM display (motor drive 0-255)
        ModuleComm::instance()->set_pwmDisplay(data.pwmDisplay);

        // Reset module connection timeout counter
        ModuleComm::instance()->set_steerModuleConnectedCounter(0);
    }

    // PGN 250: Sensor data (pressure/current)
    if (data.pgnNumber == 250) {
        ModuleComm::instance()->set_sensorData(data.sensorValue);
    }

    // NO UpdateFixPosition() - AutoSteer feedback only
}

void FormGPS::onMachineDataReady(const PGNParser::ParsedData& data)
{}

void FormGPS::onBlockageDataReady(const PGNParser::ParsedData& data)
{
}

void FormGPS::onRateControlDataReady(const PGNParser::ParsedData& data)
{
}
// Phase 6.0.24: GPS timer callback - UpdateFixPosition() at 40 Hz fixed rate
void FormGPS::onGPSTimerTimeout()
{
    // Skip if simulation mode is active (timerSim handles position updates in simulation)
    if (SettingsManager::instance()->menu_isSimulatorOn()) {
        return;
    }

    // Call UpdateFixPosition() at 40 Hz fixed rate (independent of UDP packet arrival)
    // Uses latest data stored in pn/ahrs structures by onParsedDataReady()
    // Architecture matches simulation mode: timer-driven position updates
    UpdateFixPosition();
}
