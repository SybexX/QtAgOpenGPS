// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// A Textfield input box where we can enter lat/lon
import QtQuick
import QtQuick.Controls.Fusion

TextField {
    id: latlon_textfield

    property bool suppress_onchange: false
    property color defaultTextColor

    signal manualTextChanged()

    Component.onCompleted: {
        defaultTextColor = color
    }

    function set_without_onchange(new_value) {
        suppress_onchange = true
        text = Number(new_value).toLocaleString(Qt.locale(), 'f', 9)
        suppress_onchange = false
    }

    // Save the previous text if validation is not passed.
    property string _lastValidText: ""

    onTextChanged: {
        // Check format
        var isValid = /^-?(\d+\.?\d*|\.\d+)?$/.test(text)

        if (isValid) {
            _lastValidText = text
        } else {
            // Returning the last valid text
            text = _lastValidText
            // Restoring the cursor position
            cursorPosition = _lastValidText.length
            return
        }

        var value = Number(text)
        if ((value > 180) || (value < -180)) {
            color = "red"
        } else {
            color = defaultTextColor
        }

        if (!suppress_onchange) {
            manualTextChanged()
        }
    }

    onEditingFinished: {
        var value = Number(text)
        if (value > 180) {
            text = "180.000000000"
        } else if (value < -180) {
            text = "-180.000000000"
        }
    }

    selectByMouse: true
}
