// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//

//This forwards to the real Settings singleton from AOG module
//Provides backward compatibility for "import Settings"

import QtQuick
import AOG

pragma singleton

// Forward all properties to the real Settings singleton
QtObject {
    // Expose the real Settings singleton
    property alias settings: newSettings
    
    Settings {
        id: newSettings
    }
    
    // Forward all commonly used properties for backward compatibility
	property double ab_lineLength: 1600
	property int ardMac_hydLowerTime: 4
	property int ardMac_hydRaiseTime: 3
	property bool ardMac_isDanFoss: false
	property bool ardMac_isHydEnabled: 0
	property int ardMac_setting0: 0
	property int ardMac_user1: 1
	property int ardMac_user2: 2
	property int ardMac_user3: 3
	property int ardMac_user4: 4
	property int ardSteer_maxPulseCounts: 3
	property double ardSteer_maxSpeed: 20
	property double ardSteer_minSpeed: 0
	property int ardSteer_setting0: 56
	property int ardSteer_setting1: 0
	property int ardSteer_setting2: 0
	property int as_ackerman: 100
	property int as_countsPerDegree: 110
	property int as_deadZoneDelay: 5
	property int as_deadZoneDistance: 1
	property int as_deadZoneHeading: 10
	property int as_functionSpeedLimit: 12
	property double as_guidanceLookAheadTime: 1.5
	property int as_highSteerPWM: 180
	property bool as_isAutoSteerAutoOn: false
	property bool as_isConstantContourOn: false
	property bool as_isSteerInReverse: false
	property int as_Kp: 50
	property int as_lowSteerPWM: 30
	property double as_maxSteerSpeed: 15
	property int as_minSteerPWM: 25
	property double as_minSteerSpeed: 0
	property double as_modeMultiplierStanley: 0.6
	property int as_modeTime: 1
	property double as_modeXTE: 0.1
	property int as_numGuideLines: 10
	property double as_sideHillCompensation: 0
	property double as_snapDistance: 20
	property double as_snapDistanceRef: 5
	property double as_uTurnCompensation: 1
	property int as_uTurnSmoothing: 14
	property double as_wasOffset: 3
	property bool bnd_isDrawPivot: true
	property string brand_HBrand: "AgOpenGPS"
	property string brand_TBrand: "AGOpenGPS"
	property string brand_WDBrand: "AgOpenGPS"
	property string cam_camLink: "rtsp://192.168.0.138:1945"
	property bool color_isMultiColorSections: false
	property color color_sec01: "#f9160a"
	property color color_sec02: "#4454fe"
	property color color_sec03: "#08f308"
	property color color_sec04: "#e906e9"
	property color color_sec05: "#c8bf56"
	property color color_sec06: "#00fcf6"
	property color color_sec07: "#9024f6"
	property color color_sec08: "#e86615"
	property color color_sec09: "#ffa0aa"
	property color color_sec10: "#cdccf6"
	property color color_sec11: "#d5efbe"
	property color color_sec12: "#f7c8f7"
	property color color_sec13: "#fdf190"
	property color color_sec14: "#bbfafa"
	property color color_sec15: "#e3c9f9"
	property color color_sec16: "#f7e5d7"
	property int display_antiAliasSamples: 0
	property bool display_autoDayNight: false
	property int display_brightness: 40
	property int display_brightnessSystem: 40
	property string display_buttonOrder: "0,1,2,3,4,5,6,7"
	property double display_camPitch: -62
	property double display_camSmooth: 50
	property double display_camZoom: 9
	property color display_colorDayBackground: "#f5f5f5"
	property color display_colorDayBorder: "#d7e4f2"
	property color display_colorDayFrame: "#d2d2e6"
	property color display_colorFieldDay: "#919191"
	property color display_colorFieldNight: "#3c3c3c"
	property color display_colorNightBackground: "#323241"
	property color display_colorNightBorder: "#d2d2e6"
	property color display_colorNightFrame: "#323241"
	property color display_colorSectionsDay: "#1b97a0"
	property color display_colorSectionsNight: "#1b6464"
	property color display_colorTextDay: "Black"
	property color display_colorTextNight: "#e6e6e6"
	property color display_colorVehicle: "White"
	property string display_customColors: "-62208,-12299010,-16190712,-1505559,-3621034,-16712458,-7330570,-1546731,-24406,-3289866,-2756674,-538377,-134768,-4457734,-1848839,-530985"
	property string display_customSectionColors: "-62208,-12299010,-16190712,-1505559,-3621034,-16712458,-7330570,-1546731,-24406,-3289866,-2756674,-538377,-134768,-4457734,-1848839,-530985"
	property string display_features: ""
	property bool display_isAutoOffAgIO: true
	property bool display_isAutoStartAgIO: true
	property bool display_isBrightnessOn: false
	property bool display_isDayMode: true
	property bool display_isHardwareMessages: false
	property bool display_isKeyboardOn: true
	property bool display_isLineSmooth: false
	property bool display_isLogElevation: false
	property bool display_isSectionLinesOn: true
	property bool display_isShutDownWhenNoPower: false
	property bool display_isStartFullscreen: false
	property bool display_isSvennArrowOn: false
	property bool display_isTermsAccepted: false
	property bool display_isTextureOn: true
	property bool display_isVehicleImage: true
	property int display_lightbarCmPerPixel: 5
	property double display_lineWidth: 2
	property point display_panelSimLocation: Qt.point(97, 600)
	property bool display_showBack: false
	property bool display_topTrackNum: false
	property double display_triangleResolution: 1
	property bool display_useTrackZero: false
	property double display_vehicleOpacity: 100
	property double f_boundaryTriggerDistance: 1
	property string f_currentDir: ""
	property bool f_isRemoteWorkSystemOn: false
	property bool f_isSteerWorkSwitchEnabled: false
	property bool f_isSteerWorkSwitchManualSections: false
	property bool f_isWorkSwitchActiveLow: true
	property bool f_isWorkSwitchEnabled: false
	property bool f_isWorkSwitchManualSections: false
	property double f_minHeadingStepDistance: 0.5
	property double f_userTotalArea: 0
	property bool feature_isABLineOn: true
	property bool feature_isABSmoothOn: false
	property bool feature_isAgIOOn: true
	property bool feature_isAutoSectionOn: true
	property bool feature_isAutoSteerOn: true
	property bool feature_isBndContourOn: false
	property bool feature_isBoundaryOn: true
	property bool feature_isContourOn: true
	property bool feature_isCurveOn: true
	property bool feature_isCycleLinesOn: true
	property bool feature_isHeadlandOn: true
	property bool feature_isHideContourOn: false
	property bool feature_isLateralOn: true
	property bool feature_isManualSectionOn: true
	property bool feature_isOffsetFixOn: false
	property bool feature_isRecPathOn: false
	property bool feature_isSteerModeOn: true
	property bool feature_isTramOn: false
	property bool feature_isUTurnOn: true
	property bool feature_isWebCamOn: false
	property bool feature_isYouTurnOn: true
	property int gps_ageAlarm: 20
	property double gps_dualHeadingOffset: 0
	property double gps_dualReverseDetectionDistance: 0.25
	property string gps_fixFromWhichSentence: "GGA"
	property double gps_forwardComp: 0.15
	property string gps_headingFromWhichSource: "Fix"
	property bool gps_isRTK: false
	property bool gps_isRTKKillAutoSteer: false
	property double gps_jumpFixAlarmDispance: 0
	property double gps_minimumStepLimit: 0.05
	property double gps_reverseComp: 0.3
	property double gps_simLatitude: 53.4360564
	property double gps_simLongitude: -111.160047
	property int gps_udpWatchMSec: 50
	property bool headland_isSectionControlled: true
	property double imu_fusionWeight2: 0.06
	property bool imu_invertRoll: false
	property bool imu_isDualAsIMU: false
	property bool imu_isHeadingCorrectionFromAutoSteer: false
	property bool imu_isReverseOn: true
	property double imu_pitchZeroX16: 0
	property double imu_rollFilter: 0
	property double imu_rollZero: 0
	property point jobMenu_location: Qt.point(200, 200)
	property string jobMenu_size: "640, 530"
	property string key_hotKeys: "ACFGMNPTYVW12345678"
	property bool menu_isCompassOn: true
	property bool menu_isGridOn: true
	property bool menu_isLightBarNotSteerBar: false
	property bool menu_isLightBarOn: true
	property bool menu_isMetric: true
	property bool menu_isOGLZoom: false
	property bool menu_isPureOn: true
	property bool menu_isSideGuideLines: false
	property bool menu_isSimulatorOn: true
	property bool menu_isSkyOn: true
	property bool menu_isSpeedoOn: false
	property var relay_pinConfig: [  1,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  ]
	property bool section_isFast: false
	property double section_position1: -2
	property double section_position10: 0
	property double section_position11: 0
	property double section_position12: 0
	property double section_position13: 0
	property double section_position14: 0
	property double section_position15: 0
	property double section_position16: 0
	property double section_position17: 0
	property double section_position2: -1
	property double section_position3: 1
	property double section_position4: 2
	property double section_position5: 0
	property double section_position6: 0
	property double section_position7: 0
	property double section_position8: 0
	property double section_position9: 0
	property bool seed_blockageIsOn: false
	property int seed_blockCountMax: 1000
	property int seed_blockCountMin: 100
	property int seed_blockRow1: 16
	property int seed_blockRow2: 16
	property int seed_blockRow3: 16
	property int seed_blockRow4: 0
	property int seed_numRows: 32
	property bool sound_autoSteerSound: true
	property bool sound_isHydLiftOn: true
	property bool sound_isSectionOn: true
	property bool sound_isUturnOn: true
	property double tool_defaultSectionWidth: 2
	property bool tool_isDirectionMarkers: true
	property bool tool_isDisplayTramControl: true
	property bool tool_isSectionOffWhenOut: true
	property bool tool_isSectionsNotZones: true
	property bool tool_isTBT: false
	property bool tool_isToolFront: false
	property bool tool_isToolRearFixed: false
	property bool tool_isToolTrailing: true
	property bool tool_isTramOuterInverted: false
	property int tool_numSectionsMulti: 20
	property double tool_sectionWidthMulti: 0.5
	property double tool_toolTrailingHitchLength: -2.5
	property double tool_trailingToolToPivotLength: 0
	property var tool_zones: [  2,10,20,0,0,0,0,0,0  ]
	property double tram_alpha: 0.8
	property int tram_basedOn: 0
	property bool tram_isTramOnBackBuffer: true
	property double tram_offset: 0
	property int tram_passes: 1
	property int tram_skips: 0
	property double tram_snapAdj: 1
	property double tram_width: 24
	property double vehicle_antennaHeight: 3
	property double vehicle_antennaOffset: 0
	property double vehicle_antennaPivot: 0.1
	property double vehicle_goalPointAcquireFactor: 0.9
	property double vehicle_goalPointLookAhead: 3
	property double vehicle_goalPointLookAheadHold: 3
	property double vehicle_goalPointLookAheadMult: 1.5
	property double vehicle_hitchLength: -1
	property double vehicle_hydraulicLiftLookAhead: 2
	property bool vehicle_isMachineControlToAutoSteer: false
	property bool vehicle_isPivotBehindAntenna: true
	property bool vehicle_isStanleyUsed: false
	property bool vehicle_isSteerAxleAhead: true
	property double vehicle_lookAheadMinimum: 2
	property double vehicle_maxAngularVelocity: 0.64
	property double vehicle_maxSteerAngle: 30
	property double vehicle_minCoverage: 100
	property double vehicle_minTurningRadius: 8.1
	property double vehicle_numSections: 3
	property double vehicle_panicStopSpeed: 0
	property double vehicle_purePursuitIntegralGainAB: 0
	property double vehicle_slowSpeedCutoff: 0.5
	property double vehicle_stanleyDistanceErrorGain: 1
	property double vehicle_stanleyHeadingErrorGain: 1
	property double vehicle_stanleyIntegralDistanceAwayTriggerAB: 0.25
	property double vehicle_stanleyIntegralGainAB: 0
	property double vehicle_tankTrailingHitchLength: -3
	property double vehicle_toolLookAheadOff: 0.5
	property double vehicle_toolLookAheadOn: 1
	property double vehicle_toolOffDelay: 0
	property double vehicle_toolOffset: 0
	property double vehicle_toolOverlap: 0
	property double vehicle_toolWidth: 4
	property double vehicle_trackWidth: 1.9
	property string vehicle_vehicleName: "Default Vehicle"
	property int vehicle_vehicleType: 0
	property double vehicle_wheelbase: 3.3
	property rect window_abDrawLocation: Qt.rect(0,0,1022,742)
	property rect window_bingMapSize: Qt.rect(0,0,965,700)
	property int window_bingZoom: 15
	property point window_buildTracksLocation: Qt.point(40, 40)
	property rect window_formNudgeLocation: Qt.rect(0,0,200, 200)
	property point window_gridLocation: Qt.point(20, 20)
	property point window_gridSize: Qt.point(400, 400)
	property string window_headAcheSize: "1022, 742"
	property string window_headlineSize: "1022, 742"
	property bool window_isKioskMode: false
	property bool window_isShutdownComputer: false
	property point window_location: Qt.point(30, 30)
	property string window_mapBndSize: "1022, 742"
	property bool window_maximized: false
	property bool window_minimized: false
	property point window_quickABLocation: Qt.point(100, 100)
	property string window_rateMapSize: "1022, 742"
	property int window_rateMapZoom: 15
	property string window_size: "1005, 730"
	property point window_steerSettingsLocation: Qt.point(40, 40)
	property string window_tramLineSize: "921, 676"
	property double youturn_distanceFromBoundary: 2
	property double youturn_extensionLength: 20
	property double youturn_radius: 8.1
	property double youturn_skipWidth: 1
	property double youturn_style: 0
	property double youturn_toolWidths: 2
	property double youturn_youMoveDistance: 0.25
	property var test_testStrings: [ "one", "two", "trheep", "four}"  ]
	property var test_testDoubles: [ 1.2, 1.3, 1.4 ]
	property color test_testColorRgbF: "#ff7f7f33"
}
