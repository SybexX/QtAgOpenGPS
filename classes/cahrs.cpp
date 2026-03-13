#include "cahrs.h"
#include "classes/settingsmanager.h"
#include <QDebug>

CAHRS::CAHRS(QObject *parent) : QObject(parent)
{
    loadSettings();
}

void CAHRS::loadSettings()
{
    rollZero = SettingsManager::instance()->imu_rollZero();
    rollFilter = SettingsManager::instance()->imu_rollFilter();

    //is the auto steer in auto turn on mode or not
    isAutoSteerAuto = true; //SettingsManager::instance()->as_isAutoSteerAutoOn();
    isRollInvert = SettingsManager::instance()->imu_invertRoll();
    isDualAsIMU = SettingsManager::instance()->imu_isDualAsIMU();
    isReverseOn = SettingsManager::instance()->imu_isReverseOn();

    //the factor for fusion of GPS and IMU
    forwardComp = SettingsManager::instance()->gps_forwardComp();
    reverseComp = SettingsManager::instance()->gps_reverseComp();
    fusionWeight = SettingsManager::instance()->imu_fusionWeight2();

}

void CAHRS::changeImuHeading(double newImuHeading) {
    imuHeading = newImuHeading;
    qDebug() << "changed imuHeading to " << newImuHeading;
}

void CAHRS::changeImuRoll(double newImuRoll) {
    //new roll number
    imuRoll = newImuRoll;
    qDebug() << "changed imuRoll to " << newImuRoll;
}
