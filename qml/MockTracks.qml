// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// For when we use qmlscene
import QtQuick 2.15

Item {
    id: mockTracks

    property var model: [ {index: 0, name: "AB 0", isVisible: true, mode: 2},
             {index: 1, name: "Cu 33", isVisible: true, mode: 4},
             {index: 2, name: "Piv", isVisible: true, mode: 4} ]
    
    property int idx: -1
    property int newRefSide: 1
    property bool isAutoTrack: false
    property bool isAutoSnapToPivot: false
    property bool isAutoSnapped: false
    property int howManyPathsAway: 2
    property int mode: 0
    property int newMode: 0
    property string newName: ""
}
