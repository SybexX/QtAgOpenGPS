// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// ABLine drawer off boundary
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Shapes

import ".."
import "../components" as Comp
import AOG 1.0

Popup{
	id: lineDrawer
    width: parent.width
    height: parent.height
    closePolicy: Popup.NoAutoClose

    function show() {
        lineDrawer.visible = true
    }
    function worldToPixelX(worldX) {
        return (worldX - HeadlandInterface.sX) * HeadlandInterface.zoom + lineDrawerField.width / 2;
    }
    function worldToPixelY(worldY) {
        return (worldY - HeadlandInterface.sY) * HeadlandInterface.zoom + lineDrawerField.height / 2;
    }

    function loadCurrentTrackName() {
        newLine.text = (TracksInterface.idx >= 0) ? TracksInterface.getTrackName(TracksInterface.idx) : "";
    }

    onVisibleChanged: {
        if (visible) {
            HeadlandInterface.load()
            TracksInterface.cancelCreating()
            loadCurrentTrackName()
        } else {
            HeadlandInterface.close()
        }
    }

    Connections {
        target: lineDrawerField
        function onWidthChanged() {
            HeadlandInterface.viewportWidth = lineDrawerField.width
            HeadlandInterface.updateLines()
        }
        function onHeightChanged() {
            HeadlandInterface.viewportHeight = lineDrawerField.height
            HeadlandInterface.updateLines()
        }
    }

    Connections {
        target: TracksInterface
        function onPointAChanged() { console.log(qmlLog, "pointA changed:", TracksInterface.pointA); }
        function onPointBChanged() { console.log(qmlLog, "pointB changed:", TracksInterface.pointB); }
        function onCreatingChanged() { console.log(qmlLog, "creatingChanged:", TracksInterface.isCreating); }
        function onIdxChanged() { console.log(qmlLog, "idx changed:", TracksInterface.idx)
        loadCurrentTrackName()}
    }

    Comp.TopLine {
        id: topLine
        titleText: qsTr("Click 2 points on the Boundary to Begin")
    }

    Rectangle {
        anchors.top: topLine.bottom
        anchors.left: parent.left
        anchors.right: buttons.left
        anchors.bottom: parent.bottom
        color: "black"

        Rectangle {
            id: lineDrawerField
            objectName: "lineDrawerField"
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
            color: "black"

            Rectangle {
                id: a_rect
                visible: TracksInterface.isCreating && (TracksInterface.pointA.x !== 0 || TracksInterface.pointA.y !== 0)
                width: 24; height: 24; radius: 12
                color: "#ffc059"
                x: TracksInterface.pointAPixel.x - 12
                y: TracksInterface.pointAPixel.y - 12
                z: 2
                Text { anchors.centerIn: parent; text: "A"; color: "white"; font.bold: true }
            }

            Rectangle {
                id: b_rect
                visible: TracksInterface.isCreating && (TracksInterface.pointB.x !== 0 || TracksInterface.pointB.y !== 0) &&
                         (TracksInterface.pointA.x !== TracksInterface.pointB.x || TracksInterface.pointA.y !== TracksInterface.pointB.y)
                width: 24; height: 24; radius: 12
                color: "#80c0ff"
                x: TracksInterface.pointBPixel.x - 12
                y: TracksInterface.pointBPixel.y - 12
                z: 2
                Text { anchors.centerIn: parent; text: "B"; color: "white"; font.bold: true }
            }

            Rectangle {
                visible: true
                width: 24; height: 24; radius: 12
                color: "#f33033"
                x: HeadlandInterface.vehiclePoint.x - 12
                y: HeadlandInterface.vehiclePoint.y - 12
                z: 1
            }

            Repeater {
                model: HeadlandInterface.boundaryLineModel
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeColor: model.color
                        strokeWidth: model.width
                        fillColor: "transparent"
                        startX: model.points[0].x
                        startY: model.points[0].y
                        joinStyle: ShapePath.RoundJoin
                        PathPolyline { path: model.points }
                    }
                }
            }

            Repeater {
                model: TracksInterface.model
                delegate: Shape {
                    anchors.fill: parent
                    visible: index === TracksInterface.idx

                    property var pixelPoints: {
                        if (model.mode === 2) {
                            var endA = model.endPtA;
                            var endB = model.endPtB;
                            if (endA && endB) {
                                return [
                                    Qt.point(worldToPixelX(endA.x), worldToPixelY(endA.y)),
                                    Qt.point(worldToPixelX(endB.x), worldToPixelY(endB.y))
                                ];
                            }
                        } else if (model.mode === 4) {
                            var pts = model.curvePts;
                            if (pts && pts.length > 0) {
                                return pts.map(pt => Qt.point(worldToPixelX(pt.x), worldToPixelY(pt.y)));
                            }
                        }
                        return [];
                    }

                    ShapePath {
                        strokeColor: model.mode === 2 ? "#00FFFF" : "#FF69B4"
                        strokeWidth: 4
                        fillColor: "transparent"
                        startX: pixelPoints.length > 0 ? pixelPoints[0].x : 0
                        startY: pixelPoints.length > 0 ? pixelPoints[0].y : 0
                        PathPolyline { path: pixelPoints }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                property int fromX: 0
                property int fromY: 0

                onClicked: {
                    var worldX = (mouseX - lineDrawerField.width/2);
                    var worldY = (mouseY - lineDrawerField.height/2);
                    TracksInterface.addPoint(Qt.point(worldX, worldY));
                    console.log(qmlLog, "Mouse clicked, world:", worldX.toFixed(2), worldY.toFixed(2), "isCreating:", TracksInterface.isCreating);
                }
            }
        }
    }

    // Правая панель с кнопками
    Rectangle {
        id: buttons
        color: "lightgray"
        width: 250
        anchors.bottom: parent.bottom
        anchors.top: topLine.bottom
        anchors.right: parent.right

        GridLayout {
            id: top6Buttons
            anchors.fill: parent
            anchors.margins: 5
            columns: 2
            rows: 9
            flow: Grid.LeftToRight

            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/APlusPlusB.png"
                Layout.alignment: Qt.AlignCenter
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/MappingOff.png"
                Layout.alignment: Qt.AlignCenter
                visible: false
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/APlusPlusA.png"
                Layout.alignment: Qt.AlignCenter
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/HeadlandDeletePoints.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: TracksInterface.cancelCreating()
            }
            Comp.IconButtonTransparent {
                id: trackVisibleBtn
                icon.source: prefix + "/images/TrackVisible.png"
                Layout.alignment: Qt.AlignCenter
                checkable: true
                checked: (TracksInterface.idx >= 0) ? TracksInterface.getTrackVisible(TracksInterface.idx) : false
                onClicked: {
                    if (TracksInterface.idx >= 0)
                        TracksInterface.setVisible(TracksInterface.idx, checked)
                }
                Connections {
                    target: TracksInterface
                    function onIdxChanged() {
                        trackVisibleBtn.checked = (TracksInterface.idx >= 0) ? TracksInterface.getTrackVisible(TracksInterface.idx) : false
                    }
                }
            }
            Comp.IconButtonTransparent {
                id: boundaryCurve
                icon.source: prefix + "/images/BoundaryCurveLine.png"
                Layout.alignment: Qt.AlignCenter
                text: qsTr("Boundary Curve")
                visible: false
            }
            Comp.IconButtonTransparent {
                id: cboxIsZoom
                icon.source: prefix + "/images/ZoomOGL.png"
                Layout.alignment: Qt.AlignCenter
                checkable: true
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/Trash.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: {
                    if (TracksInterface.idx >= 0)
                        TracksInterface.delete_track(TracksInterface.idx)
                }
            }
            Comp.IconButtonTransparent {
                id: trackCurve
                icon.source: prefix + "/images/ABTrackCurve.png"
                Layout.alignment: Qt.AlignCenter
                checkable: true
                onClicked: {
                    trackAB.checked = false
                    TracksInterface.cancelCreating()
                    TracksInterface.startCreating(4)
                    newLine.text = "" // очищаем поле имени для новой линии
                    console.log(qmlLog, "ABTrackCurve clicked");
                }
            }
            // Кнопка создания AB линии
            Comp.IconButtonTransparent {
                id: trackAB
                icon.source: prefix + "/images/ABTrackAB.png"
                Layout.alignment: Qt.AlignCenter
                checkable: true
                onClicked: {
                    trackCurve.checked = false
                    TracksInterface.cancelCreating()
                    TracksInterface.startCreating(2)
                    newLine.text = ""
                    console.log(qmlLog, "ABTrackAB clicked");
                }
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/ABLineCycleBk.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: TracksInterface.prev()
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/ABLineCycle.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: TracksInterface.next()
            }
            Rectangle {
                id: curveNameInput
                width: parent.width - 10
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignCenter
                color: "ghostwhite"
                border.color: "darkgray"
                border.width: 1
                height: 40
                TextInput {
                    id: newLine
                    Layout.alignment: Qt.AlignCenter
                    objectName: "drawerCurveName"
                    anchors.fill: parent
                    anchors.margins: 5
                    font.pixelSize: 16
                }
            }
            Text {
                text: "1/16"
                Layout.alignment: Qt.AlignCenter
                visible: false
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/Time.png"
                Layout.alignment: Qt.AlignCenter
                visible: false
            }
            Comp.IconButtonTransparent {
                icon.source: prefix + "/images/Cancel64.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: lineDrawer.visible = false
            }
            Comp.IconButtonTransparent {
                objectName: "btnDrawerSave"
                icon.source: prefix + "/images/OK64.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: {
                    if (TracksInterface.isCreating) {
                        var name = newLine.text.trim()
                        if (name === "") {
                            name = (TracksInterface.curveLine ? "Curve" : "AB line")+Qt.formatTime(new Date(), "HH:mm:ss");
                        }
                        TracksInterface.finishCreating(name)
                    }
                    lineDrawer.visible = false
                }
            }
        }
    }
}
