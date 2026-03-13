// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// RecordedPath Interface - Phase 6.0.20 Migration to Q_INVOKABLE
// Signals removed - now using direct Q_INVOKABLE calls to FormGPS
import QtQuick

Item {
    id: recordedPathInterface

    // Properties bindable to FormGPS via Qt 6.8 BINDABLE pattern
    property bool isDrivingRecordedPath: aog.isDrivingRecordedPath
    property string currentPathName: aog.recordedPathName

    property var recordedPathList: [
        { index: 0, name: "test1" },
        { index: 1, name: "test2" }
    ]

    // SIGNALS REMOVED - Phase 6.0.20
    // All actions now use Q_INVOKABLE direct calls:
    // - aog.recordedPathUpdateLines()
    // - aog.recordedPathOpen(name)
    // - aog.recordedPathDelete(name)
    // - aog.recordedPathStartDriving()
    // - aog.recordedPathStopDriving()
    // - aog.recordedPathClear()
}
