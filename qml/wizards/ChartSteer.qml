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

    // Свойства для данных (аналогично оригиналу)
    property int yval: 0
    property int xval1: 0
    property int xval2: 0
    property int axismin: 0
    property int axismax: 0
    property string chartName
    property string lineName1
    property string lineName2

    // Диапазоны осей (для Canvas)
    property real axisXmin: 0
    property real axisXmax: 50

    // Хранилище истории точек
    property var timeData: []      // значения по оси X (yval)
    property var line1Data: []     // значения первой линии (xval1)
    property var line2Data: []     // значения второй линии (xval2)

    // Таймер добавления точек (50 мс)
    Timer {
        id: txt
        interval: 50
        running: chartSteer.visible
        repeat: true
        onTriggered: {
            yval++;
            timeData.push(yval);
            line1Data.push(xval1);
            line2Data.push(xval2);

            // Автоматический сдвиг оси X при заполнении экрана
            if (yval > 20) {
                axisXmin = yval - 50;
                axisXmax = yval;
            }

            canvas.requestPaint(); // перерисовать
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
        anchors.bottom: legendRow.top
        color: "black"

        // Кнопки управления (справа)
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
            Text{
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("+")
                color: "lightgray"
            }
            onClicked: {
                // Сузить диапазон (приблизить)
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
            Text{
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("A")
                color: "lightgray"
            }
            onClicked: {
                // Автоподбор оси Y по текущим значениям
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
            Text{
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: qsTr("-")
                color: "lightgray"
            }
            onClicked: {
                // Расширить диапазон (отдалить)
                axismin -= 2;
                axismax += 2;
                canvas.requestPaint();
            }
        }

        // Кастомный график на Canvas
        Canvas {
            id: canvas
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.left: btnChartminus.right
            anchors.margins: 5
            antialiasing: true

            // Привязка к диапазонам осей из родителя
            property real axisYmin: axismin
            property real axisYmax: axismax
            property real axisXmin: chartSteer.axisXmin
            property real axisXmax: chartSteer.axisXmax

            // Перерисовка при изменении любого диапазона
            onAxisYminChanged: requestPaint()
            onAxisYmaxChanged: requestPaint()
            onAxisXminChanged: requestPaint()
            onAxisXmaxChanged: requestPaint()

            // Отступы для осей и подписей
            readonly property real leftMargin: 50
            readonly property real rightMargin: 30
            readonly property real topMargin: 20
            readonly property real bottomMargin: 30

            // Преобразование координат данных в пиксели
            function mapX(x) {
                return leftMargin + (x - axisXmin) / (axisXmax - axisXmin) * (width - leftMargin - rightMargin);
            }

            function mapY(y) {
                return topMargin + (axisYmax - y) / (axisYmax - axisYmin) * (height - topMargin - bottomMargin);
            }

            onPaint: {
                var ctx = canvas.getContext("2d");
                ctx.clearRect(0, 0, width, height);

                // Фон
                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, width, height);

                drawAxes(ctx);
                drawGrid(ctx);
                drawLines(ctx);
            }

            function drawAxes(ctx) {
                ctx.save();
                ctx.strokeStyle = "white";
                ctx.lineWidth = 1;

                // Ось X (через y=0)
                var yZero = mapY(0);
                ctx.beginPath();
                ctx.moveTo(leftMargin, yZero);
                ctx.lineTo(width - rightMargin, yZero);
                ctx.stroke();

                ctx.restore();
            }

            function drawGrid(ctx) {
                ctx.save();
                ctx.strokeStyle = "#88AAEE";   // голубая сетка
                ctx.lineWidth = 0.5;

                // Вертикальные линии (по X)
                var stepX = (axisXmax - axisXmin) / 5;
                if (stepX <= 0) stepX = 1;
                for (var i = 0; i <= 5; i++) {
                    var xVal = axisXmin + i * stepX;
                    var xPos = mapX(xVal);
                    ctx.beginPath();
                    ctx.moveTo(xPos, topMargin);
                    ctx.lineTo(xPos, height - bottomMargin);
                    ctx.stroke();
                }

                // Горизонтальные линии (по Y)
                var stepY = (axisYmax - axisYmin) / 5;
                if (stepY <= 0) stepY = 1;
                for (i = 0; i <= 5; i++) {
                    var yVal = axisYmin + i * stepY;
                    var yPos = mapY(yVal);
                    ctx.beginPath();
                    ctx.moveTo(leftMargin, yPos);
                    ctx.lineTo(width - rightMargin, yPos);
                    ctx.stroke();
                }

                ctx.restore();
            }
            function drawLines(ctx) {
                if (timeData.length < 2) return;

                // Первая линия (зелёная)
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

                // Вторая линия (красная)
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
        // после Canvas, но внутри steerChartWindow
        Text {
            id: maxYLabel
            anchors.right: canvas.right
            anchors.rightMargin: 5
            anchors.top: canvas.top
            anchors.topMargin: 5
            text: axismax.toFixed(1)
            font.pixelSize: 18 * theme.scaleHeight
            color: "green"
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
            color: "green"
            z: 3
        }
    }
    Rectangle {
        id: legendRow
        Layout.preferredHeight: 40 * theme.scaleHeight
        color: "transparent"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Row {
            spacing: 10 * theme.scaleWidth
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.centerIn: parent
            Text {
                anchors.bottom: parent.bottom
                text: qsTr(lineName1)
                font.pixelSize: 18 * theme.scaleHeight
                color: "green"
            }
            Text {
                anchors.bottom: parent.bottom
                text: qsTr(lineName2)
                font.pixelSize: 18 * theme.scaleHeight
                color: "red"
            }
        }
    }
}
