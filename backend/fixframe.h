// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef FIXFRAME_H
#define FIXFRAME_H

#include <QObject>

class FixFrame
{
    Q_GADGET

    Q_PROPERTY(double age MEMBER age)
    Q_PROPERTY(double hdop MEMBER hdop)
    Q_PROPERTY(double satellitesTracked MEMBER satellitesTracked)
    Q_PROPERTY(double easting MEMBER easting)
    Q_PROPERTY(double northing MEMBER northing)
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)
    Q_PROPERTY(double heading MEMBER heading)
    Q_PROPERTY(double altitude MEMBER altitude)
    Q_PROPERTY(double yawRate MEMBER imuAngVel)
    Q_PROPERTY(int fixQuality MEMBER fixQuality)
    Q_PROPERTY(double hz MEMBER hz)
    Q_PROPERTY(double rawHz MEMBER rawHz)
    Q_PROPERTY(int droppedSentences MEMBER droppedSentences)
    Q_PROPERTY(int sentenceCounter MEMBER sentenceCounter)
    Q_PROPERTY(double frameTime MEMBER frameTime)
    Q_PROPERTY(double gpsHeading MEMBER gpsHeading)

    Q_PROPERTY(double imuHeading MEMBER imuHeading)
    Q_PROPERTY(double imuRoll MEMBER imuRoll)
    Q_PROPERTY(double imuPitch MEMBER imuPitch)
    Q_PROPERTY(double imuRollDegrees MEMBER imuRollDegrees)
    //Q_PROPERTY(double imuAngVel MEMBER imuAngVel)

    Q_PROPERTY(double speedKph MEMBER speedKph)
    Q_PROPERTY(double fusedHeading MEMBER fusedHeading)
    Q_PROPERTY(double toolEasting MEMBER toolEasting)
    Q_PROPERTY(double toolNorthing MEMBER toolNorthing)
    Q_PROPERTY(double toolHeading MEMBER toolHeading)
    Q_PROPERTY(double avgPivDistance MEMBER avgPivDistance)
    Q_PROPERTY(double offlineDistance MEMBER offlineDistance)

public:
    //GPS information
    double age = 0.0;
    double hdop = 0.0;
    double satellitesTracked = 0.0;
    double easting = 0.0;
    double northing = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double heading = 0.0;
    double altitude = 0.0;
    double yawRate = 0;
    int fixQuality = 0;
    double hz = 0;
    double rawHz = 0;
    double gpsHeading = 0;

    int droppedSentences = 0;
    int sentenceCounter = 0;
    double frameTime = 0;
    double frameTimeRough = 0;

    //imu information
    double imuHeading = 0;
    double imuRoll = 0;
    double imuPitch = 0;
    double imuRollDegrees = 0;

    //currently not exposed to QML
    double imuAngVel = 0;


    //vehicle state, maybe put in a different struct
    double speedKph = 0;
    double fusedHeading = 0;
    double toolEasting = 0;
    double toolNorthing = 0;
    double toolHeading = 0;
    double avgPivDistance = 0;
    short int offlineDistance = 0;

    inline void setFrameTime(double newFrameTime) {
        //smooth out raw frame time
        frameTimeRough = newFrameTime;
        //if (frameTimeRough > 80) frameTimeRough = 80;

        frameTime = frameTime * 0.90 + frameTimeRough * 0.1;
    }
};

Q_DECLARE_METATYPE(FixFrame)

#endif // FIXFRAME_H
