import QtQuick 2.15
import QtQuick.Window 2.15
import QtLocation
import QtPositioning
import QtQuick.Controls 2.15
import QtQuick.Layouts
import AOG
import ".."
import "../components"

Popup{
    id: boundaryOSM
    width: parent.width
    height: parent.height
    closePolicy: Popup.NoAutoClose

    property bool isInitialized: false
    property int currentPointsCount: 0

    Plugin {
        id: osmSatellitePlugin
        name: "osm"
        parameters: [
            PluginParameter {
                name: "osm.mapping.host"
                //value: "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%z/%y/%x"
                //value: "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"
                //value: "https://tiles.arcgis.com/tiles/nGt4QxSblgDfeJn9/arcgis/rest/services/WorldImagery/MapServer/tile/%z/%y/%x"
                value: "http://mt1.google.com/vt/lyrs=s&x=%x&y=%y&z=%z"
            }
        ]
    }

    function show(){
        boundaryOSM.visible = true

        if (!isInitialized) {
            initializeMap()
            isInitialized = true
        }
    }

    function initializeMap() {
        mapLoader.sourceComponent = null
        mapLoader.sourceComponent = osmMapComponent
    }

    function unloadMap() {
        mapLoader.sourceComponent = null

        pointsModel.clear()
        currentPointsCount = 0
        perimeter = 0

        isInitialized = false;
        addBoundary.checked = false
    }

    function buildPath() {
        var coords = [];
        for (var i = 0; i < pointsModel.count; i++) {
            var p = pointsModel.get(i);
            coords.push(QtPositioning.coordinate(p.latitude, p.longitude));
        }

        if (pointsModel.count >= 2) {
            var first = pointsModel.get(0);
            coords.push(QtPositioning.coordinate(first.latitude, first.longitude));
        }

        return coords;
    }

    function handleMapClick(mouseX, mouseY, mapItem) {
        if (!addBoundary.checked || !mapItem) return;

        var coord = mapItem.toCoordinate(Qt.point(mouseX, mouseY));
        if (!coord || !isFinite(coord.latitude) || !isFinite(coord.longitude)) return;

        BoundaryInterface.addBoundaryOSMPoint(Number(coord.latitude), Number(coord.longitude));
        pointsModel.append({
                               latitude: coord.latitude,
                               longitude: coord.longitude
                           });

        currentPointsCount = pointsModel.count;
        updateRouteLine(mapItem);
        calculatePerimeter();
    }

    function updateRouteLine(mapItem) {
        if (!mapItem) return;

        for (var i in mapItem.children) {
            var child = mapItem.children[i];
            if (child.hasOwnProperty("line") && child.hasOwnProperty("path")) {
                child.path = buildPath();
                break;
            }
        }
    }

    property real perimeter: 0

    function calculatePerimeter() {
        if (pointsModel.count < 2) {
            perimeter = 0;
            return 0;
        }

        var totalDistance = 0;

        for (var i = 0; i < pointsModel.count - 1; i++) {
            var point1 = pointsModel.get(i);
            var point2 = pointsModel.get(i + 1);

            totalDistance += calculateDistance(
                        point1.latitude, point1.longitude,
                        point2.latitude, point2.longitude
                        );
        }

        if (pointsModel.count > 2) {
            var firstPoint = pointsModel.get(0);
            var lastPoint = pointsModel.get(pointsModel.count - 1);

            totalDistance += calculateDistance(
                        lastPoint.latitude, lastPoint.longitude,
                        firstPoint.latitude, firstPoint.longitude
                        );
        }

        perimeter = totalDistance;
        return totalDistance;
    }

    function calculateDistance(lat1, lon1, lat2, lon2) {
        var coord1 = QtPositioning.coordinate(lat1, lon1);
        var coord2 = QtPositioning.coordinate(lat2, lon2);
        return coord1.distanceTo(coord2);
    }

    function formatDistance(meters) {
        if (meters < 1) {
            return "0 м";
        } else if (meters < 1000) {
            return meters.toFixed(0) + " м";
        } else {
            return (meters / 1000).toFixed(2) + " км";
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

        Loader {
            id: mapLoader
            anchors.fill: parent
        }

        Component {
            id: osmMapComponent

            Map {
                id: osmMap
                anchors.fill: parent
                plugin: osmSatellitePlugin
                center:  QtPositioning.coordinate(Backend.fixFrame.latitude, Backend.fixFrame.longitude)
                zoomLevel: 15
                copyrightsVisible: true
                activeMapType: supportedMapTypes[supportedMapTypes.length - 1]

                MapQuickItem {
                    coordinate: QtPositioning.coordinate(Backend.fixFrame.latitude, Backend.fixFrame.longitude)
                    anchorPoint: Qt.point(8, 8)
                    sourceItem: Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "green"
                        border.color: "white"
                        border.width: 1
                    }
                }

                MapPolyline {
                    id: osmRouteLine
                    line.width: 3
                    line.color: "blue"
                    opacity: 0.8
                    path: buildPath()
                }

                MapItemView {
                    model: pointsModel
                    delegate: MapQuickItem {
                        coordinate: QtPositioning.coordinate(model.latitude, model.longitude)
                        anchorPoint: Qt.point(8, 8)
                        sourceItem: Rectangle {
                            width: 12; height: 12; radius: 6
                            color: "red"
                            border.color: "white"
                            border.width: 1
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: addBoundary.checked
                    onClicked: (mouse) => {
                                   handleMapClick(mouse.x, mouse.y, osmMap);
                               }
                }

                DragHandler {
                    id: drag
                    target: null
                    onTranslationChanged: (delta) => osmMap.pan(-delta.x, -delta.y)
                }

                PinchHandler {
                    id: pinch
                    target: null
                    onActiveChanged: if (active) {
                                         osmMap.startCentroid = osmMap.toCoordinate(pinch.centroid.position, false)
                                     }
                    onScaleChanged: (delta) => {
                                        osmMap.zoomLevel += Math.log2(delta)
                                        osmMap.alignCoordinateToPoint(osmMap.startCentroid, pinch.centroid.position)
                                    }
                    onRotationChanged: (delta) => {
                                           osmMap.bearing -= delta
                                           osmMap.alignCoordinateToPoint(osmMap.startCentroid, pinch.centroid.position)
                                       }
                    grabPermissions: PointerHandler.TakeOverForbidden
                }

                WheelHandler {
                    id: wheel
                    acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
                                     ? PointerDevice.Mouse | PointerDevice.TouchPad
                                     : PointerDevice.Mouse
                    rotationScale: 1/120
                    property: "zoomLevel"
                }
            }
        }
    }

    GridLayout{
        id: buttons
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        columnSpacing: 50* theme.scaleWidth
        flow: Grid.LeftToRight
        columns: 2
        rows: 7

        IconButtonColor{
            id: addBoundary
            icon.source: prefix + "/images/BoundaryOuter.png"
            checkable: true
            Layout.alignment: Qt.AlignCenter
            implicitWidth: deletePoint.width
            implicitHeight: deletePoint.height
        }

        IconButtonTransparent{
            id: deletePoints
            icon.source: prefix + "/images/Trash.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                pointsModel.clear();
                BoundaryInterface.reset();
                currentPointsCount = 0;

                if (mapLoader.item) {
                    updateRouteLine(mapLoader.item);
                    calculatePerimeter();
                }
            }
        }

        IconButtonTransparent{
            id: addPoint
            icon.source: prefix + "/images/AddNew.png"
            Layout.alignment: Qt.AlignCenter
            enabled: addBoundary.checked
            onClicked: {
                BoundaryInterface.stop();
                pointsModel.clear();
                currentPointsCount = 0;

                if (mapLoader.item) {
                    updateRouteLine(mapLoader.item);
                    calculatePerimeter();
                }
            }
        }

        Text {
            id: points
            Layout.alignment: Qt.AlignCenter
            font.bold: true
            font.pixelSize: 15
            text: qsTr("Points: \n") + " " + currentPointsCount
        }

        IconButtonTransparent{
            id: deletePoint
            icon.source: prefix + "/images/PointDelete.png"
            Layout.alignment: Qt.AlignCenter
            enabled: addBoundary.checked && currentPointsCount > 0
            onClicked: {
                if (pointsModel.count > 0) {
                    pointsModel.remove(pointsModel.count - 1);
                    BoundaryInterface.delete_last_point();
                    currentPointsCount = pointsModel.count;

                    if (mapLoader.item) {
                        updateRouteLine(mapLoader.item);
                        calculatePerimeter();
                    }
                }
            }
        }

        Text {
            id: bounds
            Layout.alignment: Qt.AlignCenter
            font.bold: true
            font.pixelSize: 15
            text: qsTr("Area: \n") + " " + Utils.area_to_unit_string(BoundaryInterface.area,1) + " " + Utils.area_unit() + qsTr("\nПериметр: \n") + " " + formatDistance(perimeter)
        }

        IconButtonTransparent{
            icon.source: prefix + "/images/ZoomOut48.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                if (mapLoader.item) {
                    mapLoader.item.zoomLevel = Math.round(mapLoader.item.zoomLevel - 1);
                }
            }
        }

        IconButtonTransparent{
            icon.source: prefix + "/images/ZoomIn48.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                if (mapLoader.item) {
                    mapLoader.item.zoomLevel = Math.round(mapLoader.item.zoomLevel + 1);
                }
            }
        }

        IconButtonTransparent{
            icon.source: prefix + "/images/FlagGrn.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                if (mapLoader.item) {
                    mapLoader.item.center = QtPositioning.coordinate(Backend.fixFrame.latitude, Backend.fixFrame.longitude);
                }
            }
        }

        IconButtonTransparent{
            icon.source: prefix + "/images/OK64.png"
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                pointsModel.clear();
                BoundaryInterface.reset()
                boundaryOSM.visible = false;
                unloadMap()
            }
        }
    }


    ListModel {
        id: pointsModel
    }
}
