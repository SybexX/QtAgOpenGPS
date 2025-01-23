// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// On main GL
import QtQuick
import QtQuick.Controls.Fusion
import QtCharts 2.0

import ".."
import "../components"

MoveablePopup {
    id: chartSteer
    height: 300  * theme.scaleHeight
    width: 500  * theme.scaleWidth
    visible: false
    modal: false
    property int yval:0
    x: 400 * theme.scaleWidth

    onVisibleChanged: {
        if (visible) {
            toolsMenu.visible = false
            chartsMenu.visible = false

        }
    }

    Timer {
        id:txt
        interval:50; running: true; repeat: true
        onTriggered: {
                yval++;
            chart.series(0).append(yval, aog.steerAngleSet);
            chart.series(1).append(yval, aog.steerAngleActual);
           if(yval >20)
            {
            chart.axisX().max = yval
            chart.axisX().min = yval-50
            }

                    }
    }
    TopLine{
        id: steerChartTopLine
        titleText: qsTr("Steer Chart")
        onBtnCloseClicked:  chartSteer.close()
    }

    Rectangle{
        id: steerChartWindow
        anchors.top: steerChartTopLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "lightgray"

    ChartView {
            id: chart
            animationOptions: ChartView.NoAnimation
            theme: ChartView.ChartThemeDark
            anchors.fill: parent
            legend.visible: false
            antialiasing: true

            LineSeries {
                id: lineSeries1
                name: "signal 1"
                axisX: axisX
                axisY: axisY
                //useOpenGL: chartView.openGL
            }
            LineSeries {
                id: lineSeries2
                name: "signal 2"
                axisX: axisX
                axisYRight: axisY
                //useOpenGL: chartView.openGL
            }

            ValueAxis {
                id: axisX
                min: 0
                max: 10
            }

            ValueAxis {
                id: axisX2
                min: 0
                max: 10
            }

            ValueAxis {
                id: axisY
                min: -10
                max: 10
            }

            Component.onCompleted: {
            var series1 = chart.createSeries(ChartView.SeriesLineSeries, "signal 1", axisX, axisY);
            var series2 = chart.createSeries(ChartView.SeriesLineSeries, "signal 2",axisX, axisY);
                        }
            }


     }
}
