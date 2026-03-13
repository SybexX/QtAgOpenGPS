// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// On main GL
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

import ".."
import "../components"

MoveablePopup {
    id: chartSteer
    height: 300 * theme.scaleHeight
    width: 500 * theme.scaleWidth
    visible: false
    modal: false

    property real yval: 0
    property real xval1: 0
    property real xval2: 0
    property int axismin: -1
    property int axismax: 1
    property string chartName
    property string lineName1
    property string lineName2

    property real axisXmin: 0
    property real axisXmax: 50

    property var timeData: []
    property var line1Data: []
    property var line2Data: []
    property int maxPoints: 100

    Timer {
        id: txt
        interval: 100
        running: chartSteer.visible
        repeat: true
        onTriggered: {
            yval++;
            timeData.push(yval);
            line1Data.push(xval1);
            line2Data.push(xval2);

            if (timeData.length > maxPoints) {
                timeData.shift();
                line1Data.shift();
                line2Data.shift();
            }

            if (yval > 20) {
                axisXmin = yval - 50;
                axisXmax = yval;
            }

            canvas.requestPaint();
        }
    }

    TopLine {
        id: steerChartTopLine
        titleText: qsTr(chartName)
        onBtnCloseClicked: chartSteer.close()
    }

    Rectangle {
        id: steerChartWindow
        anchors.top: steerChartTopLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "black"

        IconButtonTextBeside {
            id: btnChartplus
            color: "transparent"
            borderColor: "lightgrey"
            anchors.left: parent.left
            anchors.top: parent.top
            width: 40 * theme.scaleWidth
            height: 50 * theme.scaleHeight
            text: ""
            z: 2
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("+")
                color: "lightgray"
            }
            onClicked: {
                if (axismin > -1) axismin = -1;
                else axismin += 2;
                if (axismax < 1) axismax = 1;
                else axismax -= 2;
                canvas.requestPaint();
            }
        }

        IconButtonTextBeside {
            id: btnChartauto
            color: "transparent"
            borderColor: "lightgrey"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 40 * theme.scaleWidth
            height: 50 * theme.scaleHeight
            text: ""
            z: 2
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("A")
                color: "lightgray"
            }
            onClicked: {
                var maxVal = Math.max(xval1, xval2);
                var minVal = Math.min(xval1, xval2);
                if (maxVal > axismax) axismax = maxVal + 2;
                if (minVal < axismin) axismin = minVal - 2;
                canvas.requestPaint();
            }
        }

        IconButtonTextBeside {
            id: btnChartminus
            color: "transparent"
            borderColor: "lightgrey"
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 40 * theme.scaleWidth
            height: 50 * theme.scaleHeight
            text: ""
            z: 2
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("-")
                color: "lightgray"
            }
            onClicked: {
                axismin -= 2;
                axismax += 2;
                canvas.requestPaint();
            }
        }

        Canvas {
            id: canvas
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.left: btnChartminus.right
            anchors.margins: 5
            antialiasing: true

            property real axisYmin: axismin
            property real axisYmax: axismax
            property real axisXmin: chartSteer.axisXmin
            property real axisXmax: chartSteer.axisXmax

            onAxisYminChanged: requestPaint()
            onAxisYmaxChanged: requestPaint()
            onAxisXminChanged: requestPaint()
            onAxisXmaxChanged: requestPaint()

            readonly property real leftMargin: 50
            readonly property real rightMargin: 30
            readonly property real topMargin: 20
            readonly property real bottomMargin: 30

            function mapX(x) {
                var range = axisXmax - axisXmin;
                if (range === 0) return leftMargin;
                return leftMargin + (x - axisXmin) / range * (width - leftMargin - rightMargin);
            }

            function mapY(y) {
                var range = axisYmax - axisYmin;
                if (range === 0) return topMargin;
                return topMargin + (axisYmax - y) / range * (height - topMargin - bottomMargin);
            }

            onPaint: {
                var ctx = canvas.getContext("2d");
                ctx.clearRect(0, 0, width, height);

                // Фон
                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, width, height);

                drawGrid(ctx);
                drawAxes(ctx);
                drawLines(ctx);
            }

            function drawAxes(ctx) {
                ctx.save();
                ctx.strokeStyle = "white";
                ctx.lineWidth = 1;

                var yZero = mapY(0);
                ctx.beginPath();
                ctx.moveTo(leftMargin, yZero);
                ctx.lineTo(width - rightMargin, yZero);
                ctx.stroke();

                ctx.restore();
            }

            function drawGrid(ctx) {
                ctx.save();
                ctx.strokeStyle = "#88AAEE";
                ctx.lineWidth = 0.5;

                var stepX = (axisXmax - axisXmin) / 5;
                if (stepX <= 0) stepX = 1;
                for (var i = 0; i <= 5; i++) {
                    var xVal = axisXmin + i * stepX;
                    var xPos = mapX(xVal);
                    ctx.beginPath();
                    ctx.moveTo(xPos, 0);
                    ctx.lineTo(xPos, height);
                    ctx.stroke();
                }

                var stepY = (axisYmax - axisYmin) / 5;
                if (stepY <= 0) stepY = 1;
                for (i = 0; i <= 5; i++) {
                    var yVal = axisYmin + i * stepY;
                    var yPos = mapY(yVal);
                    ctx.beginPath();
                    ctx.moveTo(0, yPos);
                    ctx.lineTo(width, yPos);
                    ctx.stroke();
                }

                ctx.restore();
            }

            function drawLines(ctx) {
                if (timeData.length < 2) return;

                ctx.save();
                ctx.strokeStyle = "lime";
                ctx.lineWidth = 2;
                ctx.beginPath();
                for (var i = 0; i < timeData.length; i++) {
                    var x = mapX(timeData[i]);
                    var y = mapY(line1Data[i]);
                    if (i === 0) ctx.moveTo(x, y);
                    else ctx.lineTo(x, y);
                }
                ctx.stroke();

                ctx.strokeStyle = "red";
                ctx.beginPath();
                for (i = 0; i < timeData.length; i++) {
                    x = mapX(timeData[i]);
                    y = mapY(line2Data[i]);
                    if (i === 0) ctx.moveTo(x, y);
                    else ctx.lineTo(x, y);
                }
                ctx.stroke();
                ctx.restore();
            }
        }

        Text {
            id: maxYLabel
            anchors.right: canvas.right
            anchors.rightMargin: 5
            anchors.top: canvas.top
            anchors.topMargin: 5
            text: axismax.toFixed(1)
            font.pixelSize: 18 * theme.scaleHeight
            color: "lime"
            z: 3
        }
        Text {
            id: minYLabel
            anchors.right: canvas.right
            anchors.rightMargin: 5
            anchors.bottom: canvas.bottom
            anchors.bottomMargin: 5
            text: axismin.toFixed(1)
            font.pixelSize: 18 * theme.scaleHeight
            color: "lime"
            z: 3
        }

    }

    Rectangle {
        id: legendRow
        height: 40 * theme.scaleHeight
        color: "transparent"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Row {
            anchors.centerIn: parent
            spacing: 10 * theme.scaleWidth

            Text {
                text: qsTr(lineName1)
                font.pixelSize: 18 * theme.scaleHeight
                color: "lime"
            }
            Text {
                text: qsTr(lineName2)
                font.pixelSize: 18 * theme.scaleHeight
                color: "red"
            }
        }
    }
}
