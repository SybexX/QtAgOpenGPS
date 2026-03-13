// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Shapes
//import Settings
import AOG
import "components" as Comp


Popup{
    id: headacheDesigner
    width: parent.width
    height: parent.height
    //color: "ghostwhite"
    closePolicy: Popup.NoAutoClose

    function show(){
        headacheDesigner.visible = true
        headacheCurve.isChecked = true
    }

    onVisibleChanged: {
        if(visible) {
            HeadacheInterface.load()
        } else {
            HeadacheInterface.close()
        }
    }

    /*
    property var boundaryLineModel: [
        {
            index: 0,
            color: "#FF0000",
            width: 4,
            points: [
                Qt.point(50, 50),
                Qt.point(100, 50),
                Qt.point(100, 100),
                Qt.point(50, 100),
                Qt.point(50, 50)
            ]
        },
        {
            index: 1,
            color: "#00FF00",
            width: 4,
            points: [
                Qt.point(25, 25),
                Qt.point(75, 25),
                Qt.point(75, 75),
                Qt.point(25, 75),
                Qt.point(25, 25)
            ]
        }
    ]

    property var headacheLineModel: [
        {
            index: 0,
            color: "#FF0000",
            width: 4,
            points: [
                Qt.point(150, 150),
                Qt.point(200, 150),
                Qt.point(200, 200),
                Qt.point(150, 200),
                Qt.point(150, 150)
            ],
            dashed: false
        },
        {
            index: 1,
            color: "#00FF00",
            width: 4,
            points: [
                Qt.point(125, 125),
                Qt.point(175, 125),
                Qt.point(175, 175),
                Qt.point(125, 175),
                Qt.point(125, 125)
            ],
            dashed: true
        }
    ]
    */

    Connections {
        //Let backend interface know about the viewport size
        target: headacheRenderer

        function onWidthChanged() {
            HeadacheInterface.viewportWidth = headacheRenderer.width;
            HeadacheInterface.updateLines()
        }

        function onHeightChanged() {
            HeadacheInterface.viewportHeight = headacheRenderer.height;
            HeadacheInterface.updateLines()
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

        Rectangle{//renderer goes here
            id: headacheRenderer
            objectName: "headacheRenderer"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            width: parent.width > parent.height ? parent.height : parent.width
            height: width  //1:1 aspect ratio

            color: "black"

            Rectangle {
                id: a_rect
                visible: HeadacheInterface.showa
                width: 24
                height: 24
                radius: 12
                color: "#ffc059"
                x: HeadacheInterface.apoint.x - 12
                y: HeadacheInterface.apoint.y - 12
                z: 1
            }

            Rectangle {
                id: b_rect
                visible: HeadacheInterface.showb
                width: 24
                height: 24
                radius: 12
                color:  "#80c0ff"
                x: HeadacheInterface.bpoint.x - 12
                y: HeadacheInterface.bpoint.y - 12
                z: 1
            }

            Repeater {
                id: boundaryRepeater

                model: HeadacheInterface.boundaryLineModel

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
                        scale: Qt.size(1,1)
                        joinStyle: ShapePath.RoundJoin

                        PathPolyline {
                            id: ps
                            path: model.points
                        }
                    }
                }
            }
            Repeater {
                id: headlinesRepeater

                model: HeadacheInterface.headacheLineModel

                Shape {
                    smooth: true

                    anchors.fill: parent

                    ShapePath {
                        id: headlinesShapePath
                        strokeColor: model.color
                        strokeWidth: model.width
                        strokeStyle: model.dashed ? ShapePath.DashLine : ShapePath.SolidLine;
                        fillColor: "transparent"
                        startX: model.points[0].x
                        startY: model.points[0].y
                        scale: Qt.size(1,1)
                        joinStyle: ShapePath.RoundJoin

                        PathPolyline {
                            id: headachePs
                            path: model.points
                        }
                    }
                }
            }

            Shape {
                id: headlandShape
                visible: HeadacheInterface.headlandLine.length > 0
                anchors.fill: parent
                ShapePath {
                    id: headlandShapePath
                    strokeColor: "#f1e817"
                    strokeWidth: 8
                    fillColor: "transparent"
                    startX: HeadacheInterface.headlandLine.length > 0 ? HeadacheInterface.headlandLine[0].x : 0
                    startY: HeadacheInterface.headlandLine.length > 0 ? HeadacheInterface.headlandLine[0].y : 0
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        id: headlandShapePolyLine
                        path: HeadacheInterface.headlandLine
                    }
                }
            }

            MouseArea {
                id: headacheMouseArea
                anchors.fill: parent

                property int fromX: 0
                property int fromY: 0

                onClicked: {
                    if (cboxIsZoom.checked && HeadacheInterface.zoom === 1) {
                        HeadacheInterface.sX = ((parent.width / 2 - mouseX) / parent.width) * 1.1
                        HeadacheInterface.sY = ((parent.width / 2 - mouseY) / -parent.width) * 1.1
                        HeadacheInterface.zoom = 0.1
                        HeadacheInterface.updateLines()
                    } else {
                        HeadacheInterface.mouseClicked(mouseX, mouseY)
                        if (HeadacheInterface.zoom != 1.0) {
                            HeadacheInterface.zoom = 1.0
                            HeadacheInterface.sX = 0
                            HeadacheInterface.sY = 0
                            HeadacheInterface.updateLines()
                        }
                    }
                }

                onPressed: {
                    //save a copy of the coordinates
                    fromX = mouseX
                    fromY = mouseY
                }

                onPositionChanged: {
                    HeadacheInterface.mouseDragged(fromX, fromY, mouseX, mouseY)
                    fromX = mouseX
                    fromY = mouseY
                }

                //onWheel: {}
            }
        }
    }

    ColumnLayout{
        id: buttons
        //anchors.left: headacheRenderer.right
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 30
        ButtonGroup{
            buttons: [ headacheAB, headacheCurve ]
        }
        GridLayout{
            id: top6btns
            flow: Grid.LeftToRight // NOTE!! this is not how I normally do it
            //but it seems to make the most sense here
            columns: 3
            rows: 2
            width: parent.width
            rowSpacing: buttons.spacing
            Comp.IconButtonTransparent{
                //objectName: "btnBLength"
                icon.source: prefix + "/images/APlusPlusB.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.blength()
            }
            Comp.IconButtonTransparent{
                //objectName: "btnBShrink"
                icon.source: prefix + "/images/APlusMinusB.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.bshrink()
            }
            Comp.IconButtonTransparent{
                //objectName: "cBoxIsSectionControlled"
                icon.source: prefix + "/images/HeadlandSectionOn.png"
                iconChecked: prefix + "/images/HeadlandSectionOff.png"
                checkable: true
                Layout.alignment: Qt.AlignCenter
                // Threading Phase 1: Headland section control
                isChecked: SettingsManager.headland_isSectionControlled
                onCheckedChanged: HeadacheInterface.isSectionControlled(checked)
            }
            Comp.IconButtonTransparent{
                //objectName: "btnALength"
                icon.source: prefix + "/images/APlusPlusA.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.alength()
            }
            Comp.IconButtonTransparent{
                //objectName: "btnAShrink"
                icon.source: prefix + "/images/APlusMinusA.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.ashrink()
            }
            Comp.IconButtonTransparent{
                //objectName: "btnAShrink"
                icon.source: prefix + "/images/HeadlandDeletePoints.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.cancelTouch()
            }
        }
        GridLayout{
            flow: Grid.LeftToRight
            rows: 3
            width: parent.width
            columns: 2
            rowSpacing: buttons.spacing
            Comp.IconButtonColor{
                id: headacheCurve
                objectName: "rbtnCurve"
                checkable: true
                //isChecked: true
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/ABTrackCurve.png"
                onClicked: HeadacheInterface.curveLine = true
            }
            Comp.IconButtonColor{
                id: headacheAB
                objectName: "rbtnLine"
                checkable: true
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/ABTrackAB.png"
                onClicked: HeadacheInterface.curveLine = false
            }
            Comp.SpinBoxM {
                //objectName: "nudSetDistance"
                from: 0
                to: 2000
                boundValue: numTracks.value * SettingsManager.vehicle_toolWidth
                Layout.alignment: Qt.AlignCenter
                Comp.TextLine{anchors.top: parent.bottom; text: "( "+ Utils.m_unit_abbrev()+" )"}

                onValueChanged: HeadacheInterface.lineDistance = value
            }
            Comp.SpinBoxCustomized{
                id: numTracks
                from: 0
                to: 10
                value: 0
                Layout.alignment: Qt.AlignCenter
                Comp.TextLine{anchors.top: parent.bottom; text: qsTr("Tool: ")+ Utils.m_to_ft_string(SettingsManager.vehicle_toolWidth)}
            }
            Comp.IconButtonColor{
                id: cboxIsZoom
                //objectName: "cboxIsZoom"
                checkable: true
                icon.source: prefix + "/images/ZoomOGL.png"
                Layout.alignment: Qt.AlignCenter
            }

            Comp.IconButtonTransparent{
                //objectName: "btnBndLoop"            rows: 2

                icon.source: prefix + "/images/HeadlandBuild.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.createHeadland()
            }
        }
        GridLayout{
            flow: Grid.LeftToRight
            rows: 2
            columns: 3
            rowSpacing: buttons.spacing
            width: parent.width
            Comp.IconButtonTransparent{
                //objectName: "btnDeletePoints"
                icon.source: prefix + "/images/HeadlandReset.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.deleteHeadland()
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/ABLineCycleBk.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.cycleBackward()
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/ABLineCycle.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.cycleForward()
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/SwitchOff.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: {
                    HeadacheInterface.headlandOff();
                    headacheDesigner.visible = false
                }
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/Trash.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: HeadacheInterface.deleteCurve()
            }
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/OK64.png"
                Layout.alignment: Qt.AlignCenter
                onClicked: {
                    HeadacheInterface.saveExit()
                    MainWindowState.isHeadlandOn = true
                    headacheDesigner.visible = false
                }
            }
        }
    }
}
