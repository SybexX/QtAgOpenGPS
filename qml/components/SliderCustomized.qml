// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
//
import QtQuick.Controls.Fusion
import QtQuick

Slider{
    value: 0
    to: 0
    from: 0
    id: sliderCustomized
    property double multiplicationValue: 1
    property alias leftText: leftText.text
    property alias rightText: rightText.text
    property alias centerTopText: topText.text
    property alias leftTopText: leftTopText.text
    property alias rightTopText: rightTopText.text
    property alias colorLeftTopText: leftTopText.color
    property alias colorRightTopText: rightTopText.color
    stepSize: 1
    topInset: topText.textHeight
    topPadding: topInset
    leftInset: leftText.textWidth
    leftPadding: leftInset
    rightInset: rightText.textWidth
    rightPadding: rightInset
    implicitHeight: 50 * theme.scaleHeight
    implicitWidth: 250 * theme.scaleWidth

    /*As was noted on the Qt forum, the text is still part of
      the clickable area, the idea is catcher will catch those
      and keep them from actually doing anything.
      */
    MouseArea{
        id: catcher
        anchors.top: parent.top
        anchors.bottom: backgroundRect.top
        anchors.right: parent.right
        anchors.left: parent.left
        onClicked: ("")

    }

    background: Rectangle {
        id: backgroundRect
        x: parent.leftPadding
        y: parent.topPadding + parent.availableHeight / 2 - height / 2
        radius: 2
        color: "white"


    }

    handle: Rectangle{
        id: handleRect
        height: backgroundRect.height - 4
        radius: 2
        width: 40 * theme.scaleWidth
        visible: true
        color: "lightgray"
        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
        y: parent.topPadding + parent.availableHeight / 2 - height / 2
        anchors.verticalCenter: backgroundRect.verticalCenter
    }
    Button{
        id: rightSliderButton
        anchors.right: backgroundRect.right
        anchors.left: handleRect.right
        anchors.top: backgroundRect.top
        anchors.bottom: backgroundRect.bottom
        //width: 20
        onClicked: sliderCustomized.value = sliderCustomized.value + sliderCustomized.stepSize * multiplicationValue
        background: Rectangle {
            height: parent.height
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 40 * theme.scaleWidth
            border.color: enabled ? "darkgray" : "lightgray"

            Text {
                text: "+"
                font.pixelSize: sliderCustomized.font.pixelSize * 1.5
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 5 * theme.scaleWidth
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

    }
    Button{
        id: leftSliderButton
        anchors.left: backgroundRect.left
        anchors.right: handleRect.left
        anchors.top: backgroundRect.top
        onClicked: sliderCustomized.value = sliderCustomized.value - sliderCustomized.stepSize * multiplicationValue
        anchors.bottom: backgroundRect.bottom
        //width: 20
        background: Rectangle {
            height: parent.height
            implicitHeight: 40 * theme.scaleHeight
            implicitWidth: 40 * theme.scaleWidth
            border.color: enabled ? "darkgray" : "lightgray"

            Text {
                text: "–"
                font.pixelSize: sliderCustomized.font.pixelSize * 1.5
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 5 * theme.scaleWidth
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    TextLine{
        id: leftText
        text: ""
        property int textWidth: (text.length > 0) ? 30 : 0
        color: sliderCustomized.enabled ? "black" : "grey"
        anchors.left: parent.left
        anchors.verticalCenter: backgroundRect.verticalCenter
    }
    TextLine{
        id: rightText
        text: ""
        property int textWidth: (text.length > 0) ? 30 : 0
        color: sliderCustomized.enabled ? "black" : "grey"
        anchors.right: parent.right
        anchors.verticalCenter: backgroundRect.verticalCenter
        anchors.horizontalCenter: undefined
    }
    TextLine{
        id: leftTopText
        text: ""
        anchors.top: parent.top
        anchors.horizontalCenter: undefined
        color: "black"
        anchors.left: backgroundRect.left
    }
    TextLine{
        id: rightTopText
        text: ""
        anchors.right: backgroundRect.right
        anchors.horizontalCenter: undefined
        anchors.top: parent.top
        color: "black"
    }
    TextLine{
        id: topText
        property int textHeight: (text.length > 0) ? 18 : 0
        text: ""
        color: sliderCustomized.enabled ? "black" : "grey"
        anchors.top: parent.top
        anchors.horizontalCenter: backgroundRect.horizontalCenter
    }
}
