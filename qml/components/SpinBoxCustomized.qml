// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Spinbox, just customized. Allows the use of decimals
import QtQuick
import QtQuick.Controls.Fusion

Item {
    id: spinBox_Customized
    property double from
    property double value
    property double to
    property string text: ""
	property int stepSize: 1
    property bool editable
    property double boundValue
    property int fontPixelSize: 15
	property int decimals: 0
	property int decimalFactor: 1

	onDecimalsChanged: {
		if (decimals > 0)
			decimalFactor = Math.pow(10, decimals)
		else
			decimalFactor = 1
	}
    signal valueModified()

    width: spinner.width
    height: spinner.height + (spin_message.length > 0 ? spin_message.height : 0) + (spin_text.length > 0 ? spin_text.height : 0)

    //this method sets the spinner value without firing the
    //valueChanged signal

    function setValue(value) {
        spinner.value = value
    }

    onBoundValueChanged: {
        value = boundValue
        spinner.value = boundValue
    }

    SpinBox{
		id: spinner
		from: spinBox_Customized.from * decimalFactor
		to: spinBox_Customized.to * decimalFactor
		editable: spinBox_Customized.editable
		value: spinBox_Customized.value * decimalFactor
		stepSize: spinBox_Customized.stepSize
		property int decimals: spinBox_Customized.decimals
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
        height: 40 * theme.scaleHeight
        width: 150 * theme.scaleWidth

		Keys.onReturnPressed: {
			//console.debug("enter was pressed.  ignore it.")
		}

        contentItem: TextInput {
            id: text_input
            text: spinner.textFromValue(spinner.value, spinner.locale)
            font: spinner.font
            color: "black"
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
                color: "black"
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
                color: "black"
                anchors.fill: parent
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        background: Rectangle {
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 150 * theme.scaleWidth
            border.color: "darkgray"
        }
		onValueModified: {
			//this only fires when the user interactively changes the spinbox.

			if (value / decimalFactor == spinBox_Customized.from) {
				spin_message.visible = true
				spin_message.text = "Min:"+from / decimalFactor
			} else if(value / decimalFactor == spinBox_Customized.to){
				spin_message.visible = true
				spin_message.text = "Max: " + to / decimalFactor
			}else {
				spin_message.visible = false
			}
			spinBox_Customized.value = spinner.value/ decimalFactor
			text_input.text = spinBox_Customized.value
			spinBox_Customized.valueModified()

		}
		textFromValue: function(value, locale) {
			return Number(value / decimalFactor).toLocaleString(locale, 'f', spinner.decimals)
		}

		valueFromText: function(text, locale) {
			return Number.fromLocaleString(locale, text) * decimalFactor
		}
	}

	Text {
		visible: false
		onTextChanged: {
			visible = true
		}
		id: spin_text
		text: spinBox_Customized.text
		anchors.bottom: spinner.top
		anchors.left: spinner.left
		font.pixelSize: spinBox_Customized.fontPixelSize
		onVisibleChanged: {
			if (visible)
			height = text.height 
			else
			height = 0
		}
	}

	Text {
		id: spin_message
		visible: false
		text: ""
		color: "red"
		anchors.top: spinner.bottom
		anchors.left: spinner.left
		onVisibleChanged: {
			if (visible)
			height = text.height 
			else
			height = 0
		}
	}
}
