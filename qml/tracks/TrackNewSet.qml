import QtQuick
import QtQuick.Controls
//import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQml.Models
import AOG
// Interface import removed - now QML_SINGLETON

import "../components"
import ".."

/*This is where the track is actually created*/

//region SetAB
//
Item{
    id: trackNewSet
    anchors.fill: parent
    function showAPlus() {
        setAPlus.visible = true
    }
    function showAB() {
        setAB.visible = true
    }
    function showABCurve() {
        setABCurve.visible = true
    }
    function showLatLonLatLon() {
        latLonLatLon.visible = true
    }
    function showLatLonHeading() {
        latLonHeading.visible = true
    }
    function showCircleTrack() {
        circleTrack.visible = true
    }
    function showFromKML() {
        loadFromKML.open()
    }
    //region setA+

    MoveablePopup {
        id: setAPlus
        visible: false
        width: 250 * theme.scaleWidth
        height: 350 * theme.scaleHeight
        GridLayout {
            id: setAPlusLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.LeftToRight
            rows: 3
            columns: 2

            // pick left or right
            IconButtonTransparent {
                id: setAPRefSide
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/BoundaryRight.png"
                property bool refSideRight: true

                onClicked: {
                    refSideRight = !refSideRight
                    if (refSideRight)
                        TracksInterface.newRefSide = 1
                    else
                        TracksInterface.newRefSide = -1

                    if (refSideRight) {
                        icon.source = prefix + "/images/BoundaryRight.png"
                    } else {
                        icon.source = prefix + "/images/BoundaryLeft.png"
                    }
                }
            }
            IconButtonTransparent {
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/LetterABlue.png"
                onClicked: {
                    aPlusHeading.enabled = true
                    aPlusHeading.text = (Number(Utils.radians_to_deg(Backend.fixFrame.heading)).toLocaleString(Qt.locale(), 'f', 4))
                    btnAPlusOk.enabled = true
                    TracksInterface.start_new(2)
                    TracksInterface.newRefSide = setAPRefSide.refSideRight ? 1 : -1
                    TracksInterface.mark_start(Backend.fixFrame.easting, Backend.fixFrame.northing, Number(aPlusHeading.text))
                    TracksInterface.newHeading = -1
                    TracksInterface.newHeading = Utils.deg_to_radians(Number(aPlusHeading.text))
                }
            }
            NumberTextField {
                id: aPlusHeading
                enabled: false
                bottomVal: 0
                topVal: 360
                decimals: 4
                text: "0"
                Layout.columnSpan: 2
                implicitWidth: 200
                implicitHeight: 50

                onTextChanged: {
                    TracksInterface.newHeading = (Utils.deg_to_radians(Number(text)))
                }
            }
            IconButtonTransparent {
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: {
                    setAPlus.visible = false
                    TracksInterface.cancel_new()
                }
            }
            IconButtonTransparent {
                id: btnAPlusOk
                Layout.alignment: Qt.AlignCenter
                enabled: false
                icon.source: prefix + "/images/OK64.png"
                onClicked: {
                    setAPlus.visible = false
                    trackAddName.show(TracksInterface.newName)
                }
            }
        }
    }
    //endregion setA+
    //
    //region setAB
    MoveablePopup {
        id: setAB
        visible: false
        width: 250 * theme.scaleWidth
        height: 350 * theme.scaleHeight
        GridLayout {
            id: setABLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.LeftToRight
            rows: 3
            columns: 2

            // pick left or right
            IconButtonTransparent {
                id: setABRefSide
                Layout.alignment: Qt.AlignCenter
                Layout.columnSpan: 2
                icon.source: prefix + "/images/BoundaryRight.png"
                property bool refSideRight: true

                onClicked: {
                    refSideRight = !refSideRight
                    if (refSideRight)
                        TracksInterface.newRefSide = 1
                    else
                        TracksInterface.newRefSide = -1

                    if (refSideRight) {
                        icon.source = prefix + "/images/BoundaryRight.png"
                    } else {
                        icon.source = prefix + "/images/BoundaryLeft.png"
                    }
                }
            }
            IconButtonTransparent {
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/LetterABlue.png"
                onClicked: {
                    btnB.enabled = true
                    TracksInterface.start_new(2)
                    TracksInterface.newRefSide = setABRefSide.refSideRight ? 1 : -1
                    TracksInterface.mark_start(Backend.fixFrame.easting, Backend.fixFrame.northing, Backend.fixFrame.heading)
                }
            }
            IconButtonTransparent {
                id: btnB
                Layout.alignment: Qt.AlignCenter
                enabled: false
                icon.source: prefix + "/images/LetterBBlue.png"
                onClicked: {
                    btnABOk.enabled = true
                    TracksInterface.mark_end( setABRefSide.refSideRight ? 1 : -1, Backend.fixFrame.easting, Backend.fixFrame.northing)
                }

            }
            IconButtonTransparent {
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: {
                    setAB.visible = false
                    TracksInterface.cancel_new()
                }
            }
            IconButtonTransparent {
                id: btnABOk
                Layout.alignment: Qt.AlignCenter
                enabled: false
                icon.source: prefix + "/images/OK64.png"
                onClicked: {
                    setAB.visible = false
                    trackAddName.show(TracksInterface.newName)
                }
            }
        }
    }
    //endregion setAB
    //
    //region setABCurve
    MoveablePopup {
        id: setABCurve
        visible: false
        width: 250 * theme.scaleWidth
        height: 350 * theme.scaleHeight
        GridLayout {
            id: setABCurveLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.LeftToRight
            rows: 4
            columns: 2

            // pick left or right
            IconButtonTransparent {
                id: setABCurveRefSide
                Layout.alignment: Qt.AlignCenter
                Layout.columnSpan: 2
                icon.source: prefix + "/images/BoundaryRight.png"
                property bool refSideRight: true
                onClicked: {
                    refSideRight = !refSideRight
                    if (refSideRight)
                        TracksInterface.newRefSide = 1
                    else
                        TracksInterface.newRefSide = -1

                    if (refSideRight) {
                        icon.source = prefix + "/images/BoundaryRight.png"
                    } else {
                        icon.source = prefix + "/images/BoundaryLeft.png"
                    }
                }
            }
            IconButtonTransparent {
                id: btnACurve
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/LetterABlue.png"
                onClicked: {
                    btnBCurve.enabled = true
                    btnRecord.enabled = true
                    TracksInterface.start_new(4)
                    TracksInterface.newRefSide = setABCurveRefSide.refSideRight ? 1 : -1
                    TracksInterface.mark_start(Backend.fixFrame.easting, Backend.fixFrame.northing, Backend.fixFrame.heading)
                }
            }
            IconButtonTransparent {
                id: btnBCurve
                Layout.alignment: Qt.AlignCenter
                enabled: false
                icon.source: prefix + "/images/LetterBBlue.png"
                onClicked: {
                    setABCurve.visible = false
                    TracksInterface.mark_end(setABCurveRefSide.refSideRight ? 1 : -1, Backend.fixFrame.easting, Backend.fixFrame.northing)
                    trackAddName.show(TracksInterface.newName)
                }
            }
            Text {
                Layout.alignment: Qt.AlignCenter
                Layout.columnSpan: 2
                text: qsTr("Status: Driving")
                color: aogInterface.textColor
            }
            IconButtonTransparent {
                id: btnRecord
                Layout.alignment: Qt.AlignCenter
                enabled: false
                checkable: true
                icon.source: prefix + "/images/boundaryPause.png"
                iconChecked: prefix + "/images/BoundaryRecord.png"
                onCheckedChanged: {
                    if (checked) {
                        btnBCurve.enabled = false
                        TracksInterface.pause(true)
                    } else {
                        btnBCurve.enabled = true
                        TracksInterface.pause(false)
                    }
                }
            }
            IconButtonTransparent {
                Layout.alignment: Qt.AlignCenter
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: {
                    setABCurve.visible = false
                    TracksInterface.cancel_new()
                }
            }
        }
    }
    //endregion setABCurve
    //
    //region latLonLatLon
    MoveablePopup {
        id: latLonLatLon
        visible: false
        width: 350 * theme.scaleWidth
        height: 450 * theme.scaleHeight
        onVisibleChanged: {
            if (visible) {
                latPointA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7))
                lonPointA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                latPointB.text = parseFloat((Backend.fixFrame.latitude).toFixed(7)) //Backend.fixFrame.latitude
                lonPointB.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude

                TracksInterface.start_new(2)
                TracksInterface.newRefSide = 0; //in this mode ref line is where the tractor is

                update_ab();
            }
        }

        function update_ab() {
            var pta = Backend.pn.convertWGS84ToLocal(Number(latPointA.text),
                                              Number(lonPointA.text))
            var ptb = Backend.pn.convertWGS84ToLocal(Number(latPointB.text),
                                              Number(lonPointB.text))

            TracksInterface.mark_start(pta[1], pta[0], 0)
            TracksInterface.mark_end(0, ptb[1], ptb[0])
        }

        GridLayout {
            id: latLonLatLonLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.TopToBottom
            rows: 8
            columns: 2

            // send current position to point A
            IconButtonTransparent {
                icon.source: prefix + "/images/Config/ConS_ImplementAntenna.png"
                Layout.rowSpan: 3
                Layout.row: 2
                Layout.column: 0
                onClicked: {
                    latPointA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7)) //Backend.fixFrame.latitude
                    lonPointA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                }
            }

            // send current position to point B
            IconButtonTransparent {
                icon.source: prefix + "/images/Config/ConS_ImplementAntenna.png"
                Layout.rowSpan: 3
                Layout.row: 5
                Layout.column: 0
                onClicked: {
                    latPointB.text = parseFloat((Backend.fixFrame.latitude).toFixed(7)) //Backend.fixFrame.latitude
                    lonPointB.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                }
            }
            /*
             * figure out rounding
             * we don't want to convert to locale string
             */
            IconButtonTransparent {
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: {
                    latLonLatLon.visible = false
                    TracksInterface.cancel_new()
                }
                Layout.row: 8
                Layout.column: 0
            }

            //this starts on the top of the right column
            NumberTextField {
                id: latPointA
                Layout.alignment: Qt.AlignRight
                Layout.row: 1
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Latitude (+- 90)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonLatLon.update_ab()
                }
            }
            Text {
                Layout.alignment: Qt.AlignRight
                Layout.column: 1
                Layout.row: 2
                text: qsTr("A")
            }
            NumberTextField {
                id: lonPointA
                Layout.alignment: Qt.AlignRight
                Layout.row: 3
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Longitude (+- 180)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonLatLon.update_ab()
                }
            }
            NumberTextField {
                id: latPointB
                Layout.alignment: Qt.AlignRight
                Layout.row: 5 // skip a row
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Latitude (+- 90)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonLatLon.update_ab()
                }
            }
            Text {
                Layout.alignment: Qt.AlignRight
                Layout.column: 1
                Layout.row: 6
                text: qsTr("B")
            }
            NumberTextField {
                id: lonPointB
                Layout.alignment: Qt.AlignRight
                Layout.row: 7
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Longitude (+- 180)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonLatLon.update_ab()
                }
            }
            IconButtonTransparent {
                icon.source: prefix + "/images/OK64.png"
                Layout.row: 8
                Layout.column: 1
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    latLonLatLon.visible = false
                    trackAddName.show(TracksInterface.newName)
                }
            }
        }
    }
    //endregion latLonLatLon
    //
    //region latLonHeading
    MoveablePopup {
        id: latLonHeading
        visible: false
        width: 350 * theme.scaleWidth
        height: 450 * theme.scaleHeight
        onVisibleChanged: {
            if (visible) {
                latPointAA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7))
                lonPointAA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                latLonHeadingEntry.text = parseFloat((Utils.radians_to_deg(Backend.fixFrame.heading)).toFixed(4))
                TracksInterface.start_new(2)
                TracksInterface.newRefSide = 0; //in this mode ref line is where the tractor is

                update_a_heading()
            }
        }

        function update_a_heading() {
            var pta = Backend.pn.convertWGS84ToLocal(Number(latPointAA.text),
                                              Number(lonPointAA.text))

            TracksInterface.mark_start(pta[1], pta[0], 0)
            TracksInterface.newHeading = Utils.deg_to_radians(Number(latLonHeadingEntry.text))
        }

        GridLayout {
            id: latLonHeadingLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.TopToBottom
            rows: 4
            columns: 2

            // send current position to point A
            IconButtonTransparent {
                icon.source: prefix + "/images/Config/ConS_ImplementAntenna.png"
                Layout.rowSpan: 2
                Layout.column: 0
                onClicked: {
                    latPointAA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7)) //Backend.fixFrame.latitude
                    lonPointAA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                }
            }

            IconButtonTransparent {
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: {
                    latLonHeading.visible = false
                    TracksInterface.cancel_new()
                }
                Layout.row: 4
                Layout.column: 0
            }

            //this starts on the top of the right column
            NumberTextField {
                id: latPointAA
                Layout.alignment: Qt.AlignRight
                Layout.row: 1
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Latitude (+- 90)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonHeading.update_a_heading()
                }
            }
            NumberTextField {
                id: lonPointAA
                Layout.alignment: Qt.AlignRight
                Layout.row: 2
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Longitude (+- 180)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonHeading.update_a_heading()
                }
            }
            NumberTextField {
                id: latLonHeadingEntry
                bottomVal: 0
                topVal: 360
                decimals: 4
                text: "0"
                Layout.row: 3
                Layout.column: 1
                Layout.alignment: Qt.AlignRight
                implicitWidth: 200
                implicitHeight: 50
                TextLine {
                    text: qsTr("Heading")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
                onTextChanged: {
                    latLonHeading.update_a_heading()
                }
            }
            IconButtonTransparent {
                icon.source: prefix + "/images/OK64.png"
                Layout.row: 4
                Layout.column: 1
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    latLonHeading.visible = false
                    trackAddName.show(TracksInterface.newName)
                }
            }
        }
    }
    //endregion latLonHeading
    //
    //region circleTrack
    MoveablePopup {
        id: circleTrack
        visible: false
        width: 350 * theme.scaleWidth
        height: 450 * theme.scaleHeight
        onVisibleChanged: {
            if (visible) {
                latPointAAA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7))
                lonPointAAA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
            }
        }
        GridLayout {
            id: circleTrackLayout
            anchors.centerIn: parent
            width: parent.width *.9
            height: parent.height *.9
            flow: Grid.TopToBottom
            rows: 4
            columns: 2

            // send current position to point A
            IconButtonTransparent {
                icon.source: prefix + "/images/Config/ConS_ImplementAntenna.png"
                Layout.rowSpan: 2
                Layout.column: 0
                onClicked: {
                    latPointAAA.text = parseFloat((Backend.fixFrame.latitude).toFixed(7)) //Backend.fixFrame.latitude
                    lonPointAAA.text = parseFloat((Backend.fixFrame.longitude).toFixed(7)) //Backend.fixFrame.longitude
                }
            }

            IconButtonTransparent {
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: circleTrack.visible = false
                Layout.row: 3
                Layout.column: 0
            }

            //this starts on the top of the right column
            NumberTextField {
                id: latPointAAA
                Layout.alignment: Qt.AlignRight
                Layout.row: 1
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Latitude (+- 90)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
            }
            NumberTextField {
                id: lonPointAAA
                Layout.alignment: Qt.AlignRight
                Layout.row: 2
                Layout.column: 1
                implicitWidth: 200 * theme.scaleWidth
                bottomVal: -90
                topVal: 90
                decimals: 7
                text: "0"
                TextLine {
                    text: qsTr("Longitude (+- 180)")
                    anchors.bottom: parent.top
                    anchors.left: parent.left
                }
            }
            IconButtonTransparent {
                icon.source: prefix + "/images/OK64.png"
                Layout.row: 3
                Layout.column: 1
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    circleTrack.visible = false
                    trackAddName.show("Cir")
                }
            }
        }
    }
    //endregion circleTrack
    //
    //region loadFromKML
    FileDialog {
        id: loadFromKML
        //Should be populated from backend?
        //currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/QtAgOpenGPS/Fields"
        nameFilters: ["KML Files (*.kml)"]
    }
   }
