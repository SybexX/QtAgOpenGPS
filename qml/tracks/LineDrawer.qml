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
import AOG
//import AgOpenGPS 1.0

Popup{
	id: lineDrawer
    width: parent.width
    height: parent.height
    //color: "ghostwhite"
    closePolicy: Popup.NoAutoClose
    
    // Properties to hold various data
    property var boundaryLines: []
    property var headlandLine: []
    property int sliceCount: 0
    property QtObject headlandAB: QtObject {
        property bool checked: false
    }
    
	function show(){
		lineDrawer.visible = true
    }

    Comp.TopLine{
		id: topLine
        titleText: qsTr("Click 2 points on the Boundary to Begin")
	}
	Rectangle{
		anchors.top: topLine.bottom
		anchors.left: parent.left
		anchors.right: buttons.left
		anchors.bottom:  parent.bottom
		color: "black"

        Rectangle {//renderer goes here
            id: lineDrawerField
            objectName: "lineDrawerField"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            width: parent.width > parent.height ? parent.height : parent.width
            height: width  //1:1 aspect ratio
            //width: parent.width * .7
            color: "black"

            Rectangle {
                id: a_rect
                visible: HeadlandInterface.showa
                width: 24
                height: 24
                radius: 12
                color: "#ffc059"
                x: HeadlandInterface.apoint.x - 12
                y: HeadlandInterface.apoint.y - 12
                z: 1
            }

            Rectangle {
                id: b_rect
                visible: HeadlandInterface.showb
                width: 24
                height: 24
                radius: 12
                color:  "#80c0ff"
                x: HeadlandInterface.bpoint.x - 12
                y: HeadlandInterface.bpoint.y - 12
                z: 1
            }

            Rectangle {
                id: vehicle_point
                visible: true
                width: 24
                height: 24
                radius: 12
                color:  "#f33033"
                x: HeadlandInterface.vehiclePoint.x - 12
                y: HeadlandInterface.vehiclePoint.y - 12
            }

            Repeater {
                id: boundaryRepeater

                model: boundaryLines.length

                Shape {
                    property int outerIndex: index
                    smooth: true

                    anchors.fill: parent
                    Connections {
                        target: HeadlandInterface
                        function onBoundaryLinesChanged() {
                            shapePath.draw_boundaries()
                        }
                    }

                    ShapePath {
                        id: shapePath
                        strokeColor: boundaryLines[index].color
                        strokeWidth: boundaryLines[index].width
                        fillColor: "transparent"
                        startX: p[0].x
                        startY: p[0].y
                        scale: Qt.size(1,1)
                        joinStyle: ShapePath.RoundJoin

                        property var p: [Qt.point(0,0), Qt.point(lineDrawerField.width, lineDrawerField.height)]

                        PathPolyline {
                            id: ps
                            path: shapePath.p && shapePath.p.length > 0 ? shapePath.p : []
                        }


                        Component.onCompleted: draw_boundaries()


                        function draw_boundaries()
                        {
                        //    console.debug(boundaryLines[index].points)
                            p = boundaryLines[index].points
                        }
                    }
                }
            }

            Shape {
                id: headlandShape
                visible: headlandLine.length > 0
                anchors.fill: parent
                ShapePath {
                    id: headlandShapePath
                    strokeColor: "#f1e817"
                    strokeWidth: 8
                    fillColor: "transparent"
                    startX: p[0].x
                    startY: p[0].y
                    joinStyle: ShapePath.RoundJoin

                    property var p: [
                        Qt.point(0,0),
                        Qt.point(20,100),
                        Qt.point(200,150)
                    ]

                    PathPolyline {
                        id: headlandShapePolyine
                        path: headlandShapePath.p && headlandShapePath.p.length > 0 ? headlandShapePath.p : []
                    }
                }
            }

            Shape {
                id: sliceShape
                visible: sliceCount != 0
                anchors.fill: parent
                ShapePath {
                    id: sliceShapePath
                    strokeColor: headlandAB.checked ? "#f31700" : "#21f305"
                    strokeWidth: 8
                    fillColor: "transparent"
                    startX: p[0].x
                    startY: p[0].y
                    joinStyle: ShapePath.RoundJoin

                    property var p: [
                        Qt.point(0,0),
                        Qt.point(100,20),
                    ]

                    PathPolyline {
                        id: sliceShapePolyLine
                        path: sliceShapePath.p && sliceShapePath.p.length > 0 ? sliceShapePath.p : []
                    }
                }
            }

            MouseArea {
                id: headlandMouseArea
                anchors.fill: parent

                property int fromX: 0
                property int fromY: 0

                onClicked: {
                    if (cboxIsZoom.checked && HeadlandInterface.zoom === 1) {
                        sX = ((parent.width / 2 - mouseX) / parent.width) * 1.1
                        sY = ((parent.height / 2 - mouseY) / -parent.height) * 1.1
                        //console.debug("width,mouse, sx,sy",parent.width / 2, mouseX, mouseY, sX,sY);
                        zoom = 0.1
                        HeadlandInterface.updateLines()
                    } else {
                        HeadlandInterface.mouseClicked(mouseX, mouseY)
                        if (zoom != 1.0) {
                            zoom = 1.0;
                            sX = 0;
                            sY = 0;
                            HeadlandInterface.updateLines()
                        }
                    }
                }

                onPressed: {
                    //save a copy of the coordinates
                    fromX = mouseX
                    fromY = mouseY
                }

                onPositionChanged: {
                    HeadlandInterface.mouseDragged(fromX, fromY, mouseX, mouseY)
                    fromX = mouseX
                    fromY = mouseY
                }

                //onWheel: {}
            }
        }
	}
	Rectangle{
		id: buttons
		color: "lightgray"
		width: 250
		anchors.bottom: parent.bottom
		anchors.top: topLine.bottom
		anchors.right: parent.right

		//  I'll add this later, not sure how it works now.
		GridLayout{
			id: top6Buttons
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			anchors.left: parent.left
			anchors.margins: 5
			columns: 2
			rows: 9
			flow: Grid.LeftToRight
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/APlusPlusB.png"
                Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/MappingOff.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/APlusPlusA.png"
                Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/HeadlandDeletePoints.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/TrackVisible.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				id: boundaryCurve
				icon.source: prefix + "/images/BoundaryCurveLine.png"
				Layout.alignment: Qt.AlignCenter
                text: qsTr("Boundary Curve")
			}
            Comp.IconButtonTransparent{
                icon.source: prefix + "/images/ZoomOGL.png"
                Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/Trash.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/ABTrackCurve.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/ABTrackAB.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/ABLineCycleBk.png"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/ABLineCycle.png"
				Layout.alignment: Qt.AlignCenter
			}
			Rectangle{
				id: curveNameInput
				width: parent.width - 10
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignCenter
				color: "ghostwhite"
				border.color: "darkgray"
				border.width: 1
				height: 40
				TextInput{
					id: newLine
					objectName: "drawerCurveName"
					anchors.fill: parent
				}
			}
			Text{
				text: "1/16"
				Layout.alignment: Qt.AlignCenter
			}
            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/Time.png"
				Layout.alignment: Qt.AlignCenter
				onClicked: {
					var time = new Date().toLocaleTimeString(Qt.locale())
					newLine.text += " " + time
				}
			}

            Comp.IconButtonTransparent{
				icon.source: prefix + "/images/Cancel64.png"
				Layout.alignment: Qt.AlignCenter
				onClicked: lineDrawer.visible = false
			}

            Comp.IconButtonTransparent{
				objectName: "btnDrawerSave"
				icon.source: prefix + "/images/OK64.png"
				onClicked: lineDrawer.visible = false
				Layout.alignment: Qt.AlignCenter
			}
		}
	}
}
