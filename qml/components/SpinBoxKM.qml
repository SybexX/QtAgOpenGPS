// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//import Settings
import QtQuick
import "../" //to pick up Utils
import AOG

//This is a spinbox for displaying dimensions that are either
//cm or inches

//The from and to values are *cm*, but the final value output is in metres always

Item {
    id: spinBoxKM
    property int from //these are in km
    property double value //kilometers
    property int to //kilometers
    property int stepSize: 1
    property bool editable: true
    property string text: ""

    property double boundValue

    signal valueModified()

    width: spinner.width
    height: spinner.height

    //set the spinner value without triggering valueChanged
    function setValue(value) {
        spinner.setValue(Utils.km_to_mi(value))
    }

    onBoundValueChanged: {
        value = boundValue
    }

    // Qt 6.8 QProperty + BINDABLE: Update on initialization
    Component.onCompleted: spinner.value = Utils.km_to_mi(value)

    SpinBoxCustomized {
        id: spinner
        from: Utils.km_to_mi(spinBoxKM.from)
        to: Utils.km_to_mi(spinBoxKM.to)
        editable: spinBoxKM.editable
        text: spinBoxKM.text
        value: Utils.km_to_mi(spinBoxKM.value) // should be in kilometers!
        stepSize: spinBoxKM.stepSize
        anchors.fill: parent

        onValueModified: {
            spinBoxKM.value = Utils.mi_to_km(value)
            spinBoxKM.valueModified()
        }
    }
}
