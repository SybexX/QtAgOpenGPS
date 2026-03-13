// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
//
import QtQuick
import AOG
//import Settings
import "components" as Comp

Comp.OutlineText {
    id: tracknum
    font.pixelSize: 24

    property int currentTrack: 0
    property bool useDirNames: false
    property double trackHeading: 0

    // Phase 6.0.43: Parallel line number property
    property int parallelLineNum: TracksInterface.howManyPathsAway

    function generate_text(track_no) {
        var track_num = track_no

        // Threading Phase 1: Track numbering configuration
        if (SettingsManager.display_useTrackZero) {
            if (track_num >  0)
                track_num -=1
        }

        if(track_num === 0)
            return qsTr("Track", "Track as in \"track number.\"") + " " + Number(0).toLocaleString(Qt.locale(),'f',0)

        var dir = ""
        if (useDirNames === true) {
            dir = Utils.findTrackDirection(trackHeading, track_num)
        } else {
            if (track_num > 0)
                dir = qsTr("R", "Abbreviation for right-hand direction.")
            else
                dir = qsTr("L", "Abbreviation for left-hand direction.")
        }

        return qsTr("Track", "Track as in \"track number.\"") + " " +
                Number(Math.abs(currentTrack)).toLocaleString(Qt.locale(),'f',0) + " " +
                dir
    }

    // Phase 6.0.43: Generate parallel line number text (matching C# AgOpenGPS format)
    function generate_parallel_line_text(lineNum) {
        if (lineNum === 0) {
            return "0"
        } else if (lineNum > 0) {
            // Phase 6.0.43 BUG FIX: Add +1 for display (matching C# UI layer logic)
            // This adjustment belongs in UI layer, NOT backend (OpenGL.Designer.cs:2470)
            return (lineNum + 1).toString() + "R"
        } else {
            return (-lineNum).toString() + "L"
        }
    }

    //TODO: check settings to see if we should have a track 0

    // Phase 6.0.43: Display parallel line number instead of track index
    text: generate_parallel_line_text(parallelLineNum)

    color: "white"
    //visible: (aogInterface.currentABLine > -1)
}
