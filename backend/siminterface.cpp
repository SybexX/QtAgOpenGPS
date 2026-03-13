// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include <QCoreApplication>
#include "settingsmanager.h"
#include "siminterface.h"
#include "mainwindowstate.h"
#include "recordedpath.h"
#include "cvehicle.h"
#include "glm.h"

SimInterface *SimInterface::s_instance = nullptr;
QMutex SimInterface::s_mutex;
bool SimInterface::s_cpp_created = false;

SimInterface::SimInterface(QObject *parent)
    : QObject{parent}
{
    connect(&simTimer, &QTimer::timeout, this, &SimInterface::onTimeout, Qt::UniqueConnection);
    simTimer.stop(); //ensure it's not running
}

SimInterface *SimInterface::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new SimInterface();
        s_cpp_created = true;
        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

SimInterface *SimInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new SimInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void SimInterface::CalculateNewPositionFromBearingDistance(double lat, double lng, double bearing, double distance)
{
    // 1 degree = 0.0175 radian = M_PI / 180

    const double R = distance / 6371; // Earth Radius in Km

    double lat2 = asin((sin(lat) * cos(R)) + (cos(lat) * sin(R) * cos(bearing)));

    double lon2 = lng + atan2(sin(bearing) * sin(R) * cos(lat), cos(R) - (sin(lat) * sin(lat2)));


    //double lat2 = qAsin((qSin(M_PI / 180 * lat) * qCos(distance / R)) + (qCos(M_PI / 180 * lat) * qSin(distance / R)
    //                                                                     *qCos(M_PI / 180 * bearing)));
    //double lon2 = (M_PI / 180 * lng) + qAtan2(qSin(M_PI / 180 * bearing) * qSin(distance / R) *
    //                                          qCos(M_PI / 180 * lat), qCos(distance / R) - (qSin(M_PI / 180 * lat) * qSin(lat2)));
    latitude = glm::toDegrees(lat2);
    longitude = glm::toDegrees(lon2);
}


void SimInterface::startUp() {
    //start or reset timer

    // Ensure stable gpsHz by using precise timer, accurate to 1 ms.
    // With this timer, any deviation from expected Hz is guaranteed
    // to not be in the simulator mechanism but must be in the
    // actual calculations in FormGPS.
    simTimer.setTimerType(Qt::PreciseTimer);

    latitude = SettingsManager::instance()->gps_simLatitude();
    longitude = SettingsManager::instance()->gps_simLongitude();

    simTimer.start(100); //10 hz
}

void SimInterface::shutDown() {
    simTimer.stop();
}

bool SimInterface::isRunning() {
    return simTimer.isActive();
}


void SimInterface::onTimeout() {
    //qWarning() << "sim tick.";
    //sim.setSimStepDistance(stepDistance);
    if (RecordedPath::instance()->isDrivingRecordedPath() || (MainWindowState::instance()->isBtnAutoSteerOn() && (CVehicle::instance()->guidanceLineDistanceOff() !=32000)))
    {
        m_steerAngle = CVehicle::instance()->guidanceLineSteerAngle * 0.01;
    }

    double diff = fabs(m_steerAngle - steerangleAve);

    if (diff > 11)
    {
        if (steerangleAve >= m_steerAngle)
        {
            steerangleAve -= 6;
        }
        else steerangleAve += 6;
    }
    else if (diff > 5)
    {
        if (steerangleAve >= m_steerAngle)
        {
            steerangleAve -= 2;
        }
        else steerangleAve += 2;
    }
    else if (diff > 1)
    {
        if (steerangleAve >= m_steerAngle)
        {
            steerangleAve -= 0.5;
        }
        else steerangleAve += 0.5;
    }
    else
    {
        steerangleAve = m_steerAngle;
    }

    m_steerAngleActual = steerangleAve;

    double temp = m_stepDistance * tan(steerangleAve * 0.0165329252) / 2;
    headingTrue += temp;
    if (headingTrue > glm::twoPI) headingTrue -= glm::twoPI;
    if (headingTrue < 0) headingTrue += glm::twoPI;

    double vtgSpeed = fabs(4 * m_stepDistance * 10);
    //mf.pn.AverageTheSpeed(); TODO: done in signal handler

    temp = fabs(latitude * 100);
    temp -= ((int)(temp));
    temp *= 00;
    double altitude = temp + 200;

    temp = fabs(longitude * 100);
    temp -= ((int)(temp));
    temp *= 100;
    altitude += temp;

    //Calculate the next Lat Long based on heading and distance
    CalculateNewPositionFromBearingDistance(glm::toRadians(latitude), glm::toRadians(longitude), headingTrue, m_stepDistance / 1000.0);

    emit newPosition(vtgSpeed, glm::toDegrees(headingTrue), latitude, longitude, 0.7f, altitude, 12);

    if (isAccelForward)
    {
        isAccelBack = false;
        m_stepDistance = m_stepDistance + 0.02;
        if (m_stepDistance > 0.12) isAccelForward = false;
    }

    if (isAccelBack)
    {
        isAccelForward = false;
        m_stepDistance = m_stepDistance - 0.01;
        if (m_stepDistance < -0.06) isAccelBack = false;
    }
}

void SimInterface::speedup() {
    //okay to access m_stepDistance because other classes never
    //read from this property; they only might write to it.
    if (m_stepDistance < 0)
    {
        m_stepDistance = 0;
        return;
    }
    if (m_stepDistance < 0.2 ) m_stepDistance = m_stepDistance + 0.02;
    else
        m_stepDistance = m_stepDistance * 1.15;

    if (m_stepDistance > 7.5) m_stepDistance = 7.5;
    }

void SimInterface::slowdown() {
    if (m_stepDistance < 0.2 && m_stepDistance > -0.51) m_stepDistance = m_stepDistance - 0.02;
    else m_stepDistance = m_stepDistance * 0.8;
    if (m_stepDistance < -0.5) m_stepDistance = -0.5;
}

void SimInterface::stop() {
    m_stepDistance = 0;
}

void SimInterface::reverse() {
    m_stepDistance = 0;
    isAccelBack = true;
}

void SimInterface::forward() {
    m_stepDistance = 0;
    isAccelForward = true;
}

void SimInterface::reset() {
    //reset simulator to starting position
    shutDown();
    startUp();
}

void SimInterface::rotate() {
    headingTrue += M_PI;
    if (headingTrue > M_2_PI) {
        headingTrue -= M_2_PI;
    }
}
