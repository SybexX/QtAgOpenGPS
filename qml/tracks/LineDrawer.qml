// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Line Drawer - Create AB Lines and Curves from Boundary
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Shapes

import ".."
import "../components" as Comp
import AOG

Popup {
    id: lineDrawer
    width: parent.width
    height: parent.height
    closePolicy: Popup.NoAutoClose

    function show() {
        lineDrawer.visible = true
        TrackInterface.load()
    }

    onVisibleChanged: {
        if (visible) {
            TrackInterface.load()
        } else {
            TrackInterface.close()
        }
    }

    Connections {
        target: trackRenderer

        function onWidthChanged() {
            TrackInterface.viewportWidth = trackRenderer.width
            TrackInterface.updateLines()
        }

        function onHeightChanged() {
            TrackInterface.viewportHeight = trackRenderer.height
            TrackInterface.updateLines()
        }
    }

    Rectangle {
        id: leftSide
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: buttons.left
        anchors.rightMargin: 20
        layer.enabled: true
        layer.samples: 8

        Rectangle {
            id: trackRenderer
            objectName: "trackRenderer"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            width: parent.width > parent.height ? parent.height : parent.width
            height: width
            color: "black"

            Rectangle {
                id: a_rect
                visible: TrackInterface.showa
                width: 24
                height: 24
                radius: 12
                color: "#ffc059"
                x: TrackInterface.apoint.x - 12
                y: TrackInterface.apoint.y - 12
                z: 1
            }

            Rectangle {
                id: b_rect
                visible: TrackInterface.showb
                width: 24
                height: 24
                radius: 12
                color: "#80c0ff"
                x: TrackInterface.bpoint.x - 12
                y: TrackInterface.bpoint.y - 12
                z: 1
            }

            Rectangle {
                id: vehicle_point
                visible: true
                width: 24
                height: 24
                radius: 12
                color: "#f33033"
                x: TrackInterface.vehiclePoint.x - 12
                y: TrackInterface.vehiclePoint.y - 12
            }

            Repeater {
                id: boundaryRepeater
                model: TrackInterface.boundaryLineModel

                Shape {
                    smooth: true
                    anchors.fill: parent

                    ShapePath {
                        id: shapePath
                        strokeColor: model.color
                        strokeWidth: model.width
                        fillColor: "transparent"
                        startX: model.points[0].x
                        startY: model.points[0].y
                        scale: Qt.size(1, 1)
                        joinStyle: ShapePath.RoundJoin

                        PathPolyline {
                            id: ps
                            path: model.points
                        }
                    }
                }
            }

            Shape {
                id: currentTrackShape
                visible: TrackInterface.zoom === 1.0 && TrackInterface.isTrackVisible && TrackInterface.currentTrackLine && TrackInterface.currentTrackLine.length > 1
                anchors.fill: parent
                ShapePath {
                    id: currentTrackShapePath
                    strokeColor: "#00FFFF"
                    strokeWidth: 6
                    fillColor: "transparent"
                    startX: TrackInterface.currentTrackLine && TrackInterface.currentTrackLine.length > 0 ? TrackInterface.currentTrackLine[0].x : 0
                    startY: TrackInterface.currentTrackLine && TrackInterface.currentTrackLine.length > 0 ? TrackInterface.currentTrackLine[0].y : 0
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        id: currentTrackPolyLine
                        path: TrackInterface.currentTrackLine ? TrackInterface.currentTrackLine : []
                    }
                }
            }

            MouseArea {
                id: trackMouseArea
                anchors.fill: parent

                property int fromX: 0
                property int fromY: 0

                onClicked: {
                    if (cboxIsZoom.checked && TrackInterface.zoom === 1) {
                        TrackInterface.sX = ((parent.width / 2 - mouseX) / parent.width) * 1.1
                        TrackInterface.sY = ((parent.height / 2 - mouseY) / -parent.height) * 1.1
                        TrackInterface.zoom = 0.1
                        TrackInterface.updateLines()
                    } else {
                        TrackInterface.mouseClicked(mouseX, mouseY)
                        if (TrackInterface.zoom !== 1.0) {
                            TrackInterface.zoom = 1.0
                            TrackInterface.sX = 0
                            TrackInterface.sY = 0
                            TrackInterface.updateLines()
                        }
                    }
                }

                onPressed: {
                    fromX = mouseX
                    fromY = mouseY
                }

                onPositionChanged: {
                    TrackInterface.mouseDragged(fromX, fromY, mouseX, mouseY)
                    fromX = mouseX
                    fromY = mouseY
                }
            }
        }
    }

    Comp.TopLine {
        id: topLine
        titleText: TrackInterface.sliceCount === 0 ? qsTr("Click 1st point on Boundary") :
                   TrackInterface.sliceCount === 1 ? qsTr("Click 2nd point on Boundary") :
                   TrackInterface.sliceCount >= 2 ? qsTr("Ready - Choose AB Line or Curve") : qsTr("Click 2 points on the Boundary to Begin")
    }

    GridLayout {
        id: buttons
        anchors.bottom: parent.bottom
        anchors.top: topLine.bottom
        anchors.right: parent.right
        anchors.margins: 10
        flow: Grid.LeftToRight
        columns: 2
        rows: 9

        Comp.IconButtonTransparent {
            objectName: "btnBLength"
            icon.source: prefix + "/images/APlusPlusB.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.blength()
        }
        Comp.IconButtonTransparent {
            objectName: "btnALength"
            icon.source: prefix + "/images/APlusPlusA.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.alength()
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/ABTrackCurve.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                TrackInterface.curveLine = true
            }
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/ABTrackAB.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                TrackInterface.curveLine = false
            }
        }

        Comp.IconButtonTransparent {
            id: cboxIsZoom
            objectName: "cboxIsZoom"
            checkable: true
            icon.source: prefix + "/images/ZoomOGL.png"
            Layout.alignment: Qt.AlignCenter
        }

        Comp.IconButtonTransparent {
            objectName: "btnDeletePoints"
            icon.source: prefix + "/images/HeadlandReset.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.cancelTouch()
        }

        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/ABLineCycleBk.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.cycleBackward()
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/ABLineCycle.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.cycleForward()
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/TrackVisible.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.setTrackVisible(!TrackInterface.isTrackVisible)
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/Trash.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: TrackInterface.deleteTrack()
        }
        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/Time.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                var time = Qt.formatDateTime(new Date(), "dd.MM.yyyy HH:mm:ss")
                newLine.text += " " + time
            }
        }

        Text {
            text: TrackInterface.trackCount > 0 ? "%1/%2".arg(TrackInterface.currentTrackIndex + 1).arg(TrackInterface.trackCount) : "0/0"
            Layout.alignment: Qt.AlignCenter
        }

        Rectangle {
            id: curveNameInput
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.margins: 4
            color: "ghostwhite"
            border.color: "darkgray"
            border.width: 1
            height: 40
            TextInput {
                id: newLine
                objectName: "drawerCurveName"
                anchors.fill: parent
                anchors.margins: 4
                text: TrackInterface.trackName
                onTextChanged: TrackInterface.trackName = text
            }
        }

        Comp.IconButtonTransparent {
            icon.source: prefix + "/images/Cancel64.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                TrackInterface.cancelTrackCreation()
                lineDrawer.visible = false
            }
        }

        Comp.IconButtonTransparent {
            objectName: "btnDrawerSave"
            icon.source: prefix + "/images/OK64.png"
            onClicked: {
                TrackInterface.trackName = newLine.text
                TrackInterface.curveLine?TrackInterface.createCurve():TrackInterface.createABLine()
                TrackInterface.saveExit()
            }
            Layout.alignment: Qt.AlignCenter
        }
    }
}
