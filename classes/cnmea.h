#ifndef CNMEA_H
#define CNMEA_H

#include <QObject>
#include <sstream>
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <QBuffer>
#include <QBindable>
#include "vec2.h"
#include "glm.h"
#include <QTextStream>
#include "simpleproperty.h"

class CNMEA : public QObject
{
    Q_OBJECT

    // GPS/NMEA Coordinates - Phase 6.0.4.2
    // Phase 6.0.20 Task 24 Step 3.5: Read-only Q_PROPERTY (QML cannot modify field origin)
    Q_PROPERTY(double latStart READ latStart
               NOTIFY latStartChanged BINDABLE bindableLatStart)
    Q_PROPERTY(double lonStart READ lonStart
               NOTIFY lonStartChanged BINDABLE bindableLonStart)

public:
    //WGS84 Lat Long
    double latitude, longitude;

    double prevLatitude, prevLongitude;

    // Phase 6.0.20 Task 24 Step 3.5: mPerDegreeLat moved to FormGPS Q_PROPERTY
    // mPerDegreeLon calculated locally in conversion functions (geodetic precision)

    //our current fix
    //moved to CVehicle
    Vec2 fix = Vec2(0, 0);

    Vec2 prefSpeedFix= Vec2(0, 0);


    // GPS data with safe default values (0.0 = no data yet)
    // Phase 6.0.24 Problem 18: Initialize to avoid garbage values at startup
    double altitude = 0.0;
    double speed = 0.0;
    double newSpeed = 0.0;
    double vtgSpeed = 0.0;  // Was DOUBLE_MAX - caused infinite speed display at startup

    // Heading and quality data - NMEA standard defaults
    // hdop=99.9, age=99.9 = standard NMEA values for "no data available"
    double headingTrueDual = 0.0;
    double headingTrue = 0.0;
    double hdop = 99.9;
    double age = 99.9;
    double headingTrueDualOffset = 0.0;

    int fixQuality, ageAlarm;
    int satellitesTracked;


    QString logNMEASentence;

    //StringBuilder logNMEASentence = new StringBuilder();

    explicit CNMEA(QObject *parent = 0);

    void loadSettings(void);

    // Geodetic Conversion Functions
    // Exposed to QML for coordinate transformations
    Q_INVOKABLE QVariantList convertLocalToWGS84(double northing, double easting);
    Q_INVOKABLE QVariantList convertWGS84ToLocal(double latitude, double longitude);

    void SetLocalMetersPerDegree();
    void ConvertWGS84ToLocal(double Lat, double Lon, double &outNorthing, double &outEasting);
    void ConvertLocalToWGS84(double Northing, double Easting, double &outLat, double &outLon);
    QString GetLocalToWSG84_KML(double Easting, double Northing);

    // GPS/NMEA Coordinates
    double latStart() const;
    void setLatStart(double value);
    QBindable<double> bindableLatStart();

    double lonStart() const;
    void setLonStart(double value);
    QBindable<double> bindableLonStart();


    //used to offset the antenna position to compensate for drift
    SIMPLE_BINDABLE_PROPERTY(Vec2, fixOffset)
    SIMPLE_BINDABLE_PROPERTY(double, mPerDegreeLat)

signals:
    //void setAveSpeed(double);
    void checkZoomWorldGrid(double, double);
    void latStartChanged();
    void lonStartChanged();


private:
    inline void updateMPerDegreeLat();

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CNMEA, double, m_latStart, 0, &CNMEA::latStartChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CNMEA, double, m_lonStart, 0, &CNMEA::lonStartChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CNMEA, Vec2, m_fixOffset, Vec2(0,0), &CNMEA::fixOffsetChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CNMEA, double, m_mPerDegreeLat, 0, &CNMEA::mPerDegreeLatChanged)
};

#endif // CNMEA_H
