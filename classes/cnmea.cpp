#include <QCoreApplication>
#include <QLoggingCategory>
#include <math.h>
#include "cnmea.h"
#include "vec2.h"
#include "glm.h"
#include "classes/settingsmanager.h"
#include "backend.h"
#include "worldgrid.h"

Q_LOGGING_CATEGORY (cnmea_log, "cnmea.qtagopengps")

CNMEA::CNMEA(QObject *parent) : QObject(parent)
{
    // Phase 6.0.24 Problem 18: Initialize GPS position variables to safe defaults
    // Defense in depth - ensures coherent values even if class member initialization forgotten
    latitude = 0.0;
    longitude = 0.0;
    prevLatitude = 0.0;
    prevLongitude = 0.0;

    // GPS quality indicators - 0 = Invalid fix, no satellites
    fixQuality = 0;
    satellitesTracked = 0;

    loadSettings();
}

double CNMEA::latStart() const { return m_latStart.value(); }
void CNMEA::setLatStart(double latStart) {
    qDebug(cnmea_log) << "[GEODETIC_DEBUG] setLatStart called with value:" << latStart;
    m_latStart = latStart;  // ✅ Qt 6.8 Official Doc - Direct assignment emits signal
    updateMPerDegreeLat();
    qDebug(cnmea_log) << "[GEODETIC_DEBUG] After set - m_latStart:" << m_latStart.value() << "mPerDegreeLat:" << m_mPerDegreeLat;
}
QBindable<double> CNMEA::bindableLatStart() { return &m_latStart; }

double CNMEA::lonStart() const { return m_lonStart.value(); }
void CNMEA::setLonStart(double lonStart) {
    qDebug(cnmea_log) << "[GEODETIC_DEBUG] setLonStart called with value:" << lonStart;
    m_lonStart = lonStart;  // ✅ Qt 6.8 Official Doc - Direct assignment emits signal
}
QBindable<double> CNMEA::bindableLonStart() { return &m_lonStart; }

void CNMEA::loadSettings(void)
{
    ageAlarm  = SettingsManager::instance()->gps_ageAlarm();
}

//moved to CVehicle
/*
void CNMEA::AverageTheSpeed()
{
    mf.avgSpeed = (mf.avgSpeed * 0.75) + (speed * 0.25);
}
*/

void CNMEA::SetLocalMetersPerDegree()
{
    double northing, easting;

    ConvertWGS84ToLocal(latitude, longitude, northing, easting);

    WorldGrid::instance()->set_fixEasting(easting);
    WorldGrid::instance()->set_fixNorthing(northing);
}


// Geodetic Conversion Functions - Phase 6.0.20 Task 24 Step 3.5
// Wrappers for QML - delegate to CNMEA for actual conversion logic
QVariantList CNMEA::convertLocalToWGS84(double northing, double easting) {
    double outLat, outLon;
    // Call CNMEA conversion function (single source of truth)
    ConvertLocalToWGS84(northing, easting, outLat, outLon);
    // Return [latitude, longitude] as QVariantList for QML
    return QVariantList() << outLat << outLon;
}

QVariantList CNMEA::convertWGS84ToLocal(double latitude, double longitude) {
    double outNorthing, outEasting;
    // Call CNMEA conversion function (single source of truth)
    ConvertWGS84ToLocal(latitude, longitude, outNorthing, outEasting);
    // Return [northing, easting] as QVariantList for QML
    return QVariantList() << outNorthing << outEasting;
}

void CNMEA::ConvertWGS84ToLocal(double Lat, double Lon, double &outNorthing, double &outEasting)
{
    // Phase 6.0.20 Task 24 Step 3.5: Calculate mPerDegreeLon locally for geodetic precision
    double mPerDegreeLon = 111412.84 * cos(Lat * 0.01745329251994329576923690766743)
                         - 93.5 * cos(3.0 * Lat * 0.01745329251994329576923690766743)
                         + 0.118 * cos(5.0 * Lat * 0.01745329251994329576923690766743);

    outNorthing = (Lat - m_latStart) * m_mPerDegreeLat;
    outEasting = (Lon - m_lonStart) * mPerDegreeLon;

    //Northing += mf.RandomNumber(-0.02, 0.02);
    //Easting += mf.RandomNumber(-0.02, 0.02);
}

void CNMEA::ConvertLocalToWGS84(double Northing, double Easting, double &outLat, double &outLon)
{
    outLat = ((Northing + fixOffset().northing) / m_mPerDegreeLat) + m_latStart;

    // Phase 6.0.20 Task 24 Step 3.5: Calculate mPerDegreeLon locally with output latitude
    double mPerDegreeLon = 111412.84 * cos(outLat * 0.01745329251994329576923690766743)
                         - 93.5 * cos(3.0 * outLat * 0.01745329251994329576923690766743)
                         + 0.118 * cos(5.0 * outLat * 0.01745329251994329576923690766743);

    outLon = ((Easting + fixOffset().easting) / mPerDegreeLon) + m_lonStart;
}

// Geodetic Conversion - Phase 6.0.20 Task 24 Step 3.5
// mPerDegreeLat getter is inline in .h (simple member variable, no BINDABLE overhead)

inline void CNMEA::updateMPerDegreeLat() {
    // WGS84 geodetic formula for meters per degree latitude
    // Based on latStart (fixed reference point for the field)
    double latStart = m_latStart.value();
    m_mPerDegreeLat = 111132.92 - 559.82 * cos(2.0 * latStart * 0.01745329251994329576923690766743)
                    + 1.175 * cos(4.0 * latStart * 0.01745329251994329576923690766743)
                    - 0.0023 * cos(6.0 * latStart * 0.01745329251994329576923690766743);
    // Direct assignment - no signal emission (C++ only, not exposed to QML)
}


QString CNMEA::GetLocalToWSG84_KML(double Easting, double Northing)
{
    double latStart = m_latStart;
    double lonStart = m_lonStart;
    double mPerDegreeLat = m_mPerDegreeLat;

    double Lat = (Northing / mPerDegreeLat) + latStart;

    // Phase 6.0.20 Task 24 Step 3.5: Calculate mPerDegreeLon locally with calculated latitude
    double mPerDegreeLon = 111412.84 * cos(Lat * 0.01745329251994329576923690766743)
                         - 93.5 * cos(3.0 * Lat * 0.01745329251994329576923690766743)
                         + 0.118 * cos(5.0 * Lat * 0.01745329251994329576923690766743);

    double Lon = (Easting / mPerDegreeLon) + lonStart;

    return QString("%1, %2, 0").arg(Lon,0,'g',7).arg(Lat,0,'g',7); //shouldn't use locale
}
