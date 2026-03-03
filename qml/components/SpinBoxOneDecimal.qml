// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Spinbox for displaying one decimal only.
import QtQuick
import QtQuick.Controls.Fusion

Item {
    id: spinBox_singledigit
    property double from: 0
    property double value: 1
    property double to: 10 * 10
    property string text: ""
	property int stepSize: 1
	property int decimals: 1
    property bool editable: true
    property double boundValue
    width: spinner.width
    height: spinner.height + (spin_message.length > 0 ? spin_message.height : 0) + (text.length > 0 ? 20 : 0)

    signal valueModified()

    function setValue(newvalue) {
        spinner.value = newvalue * 10
        spinBox_singledigit.value = newvalue
    }

    onBoundValueChanged: {
        value = boundValue
    }

    SpinBox {
        id: spinner
        from: spinBox_singledigit.from * 10
        to: spinBox_singledigit.to *10
        editable: spinBox_singledigit.editable
        value: spinBox_singledigit.value * 10
		stepSize: spinBox_singledigit.stepSize
        //anchors.horizontalCenter: parent.horizontalCenter
        //anchors.verticalCenter: parent.verticalCenter
        height: 40 * theme.scaleHeight
        width: 150 * theme.scaleWidth
        property int decimals: spinBox_singledigit.decimals

        contentItem: TextInput {
            id: text_input
            text: spinner.textFromValue(spinner.value, spinner.locale)
            font: spinner.font
            color: enabled ? "black" : "gray"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            anchors.horizontalCenter: spinner.horizontalCenter
            anchors.verticalCenter: spinner.verticalCenter
            readOnly: !spinner.editable
            validator: spinner.validator
            inputMethodHints: Qt.ImhFormattedNumbersOnly
        }

        up.indicator: Rectangle {
            x: spinner.mirrored ? 0 : parent.width - width
            height: parent.height
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 40 * theme.scaleWidth
            color: spinner.up.pressed ? "#yellow" : "#f6f6f6"
            border.color: enabled ? "darkgray" : "lightgray"

            Text {
                text: "+"
                font.pixelSize: spinner.font.pixelSize * 2
                color: enabled ? "black" : "gray"
                anchors.fill: parent
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        down.indicator: Rectangle {
            x: spinner.mirrored ? parent.width - width : 0
            height: parent.height
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 40 * theme.scaleWidth
            color: spinner.down.pressed ? "#yellow" : "#f6f6f6"
            border.color: enabled ? "darkgray" : "lightgray"

            Text {
                text: "–"
                font.pixelSize: spinner.font.pixelSize * 2
                color: enabled ? "black" : "gray"
                anchors.fill: parent
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        background: Rectangle {
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 150 * theme.scaleWidth
            border.color: enabled ? "black" : "gray"
        }

        onValueModified: {
            if (value == spinner.from) {
                spin_message.visible = true
                spin_message.text = "Must be "+spinner.from /10+" or greater"
            } else if(value == spinner.to){
                spin_message.visible = true
                spin_message.text = "Can't be larger than " + spinner.to /10
            }else {
                spin_message.visible = false
            }

            spinBox_singledigit.value = value / 10

            spinBox_singledigit.valueModified()
        }

        validator: DoubleValidator {}

		textFromValue: function(value, locale) {
			return Number(value / 10).toLocaleString(locale, 'f', spinner.decimals)
		}

		valueFromText: function(text, locale) {
			return Number.fromLocaleString(locale, text) * 10
		}
	}

	Text {
        text: spinBox_singledigit.text
        anchors.bottom: spinner.top
        anchors.left: spinner.left
        font.pixelSize: 15
        color: enabled ? "black" : "gray"
    }
    Text {
        id: spin_message
        visible: false
        text: "message"
        color: "red"
        anchors.top: spinner.bottom
        anchors.left: spinner.left
    }
}
