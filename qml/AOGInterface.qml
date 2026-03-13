// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Link between the backend and QML. 
import QtQuick
import QtQuick.Controls.Fusion
import AOG

/* Migrating away from the use of this QML Item.  Acessing properties and
  methods of various backend singletons mainly going forward. See backend/
  singleton class files.
*/

Item {
    // Expose theme colors as aog properties for compatibility
    property color backgroundColor: theme.backgroundColor
    property color textColor: theme.textColor
    property color borderColor: theme.borderColor
    property color blackDayWhiteNight: theme.blackDayWhiteNight
    property color whiteDayBlackNight: theme.whiteDayBlackNight
    property string timeTilFinished: ""
    property string workRate: "value"
    property string percentOverlap: "value"
    property string percentLeft: "value"

    //property int lblmodeActualXTE: 0
    property double lblmodeActualHeadingError: 0 // dead code or to be implemed ???


    // Qt 6.8 COMPUTED PROPERTIES: Reactive bindings based on TracksInterface.idx and .mode
    // TrackMode enum: None=0, AB=2, Curve=4, bndTrackOuter=8, bndTrackInner=16, bndCurve=32, waterPivot=64
    readonly property int currentABLine: {
        let i = TracksInterface.idx
        if (i >= 0 && TracksInterface.mode === 2)  // TrackMode::AB = 2
            return i
        return -1
    }

    readonly property int currentABCurve: {
        let i = TracksInterface.idx
        if (i >= 0 && TracksInterface.mode === 4)  // TrackMode::Curve = 4
            return i
        return -1
    }

    property double currentABLine_heading: 0 //TODO delete or move to interfaces/LinesInterface.qml.  seems to be unused

    // ⚡ PHASE 6.3.0: Q_PROPERTY CONVERSIONS - Missing AOGInterface properties
    // Added for conversion from InterfaceProperty to Q_PROPERTY
    // These properties replace the InterfaceProperty declarations in headers
    property double userSquareMetersAlarm: 0  // From classes/cfielddata.h:39

}
