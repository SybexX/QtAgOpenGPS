// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// For when we use qmlscene
import QtQuick 2.15

Item {

    ListModel {
        id: mymodel
        ListElement { index: 0; name: "AB 0"; isVisible: true; mode: 2}
        ListElement {index: 1; name: "Cu 33"; isVisible: true; mode: 4}
        ListElement {index: 2; name: "Piv this is a very long name"; isVisible: true; mode: 64} 
    }
    
    property var model: mymodel
    property int idx: -1
    property int newRefSide: 1
    property bool isAutoTrack: false
    property bool isAutoSnapToPivot: false
    property bool isAutoSnapped: false
    property int howManyPathsAway: 2
    
    function rowCount() {
        return mymodel.count
    }

    property int mode: 0
    property int newMode: 0
    property string newName: ""
    
    Component.onCompleted: {
        console.log(mymodel)
    }

}
