// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// This loads the setting (or some of them) into variables, that we can access later
#include "formgps.h"
#include "classes/settingsmanager.h"
#include "backend.h"
#include "backendaccess.h"

void FormGPS::loadSettings()
{
    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_YT(yt);

    isMetric = SettingsManager::instance()->menu_isMetric();

    isUTurnOn = SettingsManager::instance()->feature_isUTurnOn();
    isLateralOn = SettingsManager::instance()->feature_isLateralOn();

    pn.headingTrueDualOffset = SettingsManager::instance()->gps_dualHeadingOffset();

    frameDayColor = SettingsManager::instance()->display_colorDayFrame();
    frameNightColor = SettingsManager::instance()->display_colorNightFrame();
    sectionColorDay = SettingsManager::instance()->display_colorSectionsDay();
    fieldColorDay = SettingsManager::instance()->display_colorFieldDay();
    fieldColorNight = SettingsManager::instance()->display_colorFieldNight();

    //check color for 255, reset it to properties
    //Properties.Settings.Default.setDisplay_colorDayFrame = frameDayColor;
    //Properties.Settings.Default.setDisplay_colorNightFrame = frameNightColor;
    //Properties.Settings.Default.setDisplay_colorSectionsDay = sectionColorDay;
    //Properties.Settings.Default.setDisplay_colorFieldDay = fieldColorDay;
    //Properties.Settings.Default.setDisplay_colorFieldNight = fieldColorNight;

    isSkyOn = SettingsManager::instance()->menu_isSkyOn();
    isTextureOn = SettingsManager::instance()->display_isTextureOn();

    isGridOn = SettingsManager::instance()->menu_isGridOn();
    isBrightnessOn = SettingsManager::instance()->display_isBrightnessOn();

    isCompassOn = SettingsManager::instance()->menu_isCompassOn();
    isSpeedoOn = SettingsManager::instance()->menu_isSpeedoOn();
    isSideGuideLines = SettingsManager::instance()->menu_isSideGuideLines();
    isSvennArrowOn = SettingsManager::instance()->display_isSvennArrowOn();

    lightbarCmPerPixel = SettingsManager::instance()->display_lightbarCmPerPixel();

    //isLogNMEA = SettingsManager::instance()->getValue("menu/isLogNMEA;
    isPureDisplayOn = SettingsManager::instance()->menu_isPureOn();

    isAutoStartAgIO = SettingsManager::instance()->display_isAutoStartAgIO();

    vehicleOpacity = SettingsManager::instance()->display_vehicleOpacity() * 0.01;
    vehicleOpacityByte = (char)(255 * (SettingsManager::instance()->display_vehicleOpacity() * 0.01));
    isVehicleImage = SettingsManager::instance()->display_isVehicleImage();

    //TODO: custom colors for display

    //TODO: check for 255
    textColorDay = SettingsManager::instance()->display_colorTextDay();
    textColorNight = SettingsManager::instance()->display_colorTextNight();

    vehicleColor = SettingsManager::instance()->display_colorVehicle();

    isLightbarOn = SettingsManager::instance()->menu_isLightBarOn();


    //hotkeys = Properties.Settings.Default.setKey_hotkeys.ToCharArray();
    // udpWatchLimit = SettingsManager::instance()->gps_udpWatchMSec(); // ❌ REMOVED - Phase 4.6: No UDP FormGPS
    //check for 255
    //TODO
    //string[] words = Properties.Settings.Default.setDisplay_customColors.Split(',');

    isRTK = SettingsManager::instance()->gps_isRTK();
    isRTK_KillAutosteer = SettingsManager::instance()->gps_isRTKKillAutoSteer();

    pn.ageAlarm = SettingsManager::instance()->gps_ageAlarm();

    isConstantContourOn = SettingsManager::instance()->as_isConstantContourOn();
    isSteerInReverse = SettingsManager::instance()->as_isSteerInReverse();

    Backend::instance()->set_guidanceLookAheadTime(SettingsManager::instance()->as_guidanceLookAheadTime());

    //gyd pulls directly from settings
    //gyd.sideHillCompFactor = property_setAS_sideHillComp;

    bnd.UpdateFieldBoundaryGUIAreas();

    isStanleyUsed = SettingsManager::instance()->vehicle_isStanleyUsed();
    isDay = SettingsManager::instance()->display_isDayMode();

    tool.loadSettings();
    if (tool.isSectionsNotZones){
        tool.sectionSetPositions();
        tool.sectionCalcWidths();
    } else {
        tool.sectionCalcMulti();
    }

    //disable youturn buttons
    headingFromSource = SettingsManager::instance()->gps_headingFromWhichSource();

    // PHASE 6.0.35 FIX: Load GPS heading thresholds from settings (not hardcoded)
    // C# original: loaded from Settings.Default.setF_minHeadingStepDistance (default 0.5m)
    minHeadingStepDist = SettingsManager::instance()->f_minHeadingStepDistance();
    gpsMinimumStepDistance = SettingsManager::instance()->gps_minimumStepLimit();

    //load various saved settings or properties into the support classes
    ahrs.loadSettings();
    pn.loadSettings();
    CVehicle::instance()->loadSettings();
    yt.loadSettings();
}
