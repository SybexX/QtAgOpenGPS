// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
// Displays the main QML page. All other QML windows are children of this one.
//Loaded by formgps_ui.cpp.
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import QtQuick.Dialogs
// Interface import removed - now QML_SINGLETON
import AOG
import "agio" as AgIOModule


import "interfaces" as Interfaces
import "boundary" as Boundary
import "steerconfig" as SteerConfig
import "config" as ConfigSettings //"Config" causes errors
import "field" as Field
import "tracks" as Tracks
import "components" as Comp
import "wizards" as Wiz

Window {
    //We draw native opengl to this root object
    id: mainWindow
    objectName: "mainWindow"
    visibility: SettingsManager.display_isStartFullscreen ? Window.FullScreen : Window.Automatic
    // ⚡ Qt 6.8 Modern Pattern: Simple initialization notification
    // No complex property bindings or signal handlers needed

    // Function to toggle the FieldView test window
    function toggleFieldViewTest() {
        fieldViewTestWindow.visible = !fieldViewTestWindow.visible
    }

    // Keyboard shortcut to toggle FieldView test window (Ctrl+Shift+F)
    Shortcut {
        sequence: "Ctrl+Shift+F"
        onActivated: toggleFieldViewTest()
    }

    signal keyPressed(int val)
    signal hotKeyPressed(int index)

    Instantiator {
        model: SettingsManager.key_hotKeys
        delegate: Shortcut {
            enabled: !hotKeySettings.visible
            sequence: modelData
            context: Qt.ApplicationShortcut
            onActivated: hotKeyPressed(index)
        }
    }

    LoggingCategory {
        id: qmlLog

        name: "qml.qtagopengps"
    }

    AOGTheme {
        id: theme
        objectName: "theme"
    }

    AOGInterface {
        id: aogInterface  // Renamed to avoid conflict with global aog
        objectName: "aogInterface"
    }
    //property string prefix: "../.." //make image show in QtDS

    height: theme.defaultHeight
    color: "#0d0d0d"
    width: theme.defaultWidth

    // onVisibleChanged: if(SettingsManager.display_isStartFullscreen){
    //                       mainWindow.showMaximized()
    //                   }

    Component.onCompleted: {
        // Singletons created on first access. No need to check if they
        // exist.
        console.log(qmlLog, "=== FACTORY FUNCTION DEBUG ===")
        console.log(qmlLog, "Settings available:", typeof SettingsManager !== 'undefined')
        console.log(qmlLog, "TracksInterface available:", typeof TracksInterface !== 'undefined')
        console.log(qmlLog, "VehicleInterface available:", typeof VehicleInterface !== 'undefined')
        console.log(qmlLog, "AgIOService available:", typeof AgIOService !== 'undefined')

        console.log(qmlLog, "Settings.display_isStartFullscreen:", SettingsManager.display_isStartFullscreen)

        console.log(qmlLog, "=== AGIO SERVICE TEST ===")
        console.log(qmlLog, "GPS Connected:", AgIOService.gpsConnected)
        console.log(qmlLog, "Latitude:", Backend.fixFrame.latitude)
        console.log(qmlLog, "Longitude:", Backend.fixFrame.longitude)
        console.log(qmlLog, "Vehicle XY:", VehicleInterface.screenCoord)
        console.log(qmlLog, "Thread test:")
        AgIOService.testThreadCommunication()
        console.log(qmlLog, "=== END AGIO TEST ===")

        console.log(qmlLog, "TracksInterface.idx:", TracksInterface.idx)
        console.log(qmlLog, "TracksInterface.count:", TracksInterface.count)
        console.log(qmlLog, "TracksInterface.model:", TracksInterface.model)
        console.log(qmlLog, "TracksInterface identity:", TracksInterface)

        console.log(qmlLog, "VehicleInterface.isReverse:", VehicleInterface.isReverse)
        console.log(qmlLog, "VehicleInterface.vehicleList length:", VehicleInterface.vehicleList ? VehicleInterface.vehicleList.length : "undefined")
        console.log(qmlLog, "VehicleInterface identity:", VehicleInterface)

        // AgIOSettings debug removed - replaced by AgIOService in Phase 4.2

        console.log(qmlLog, "=== END FACTORY FUNCTION DEBUG ===")

        // Phase 6.0.20 Task 24 Step 3.5 - Test geodetic conversion functions
        console.log(qmlLog, "[GEODETIC_TEST] latStart:", Backend.pn.latStart, "lonStart:", Backend.pn.lonStart )
        if (Backend.pn.latStart !== 0 && Backend.pn.lonStart !== 0) {
            var local = Backend.pn.convertWGS84ToLocal(Backend.pn.latStart, Backend.pn.lonStart)
            console.log(qmlLog, "[GEODETIC_TEST] WGS84->Local origin conversion: northing=", local[0], "easting=", local[1])
            var wgs84 = Backend.pn.convertLocalToWGS84(local[0], local[1])
            console.log(qmlLog, "[GEODETIC_TEST] Local->WGS84 round-trip: lat=", wgs84[0], "lon=", wgs84[1])
        } else {
            console.log(qmlLog, "[GEODETIC_TEST] Field origin not set - skipping conversion test")
        }

        // ⚡ Qt 6.8 Pattern: Component is ready
        console.log(qmlLog, "✅ QML MainWindow Component.onCompleted")
        // C++ will be notified via objectCreated signal automatically
    }

    // Phase 6.0.20 Task 24 Step 3.5 - Test when field is loaded
    Connections {
        target: Backend.pn
        function onLatStartChanged() {
            if (Backend.pn.latStart !== 0 && Backend.pn.lonStart !== 0) {
                console.log(qmlLog, "[GEODETIC_TEST] Field loaded - latStart:", Backend.pn.latStart, "lonStart:", Backend.pn.lonStart, "mPerDegreeLat:", Backend.pn.mPerDegreeLat)
                var local = Backend.pn.convertWGS84ToLocal(Backend.pn.latStart, Backend.pn.lonStart)
                console.log(qmlLog, "[GEODETIC_TEST] WGS84->Local origin: northing=", local[0], "easting=", local[1])
                var wgs84 = Backend.pn.convertLocalToWGS84(local[0], local[1])
                console.log(qmlLog, "[GEODETIC_TEST] Local->WGS84 round-trip: lat=", wgs84[0], "lon=", wgs84[1])
            }
        }
    }

    // REMOVED: save_everything signal replaced by formGPS.applicationClosing property
    // signal save_everything(bool saveVehicle)

    function close() {
        if (areWindowsOpen()) {
            timedMessage.addMessage(2000,qsTr("Some windows are open. Close them first."))
            console.log(qmlLog, "some windows are open. close them first")
            return
        }
        if (MainWindowState.autoBtnState + MainWindowState.manualBtnState  > 0) {
            timedMessage.addMessage(2000,qsTr("Section Control on. Shut off Section Control."))
            close.accepted = false
            console.log(qmlLog, "Section Control on. Shut off Section Control.")
            return
        }
        if (mainWindow.visibility !== (Window.FullScreen) && mainWindow.visibility !== (Window.Maximized)){
            SettingsManager.window_size = ((mainWindow.width.toString() + ", "+  (mainWindow.height).toString()))
        }

        if (Backend.isJobStarted) {
            closeDialog.visible = true
            close.accepted = false
            console.log(qmlLog, "job is running. close it first")
            return
        }
        Qt.quit()
    }
    function areWindowsOpen() {
        if (config.visible === true) {
            console.log(qmlLog, "config visible")
            return true
        }
        else if (headlandDesigner.visible === true) {
            console.log(qmlLog, "headlandDesigner visible")
            return true
        }
        else if (headacheDesigner.visible === true) {
            console.log(qmlLog, "headacheDesigner visible")
            return true
        }
        else if (steerConfigWindow.visible === true) {
            console.log(qmlLog, "steerConfigWindow visible")
            return true
        }
        /*
        else if (abCurvePicker.visible === true) {
            console.log(qmlLog, "abCurvePicker visible")
            return true
        }
        else if (abLinePicker.visible === true) {
            console.log(qmlLog, "abLinePicker visible")
            return true
        }*/
        else if (tramLinesEditor.visible === true) {
            console.log(qmlLog, "tramLinesEditor visible")
            return true
        }
        else if (lineEditor.visible === true) {
            console.log(qmlLog, "lineEditor visible")
            return true
        }
        //if (boundaryMenu.visible == true) return false
        //if (lineDrawer.visible) return false
        //if (lineNudge.acive) return false
        //if (refNudge.acive) return false
        else if (setSimCoords.visible === true) {
            console.log(qmlLog, "setSimCoords visible")
            return true
        }
        /* Must implement the new track dialog
        else if (trackNew.visible === true) {
            console.log(qmlLog, "trackNew visible")
            return true
        }*/
        else if (fieldNew.visible === true) {
            console.log(qmlLog, "FieldNew visible")
            return true
        }
        //if (fieldFromKML.visible) return false
        else if (fieldOpen.visible === true) return true
        //if (contextFlag.visible == true) return false
        else return false
    }

    Interfaces.RecordedPathInterface {
        id: recordedPathInterface
        objectName: "recordedPathInterface"
    }

    Comp.TimedMessage {
        //This is a popup message that dismisses itself after a timeout
        id: timedMessage
        objectName: "timedMessage"

        //Connect Backend.timedMessage to this object
        Connections {
            target: Backend

            function onTimedMessage(timeout:int , title: string, message: string) {
                timedMessage.addMessage(timeout, title, message);
            }
        }
    }

    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Active
    }

    MainTopPanel{
        id: topLine
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }


    // FieldViewItem content area
    FieldViewItem {
        //id: testFieldView
        //anchors.top: titleBar.bottom
        //anchors.left: parent.left
        //anchors.right: parent.right
        //anchors.bottom: controlsRow.top
        //anchors.margins: 4

        camera {
            x: Backend.fixFrame.easting
            y: Backend.fixFrame.northing
            rotation: Camera.camFollowing ? -Utils.radians_to_deg(VehicleInterface.fixHeading) : 0
            zoom: Camera.camSetDistance
            pitch: SettingsManager.display_camPitch
            fov: 40
        }

        grid {
            color: Qt.rgba(0,0,0,0.7)
            //size: SettingsManager.window_gridSize
            visible: SettingsManager.menu_isGridOn
            thickness: 1.5
        }

        fieldSurface {
            showTexture: SettingsManager.display_isTextureOn

            // Field color: day/night from settings
            color: SettingsManager.display_isDayMode ?
                       SettingsManager.display_colorFieldDay :
                       SettingsManager.display_colorFieldNight
        }

        vehicle: VehicleInterface.vehicleProperties

        /*
        vehicle {
            color: SettingsManager.display_colorVehicle
            opacity: 0.75 //only used for arrow vehicle
            type: SettingsManager.vehicle_vehicleType
            trackWidth: SettingsManager.vehicle_trackWidth
            wheelBase: SettingsManager.vehicle_wheelbase
            drawbarLength: ! SettingsManager.tool_isToolFront ? (SettingsManager.tool_isToolRearFixed ? 0 : SettingsManager.vehicle_hitchLength) : 0;
            threePtLength: ! SettingsManager.tool_isToolFront ? (SettingsManager.tool_isToolRearFixed ? SettingsManager.vehicle_hitchLength:0 ) : 0;
            frontHitchLength: 0
            steerAngle: SimInterface.isRunning() ? SimInterface.steerAngleActual : ModuleComm.actualSteerAngleDegrees
            antennaOffset: SettingsManager.vehicle_antennaOffset
            antennaForward: SettingsManager.vehicle_antennaPivot
            markBoundary: 0 //if nonzero, draws boundary marking line to this distance
            svennArrow: true
            firstHeadingSet: true
        }
        */

        tools {
            tools: Tools.toolsProperties.tools
            /*
            tools: [
                Tool {
                    trailing: true
                    isTBTTank: true
                    hitchLength: -2.5
                    offset: 0.0
                    heading: 20
                },
                Tool {
                    trailing: true
                    hitchLength: -3
                    offset: 0.0
                    heading: -15

                    sections: [
                        SectionProperties {
                            leftPosition: -1
                            rightPosition: 0
                        },
                        SectionProperties {
                            leftPosition: 0
                            rightPosition: 1
                        }
                    ]
                }
            ]*/
            visible: true
        }

        boundaries: BoundaryInterface.properties
        /*boundaries: Boundaries {

            colorInner: "yellow"

            colorOuter: "red"

            outer: [
                BoundaryLine {
                    points: [
                        Qt.vector3d(20,20,0),
                        Qt.vector3d(20,-20,0),
                        Qt.vector3d(-20,-20,0),
                        Qt.vector3d(-20,20,0)//,
                        //Qt.vector3d(20,20,0)
                    ]
                    visible: true
                }
            ]

            inner: [
                BoundaryLine {
                    points: [
                        Qt.vector3d(10,10,0),
                        Qt.vector3d(10,-10,0),
                        Qt.vector3d(-10,-10,0),
                        Qt.vector3d(-10,10,0)//,
                    ]
                    visible: true
                }
            ]

            beingMade: [
                Qt.vector3d(15,15,0),
                Qt.vector3d(15,-15,0),
                Qt.vector3d(-15,-15,0)
            ]

            markBoundary: -2 //metres left or right from pivot
        }
        */

        tracks: TracksInterface.properties

        recordedPath: RecordedPath.properties

        layers: LayerService.layers
        // Demo coverage layer (QML-only, no LayerService connection)
        /*
        layers: Layers {
            id: demoLayers
            visible: true

            Component.onCompleted: {
                // Create demo layer
                addLayer(0, "Demo Layer")

                // Add demo triangles within 50m radius of origin
                // Create a grid of colored patches
                var colors = [
                    Qt.rgba(1, 0, 0, 0.6),    // Red
                    Qt.rgba(0, 1, 0, 0.6),    // Green
                    Qt.rgba(0, 0, 1, 0.6),    // Blue
                    Qt.rgba(1, 1, 0, 0.6),    // Yellow
                    Qt.rgba(1, 0, 1, 0.6),    // Magenta
                    Qt.rgba(0, 1, 1, 0.6)     // Cyan
                ]

                var patchSize = 8  // metres per patch
                var colorIndex = 0

                // Create patches from -40 to +40 metres
                for (var x = -40; x < 40; x += patchSize) {
                    for (var y = -40; y < 40; y += patchSize) {
                        // Check if within 50m radius
                        var cx = x + patchSize / 2
                        var cy = y + patchSize / 2
                        if (Math.sqrt(cx*cx + cy*cy) > 45) continue

                        var color = colors[colorIndex % colors.length]
                        colorIndex++

                        // Two triangles forming a quad
                        // Triangle 1: bottom-left, bottom-right, top-left
                        addTriangle(0,
                            Qt.vector3d(x, y, 0),
                            Qt.vector3d(x + patchSize, y, 0),
                            Qt.vector3d(x, y + patchSize, 0),
                            color)

                        // Triangle 2: top-left, bottom-right, top-right
                        addTriangle(0,
                            Qt.vector3d(x, y + patchSize, 0),
                            Qt.vector3d(x + patchSize, y, 0),
                            Qt.vector3d(x + patchSize, y + patchSize, 0),
                            color)
                    }
                }

                console.log("Demo layer created with", triangleCount(0), "triangles")
            }
        }*/

        // Visibility settings
        showCoverage: true
        showGuidance: true

        // Field surface texture mode from settings

        // Background color: sky blue for day, black for night
        // Day: rgb(69, 102, 179) = rgba(0.27, 0.4, 0.7, 1)
        // Night: black
        backgroundColor: SettingsManager.display_isDayMode ?
                             Qt.rgba(0.27, 0.4, 0.7, 1) :
                             Qt.rgba(0, 0, 0, 1)

        // Vehicle color from settings

        // Boundary and guidance colors (yellow and green)
        guidanceColor: Qt.rgba(0, 1, 0, 1)
        id: glcontrolrect
        objectName: "openglcontrol"

        anchors.top: topLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        //z: -1

        Component.onCompleted: {
            //Let the backend have access to this item to
            //set properties and render callbacks
            Backend.aogRenderer = glcontrolrect;
        }

        //for moving the center of the view around
        // ✅ PHASE 6.3.0: shiftX/shiftY are now Q_PROPERTY in C++ AOGRendererInSG class
        // shiftX: 0 //-1 left to 1 right (default value set in C++)
        // shiftY: 0 //-1 down to 1 up (default value set in C++)

        signal clicked(var mouse)
        signal dragged(int fromX, int fromY, int toX, int toY)
        signal zoomOut()
        signal zoomIn()

        MouseArea {
            id: mainMouseArea
            anchors.fill: parent

            property int fromX: 0
            property int fromY: 0
            property Matrix4x4 clickModelView
            property Matrix4x4 clickProjection
            property Matrix4x4 panModelView
            property Matrix4x4 panProjection

            onClicked: function(mouse) {
                parent.clicked(mouse)
            }

            onPressed: if(panButton.checked){
                           //save a copy of the coordinates
                           fromX = mouseX
                           fromY = mouseY
                       }

            onPositionChanged: if(panButton.checked){
                                   parent.dragged(fromX, fromY, mouseX, mouseY)
                                   fromX = mouseX
                                   fromY = mouseY
                               }

            onWheel:(wheel)=>{
                        if (wheel.angleDelta.y > 0) {
                            Backend.zoomIn() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                        } else if (wheel.angleDelta.y <0 ) {
                            Backend.zoomOut() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                        }
                    }

            Image {
                id: reverseArrow
                x: VehicleInterface.screenCoord.x - 150
                y: VehicleInterface.screenCoord.y - height
                width: 70 * theme.scaleWidth
                height: 70 * theme.scaleHeight
                source: prefix + "/images/Images/z_ReverseArrow.png"
                visible: VehicleInterface.isReverse || VehicleInterface.isChangingDirection
            }
            MouseArea{
                //button that catches any clicks on the vehicle in the GL Display
                id: resetDirection
                onClicked: {
                    //isn't this redundant with the onPressed?
                    Backend.resetDirection()
                    console.log(qmlLog, "reset direction clicked")
                }
                propagateComposedEvents: true
                x: VehicleInterface.screenBounding.x
                y: VehicleInterface.screenBounding.y
                width: VehicleInterface.screenBounding.width
                height: VehicleInterface.screenBounding.height
                onPressed: (mouse)=>{
                               Backend.resetDirection()
                               console.log(qmlLog, "reset direction pressed")
                               mouse.accepted = false

                           }
            }
            //            Rectangle{
            //              // to show the reset vehicle direction button for testing purposes
            //                color: "blue"
            //                anchors.fill: resetDirection
            //            }

        } // MouseArea

    }

    Rectangle{
        id: noGPS
        anchors.fill: glcontrolrect
        color: "#0d0d0d"
        visible: Backend.fixFrame.sentenceCounter> 29
        onVisibleChanged: if(visible){
                              console.log(qmlLog, "no gps now visible")
                          }

        Image {
            id: noGPSImage
            source: prefix + "/images/Images/z_NoGPS.png"
            anchors.centerIn: parent
            anchors.margins: 200
            visible: noGPS.visible
            height: parent.height /2
            width: height
        }
    }

    Item {//item to hold all the main window buttons. Fills all of main screen

        id: buttonsArea
        anchors.top: parent.top
        anchors.topMargin: 2 //TODO: convert to scalable //do we need?
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        MainLeftColumn {
            id: leftColumn
            anchors.top: parent.top
            anchors.topMargin: topLine.height + 6
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.bottomMargin: 6
        }

        Speedometer {
            anchors.top: parent.top
            anchors.right: rightColumn.left
            anchors.topMargin: topLine.height + 10
            anchors.margins: 10
            visible: SettingsManager.menu_isSpeedoOn

            speed: Utils.speed_to_unit(VehicleInterface.avgSpeed)
        }

        SteerCircle { //the IMU indicator on the bottom right -- Called the "SteerCircle" in AOG
            anchors.bottom: bottomButtons.top
            anchors.right: rightColumn.left
            anchors.margins: 10
            visible: true
            rollAngle: Backend.fixFrame.imuRollDegrees
            steerColor: (ModuleComm.steerModuleConnectedCounter > 30 ?
                             "#f0f218f0" :
                             (ModuleComm.steerSwitchHigh === true ?
                                  "#faf80007" :
                                  (MainWindowState.isBtnAutoSteerOn === true ?
                                       "#f80df807" : "#f0f2c007")))

        }

        MainRightColumn{
            id: rightColumn
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: topLine.height + 6
            anchors.rightMargin: 6
            anchors.bottomMargin: 6
        }


        Comp.IconButton {
            id: btnContourPriority
            anchors.top: parent.top
            anchors.topMargin: theme.buttonSize + 3
            anchors.right: rightColumn.left
            anchors.rightMargin: 3
            checkable: true
            isChecked: true
            visible: false
            icon.source: prefix + "/images/ContourPriorityLeft.png"
            iconChecked: prefix + "/images/ContourPriorityRight.png"
            onClicked: Backend.contourPriority(checked) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
        }

        MainBottomRow{
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: leftColumn.width + 10
            anchors.right: parent.right
            anchors.rightMargin: rightColumn.width + 10
            anchors.bottomMargin: 6

        }

        //----------------inside buttons-----------------------
        Item{
            //plan to move everything on top of the aogRenderer that isn't
            //in one of the buttons columns
            id: inner
            anchors.left: leftColumn.right
            anchors.top: parent.top
            anchors.topMargin: topLine.height
            anchors.right: rightColumn.left
            anchors.bottom: bottomButtons.top
            visible: !noGPS.visible
            Comp.IconButtonTransparent{ //button to pan around main GL
                id: panButton
                implicitWidth: 50
                implicitHeight: 50 * theme.scaleHeight
                checkable: true
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 30
                icon.source: prefix + "/images/Pan.png"
                iconChecked: prefix + "/images/SwitchOff.png"
                onClicked: {
                    if (!checked) {
                        Backend.centerOgl()
                    }

                }
            }
            Image{
                id: hydLiftIndicator
                property bool isDown: VehicleInterface.hydLiftDown
                visible: false
                source: prefix + "/images/Images/z_Lift.png"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 80 * theme.scaleWidth
                height: 130 * theme.scaleHeight
                onIsDownChanged: {
                    if(!isDown){
                        hydLiftIndicatorColor.colorizationColor = "#00F200"
                        hydLiftIndicatorColor.rotation = 0
                    }else{
                        hydLiftIndicatorColor.rotation = 180
                        hydLiftIndicatorColor.colorizationColor = "#F26600"
                    }
                }
            }
            MultiEffect{
                id: hydLiftIndicatorColor
                anchors.fill: hydLiftIndicator
                visible: bottomButtons.hydLiftIsOn
                colorizationColor:"#F26600"
                colorization: 1.0
                source: hydLiftIndicator
            }

            Comp.OutlineText{
                id: simulatorOnText
                visible: SettingsManager.menu_isSimulatorOn
                anchors.top: parent.top
                anchors.topMargin: lightbar.height+ 10
                anchors.horizontalCenter: lightbar.horizontalCenter
                font.pixelSize: 30
                color: "#cc5200"
                text: qsTr("Simulator On")
            }

            Comp.OutlineText{
                id: ageAlarm //Lost RTK count up display
                property int age: Backend.fixFrame.age
                visible: SettingsManager.gps_isRTK
                anchors.top: simulatorOnText.bottom
                anchors.topMargin: 30
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Lost RTK")
                font.pixelSize: 65
                color: "#cc5200"
                onAgeChanged: {
                    if (age < 20)
                        text = ""
                    else if (age> 20 && age < 60)
                        text = qsTr("Age: ")+age
                    else
                        text = "Lost RTK"
                }
                onTextChanged: if (ageAlarm.text.length > 0)
                                   console.log(qmlLog, "rtk alarm sound")

            }

            AutoUturnBtn{
                id: autoTurn // where the auto turn button and distance to turn are held
                anchors.top:gridTurnBtns.top
                anchors.right: parent.right
                anchors.rightMargin: 200
                width: 100 * theme.scaleWidth
                height: 100 * theme.scaleHeight
            }

            ManualTurnBtns{
                id: gridTurnBtns
                anchors.top: lightbar.bottom
                anchors.left: parent.left
                anchors.topMargin: 30
                anchors.leftMargin: 150
            }

            LightBar {
                id: lightbar
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 5
                dotDistance: VehicleInterface.avgPivDistance / 10 //avgPivotDistance is averaged
                visible: (VehicleInterface.guidanceLineDistanceOff !== 32000 &&
                          SettingsManager.menu_isLightBarOn) ?
                             true : false
            }

            TrackNum {
                id: tracknum
                anchors.top: lightbar.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 5

                font.pixelSize: 24

                //only use dir names for AB Lines with heading
                useDirNames: (aogInterface.currentABLine > -1)
                currentTrack: TracksInterface.idx

                trackHeading: aogInterface.currentABLine > -1 ?
                                  aogInterface.currentABLine_heading :
                                  0

                visible: (SettingsManager.display_topTrackNum &&
                          ((aogInterface.currentABLine > -1) ||
                           (aogInterface.currentABCurve > -1)))
                //TODO add contour
            }

            TramIndicators{
                id: tramLeft
                anchors.top: tracknum.bottom
                anchors.margins: 30
                anchors.left: parent.horizontalCenter
                visible: SettingsManager.feature_isTramOn
            }
            TramIndicators{
                id: tramRight
                anchors.top: tracknum.bottom
                anchors.margins: 30
                anchors.right: parent.horizontalCenter
                visible: SettingsManager.feature_isTramOn
            }

            //Components- this is where the windows that get displayed over the
            //ogl get instantiated.
            Field.FieldData{ //window that displays field acreage and such
                id: fieldData
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: false
            }
            GPSData{ //window that displays GPS data
                id: gpsData
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: false
            }
            BlockageData{ //window that displays GPS data
                id: blockageData
                z: 2
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: AgIOService.blockageConnected && SettingsManager.seed_blockageIsOn
                MouseArea {
                    id: blockageViewSwitch
                    anchors.fill: parent
                    onClicked: blockageRows.viewSwitch = !blockageRows.viewSwitch
                    }
            }
            RateData{ //window that displays Machine data
                id: rateData
                visible: false
            }

            SimController{
                id: simBarRect
                //z: 2
                anchors.bottom: timeText.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 8
                visible: SettingsManager.menu_isSimulatorOn
                height: 60 * theme.scaleHeight
                onHeightChanged: anchors.bottomMargin = (8 * theme.scaleHeight)
            }
            RecPath{// recorded path menu
                id: recPath
                visible: false
            }

            Comp.OutlineText{ //displays time on bottom right of GL
                id: timeText
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: (50 * theme.scaleWidth)
                font.pixelSize: 20
                color: "#cc5200"
                property string currentTime: "HH:mm:ss"
                text: currentTime

                Timer{
                    id: timer
                    interval: 1000
                    repeat: true
                    running: Backend.fixFrame.rawHz>10?
                    onTriggered: timeText.text = Qt.formatTime(new Date(), "HH:mm:ss")
                }

                // Connections {
                //     target: timer
                //     onTriggered: {
                //         if (timeText.visible) {
                //             timeText.currentTime = Qt.formatTime(new Date(), "HH:mm:ss")
                //         }
                //     }
                // }

            }
            Comp.ToolsSectionButtons {
                id: sectionButtons
                visible: Backend.isJobStarted ? true : false
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: simBarRect.top
                anchors.bottomMargin: 8
                height: 40 * theme.scaleHeight
                width: 660  * theme.scaleWidth
                toolsModel: Tools.toolsWithSectionsModel
                //onHeightChanged: anchors.bottomMargin = (bottomButtons.height + simBarRect.height + (24 * theme.scaleHeight))
            }
            Comp.BlockageRows {
                id: blockageRows
                visible:  AgIOService.blockageConnected && SettingsManager.seed_blockageIsOn
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                height: 120 * theme.scaleHeight
                width: 800  * theme.scaleWidth
            }
            DisplayButtons{ // window that shows the buttons to change display. Rotate up/down, day/night, zoom in/out etc. See DisplayButtons.qml
                id: displayButtons
                width: childrenRect.width + 10
                height: childrenRect.height + 10
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 20
                visible: false
                z:1
            }

            Tracks.TrackButtons{
                id: trackButtons
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 20
                visible: false
                z: 100
            }
            Comp.IconButtonTransparent{ //button on bottom left to show/hide the bottom and right buttons
                id: toggleButtons
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 25
                visible: Backend.isJobStarted
                width: 45 * theme.scaleWidth
                height: 25 * theme.scaleHeight
                icon.source: prefix + "/images/MenuHideShow.png"
                onClicked: if(leftColumn.visible){
                               leftColumn.visible = false
                           }else{
                               leftColumn.visible = true
                           }
            }
            Compass{
                id: compass
                anchors.top: parent.top
                anchors.right: zoomBtns.left
                heading: -Utils.radians_to_deg(VehicleInterface.fixHeading)
                visible: SettingsManager.menu_isCompassOn
            }
            Column{
                id: zoomBtns
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 100
                width: children.width
                Comp.IconButton{
                    implicitWidth: 30 * theme.scaleWidth
                    implicitHeight: 30 * theme.scaleHeight
                    radius: 0
                    icon.source: prefix + "/images/ZoomIn48.png"
                    onClicked: Backend.zoomIn() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                }
                Comp.IconButton{
                    implicitWidth: 30 * theme.scaleWidth
                    implicitHeight: 30 * theme.scaleHeight
                    radius: 0
                    icon.source: prefix + "/images/ZoomOut48.png"
                    onClicked: Backend.zoomOut() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
                }
            }
        }



        Comp.SliderCustomized { //quick dirty hack--the up and down buttons change this value, so the speed changes
            id: speedSlider
            //anchors.bottom: bottomButtons.top
            //            anchors.bottomMargin: 3
            //            anchors.left:bottomButtons.left
            //            anchors.leftMargin: 3
            from: -80
            to: 300
            value: 0
            visible: false
        }

        StartUp{ //splash we show on startup
            id: startUp
            z:10
            //visible: true
            visible: false  //no reason to look at this until release
        }


        Field.FieldToolsMenu {
            id: fieldTools
            visible: false
        }
        Field.FieldMenu {
            id: fieldMenu
            objectName: "slideoutMenu"
            visible: false
        }
        ToolsWindow {
            id: toolsMenu
            visible: false
        }
        HamburgerMenu{ // window behind top left on main GL
            id: hamburgerMenu
            visible: false
        }

        ConfigSettings.Config {
            id:config
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            visible:false

            onAccepted: {
                console.debug("accepting settings and closing window.")
                aog.settings_save()
                aog.settingsReload() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            onRejected: {
                console.debug("rejecing all settings changes.")
                aog.settings_revert()
                aog.settingsReload() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }

        }
        HeadlandDesigner{
            id: headlandDesigner
            objectName: "headlandDesigner"
            //anchors.horizontalCenter: parent.horizontalCenter
            //anchors.verticalCenter: parent.verticalCenter
            visible: false
        }
        HeadAcheDesigner{
            id: headacheDesigner
            objectName: "headacheDesigner"
            //anchors.horizontalCenter: parent.horizontalCenter
            //anchors.verticalCenter: parent.verticalCenter
            visible: false
        }
        SteerConfig.SteerConfigWindow {
            id:steerConfigWindow
            visible: false
        }
        SteerConfig.SteerConfigSettings{
            id: steerConfigSettings
            visible: false
        }
        TramLinesEditor{
            id: tramLinesEditor
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 150
            anchors.topMargin: 50
            visible: false
        }
        LineEditor{
            id: lineEditor
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 150
            anchors.topMargin: 50
            visible: false
        }
        Boundary.BoundaryMenu{
            id: boundaryMenu
            visible: false
        }

        Tracks.LineDrawer {//window where lines are created off field boundary/edited
            id:lineDrawer
            //anchors.horizontalCenter: parent.horizontalCenter
            //anchors.bottom: parent.bottom
            //height: 768
            //width:1024
            visible:false
        }
        Tracks.LineNudge{
            id: lineNudge
            visible: false
        }
        Tracks.RefNudge{
            id: refNudge
            visible: false
        }
        SetSimCoords{
            id: setSimCoords
            anchors.fill: parent
        }
        HotKeys{
            id: hotKeySettings
        }
        ConfigSettings.SetColors{
            id: setColors
            //anchors.fill: parent
        }
        ConfigSettings.SectionColors{
            id: sectionColors
            //anchors.fill: parent
        }
        Tracks.TrackNewButtons{
            id: trackNewButtons
            visible: false
        }
        Tracks.TrackNewSet{
            id: trackNewSet
            anchors.fill: parent
        }

        Tracks.TrackList{
            id: trackList
        }

        Tracks.TracksNewAddName{
            id: trackAddName
        }
        Wiz.ChartSteer{
            id: steerCharta
            height: 300  * theme.scaleHeight
            width: 400  * theme.scaleWidth
            xval1: ModuleComm.actualSteerAngleDegrees
            xval2: VehicleInterface.driveFreeSteerAngle
            axismin: -10
            axismax: 10
            lineName1:"Actual " + Math.round(ModuleComm.actualSteerAngleDegrees *10) / 10
            lineName2: "SetPoint " + Math.round(VehicleInterface.driveFreeSteerAngle *10) / 10
            chartName: qsTr("Steer Chart")
            visible: false
            function show(){
                steerCharta.visible = true
            }
        }

        Wiz.ChartSteer{
            id: xteCharta
            height: 300  * theme.scaleHeight
            width: 400  * theme.scaleWidth
            xval1: VehicleInterface.modeActualXTE
            xval2: Number(aog.dataSteerAngl)
            axismin: -100
            axismax: 100
            lineName1:"XTE " + Math.round(VehicleInterface.modeActualXTE *10) / 10
            lineName2:"HE " + Math.round(Number(aog.dataSteerAngl) *10) / 10
            chartName: qsTr("XTE Chart")
            visible: false
            function show(){
                xteCharta.visible = true
            }
        }

        Wiz.ChartSteer{
            id: headingCharta
            height: 300  * theme.scaleHeight
            width: 400  * theme.scaleWidth
            xval1: Backend.fixFrame.heading  // Rectangle Pattern: direct property access
            xval2: Backend.fixFrame.imuHeading > 360 ? 0 : Backend.fixFrame.imuHeading  // Show real IMU heading, 0 if invalid
            axismin: -10
            axismax: 360
            lineName1:"Fix2fix "+Math.round(Backend.fixFrame.heading *10) / 10
            lineName2:"IMU "+ Math.round((Backend.fixFrame.imuHeading > 360 ? 0 : Backend.fixFrame.imuHeading) *10) / 10
            chartName: qsTr("Heading Chart")
            visible: false
            function show(){
                headingCharta.visible = true
            }
        }

        // Wiz.Camera{
        //     id: cam1
        //     height: 300  * theme.scaleHeight
        //     width: 400  * theme.scaleWidth
        // }
        Wiz.WasWizard{
            id: wasWizard
            height: 300  * theme.scaleHeight
            width: 400  * theme.scaleWidth
            visible: false
            function show(){
                wasWizard.visible = true
            }
        }

        Rectangle{//show "Are you sure?" when close button clicked
            id: closeDialog
            width: 500 * theme.scaleWidth
            height: 100 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: aogInterface.backgroundColor
            border.color: aogInterface.blackDayWhiteNight
            border.width: 2
            visible: false
            Comp.IconButtonText{
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                color1: "transparent"
                color2: "transparent"
                color3: "transparent"
                icon.source: prefix + "/images/back-button.png"
                onClicked: parent.visible = false
            }
            Comp.IconButtonText{
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                color1: "transparent"
                color2: "transparent"
                color3: "transparent"
                icon.source: prefix + "/images/ExitAOG.png"
                onClicked: {
                    Backend.applicationClosing = true  // Save vehicle when exiting app (Qt 6.8 binding)
                    Qt.quit()
                }
            }
        }
        Item{
            id: windowsArea      //container for declaring all the windows
            anchors.fill: parent //that can be displayed on the main screen

            // ===== Floating FieldViewItem Test Window =====
            Rectangle {
                id: fieldViewTestWindow
                x: 50
                y: 100
                width: 420
                height: 400
                color: "#2d2d2d"
                border.color: "#888888"
                border.width: 2
                radius: 8
                visible: false
                z: 100  // Always on top

                // Title bar for dragging
                Rectangle {
                    id: titleBar
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 30
                    color: "#444444"
                    radius: 6

                    // Square off bottom corners
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 10
                        color: parent.color
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "FieldView Test (Scene Graph)"
                        color: "#ffffff"
                        font.bold: true
                    }

                    // Close button
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 5
                        width: 20
                        height: 20
                        radius: 10
                        color: closeMouseArea.containsMouse ? "#ff4444" : "#666666"

                        Text {
                            anchors.centerIn: parent
                            text: "X"
                            color: "#ffffff"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        MouseArea {
                            id: closeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: fieldViewTestWindow.visible = false
                        }
                    }

                    // Drag handling
                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        anchors.rightMargin: 30  // Don't interfere with close button
                        property point clickPos: "0,0"

                        onPressed: function(mouse) {
                            clickPos = Qt.point(mouse.x, mouse.y)
                        }

                        onPositionChanged: function(mouse) {
                            var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
                            fieldViewTestWindow.x += delta.x
                            fieldViewTestWindow.y += delta.y
                        }
                    }
                }

                Rectangle {
                }

                // Controls column at bottom
                Column {
                    id: controlsRow
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 4
                    spacing: 2

                    // Info display row
                    Row {
                        spacing: 8
                        Text {
                            text: "Pos: " + Backend.fixFrame.easting.toFixed(1) + ", " + Backend.fixFrame.northing.toFixed(1)
                            color: "#aaaaaa"
                            font.pixelSize: 10
                        }
                        Text {
                            text: "Hdg: " + Utils.radians_to_deg(VehicleInterface.fixHeading).toFixed(1) + "°"
                            color: "#aaaaaa"
                            font.pixelSize: 10
                        }
                    }

                    // WorldGrid extents row
                    Row {
                        spacing: 8
                        Text {
                            text: "Grid: E[" + WorldGrid.eastingMin.toFixed(0) + "," + WorldGrid.eastingMax.toFixed(0) +
                                  "] N[" + WorldGrid.northingMin.toFixed(0) + "," + WorldGrid.northingMax.toFixed(0) + "]"
                            color: "#888888"
                            font.pixelSize: 9
                        }
                    }

                    // Controls row
                    Row {
                        spacing: 4

                        // Zoom controls - use Camera singleton
                        Rectangle {
                            width: 25
                            height: 25
                            color: "#555555"
                            radius: 4
                            Text { anchors.centerIn: parent; text: "-"; color: "white"; font.pixelSize: 16 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Backend.zoomOut()
                            }
                        }

                        Text {
                            height: 25
                            verticalAlignment: Text.AlignVCenter
                            text: "Zoom: " + Math.abs(Camera.camSetDistance).toFixed(0)
                            color: "#cccccc"
                            font.pixelSize: 11
                        }

                        Rectangle {
                            width: 25
                            height: 25
                            color: "#555555"
                            radius: 4
                            Text { anchors.centerIn: parent; text: "+"; color: "white"; font.pixelSize: 16 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Backend.zoomIn()
                            }
                        }

                        // Spacer
                        Item { width: 6; height: 1 }

                        // Pitch controls - use Backend tilt methods
                        Rectangle {
                            width: 25
                            height: 25
                            color: "#555555"
                            radius: 4
                            Text { anchors.centerIn: parent; text: "^"; color: "white"; font.pixelSize: 14 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Backend.tiltUp()
                            }
                        }

                        Text {
                            height: 25
                            verticalAlignment: Text.AlignVCenter
                            text: "Tilt: " + SettingsManager.display_camPitch.toFixed(0)
                            color: "#cccccc"
                            font.pixelSize: 11
                        }

                        Rectangle {
                            width: 25
                            height: 25
                            color: "#555555"
                            radius: 4
                            Text { anchors.centerIn: parent; text: "v"; color: "white"; font.pixelSize: 14 }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Backend.tiltDown()
                            }
                        }

                        // Spacer
                        Item { width: 6; height: 1 }

                        // Following mode toggle
                        Rectangle {
                            width: 50
                            height: 25
                            color: Camera.camFollowing ? "#446644" : "#555555"
                            radius: 4
                            Text {
                                anchors.centerIn: parent
                                text: Camera.camFollowing ? "Follow" : "North"
                                color: "white"
                                font.pixelSize: 10
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Camera.set_camFollowing(!Camera.camFollowing)
                            }
                        }
                    }
                }

                // Resize handle (bottom-right corner)
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: 15
                    height: 15
                    color: "transparent"

                    // Diagonal lines to indicate resize
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.strokeStyle = "#888888"
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            ctx.moveTo(width - 3, height)
                            ctx.lineTo(width, height - 3)
                            ctx.moveTo(width - 7, height)
                            ctx.lineTo(width, height - 7)
                            ctx.moveTo(width - 11, height)
                            ctx.lineTo(width, height - 11)
                            ctx.stroke()
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        property point clickPos: "0,0"

                        onPressed: function(mouse) {
                            clickPos = Qt.point(mouse.x, mouse.y)
                        }

                        onPositionChanged: function(mouse) {
                            var deltaX = mouse.x - clickPos.x
                            var deltaY = mouse.y - clickPos.y
                            var newWidth = Math.max(300, fieldViewTestWindow.width + deltaX)
                            var newHeight = Math.max(250, fieldViewTestWindow.height + deltaY)
                            fieldViewTestWindow.width = newWidth
                            fieldViewTestWindow.height = newHeight
                        }
                    }
                }
            }

            Field.FieldFromExisting{
                id: fieldFromExisting
                x: 0
                y: 0
            }
            Field.FieldNew{
                id: fieldNew
            }
            Field.FieldFromKML{
                id: fieldFromKML
                x: 100
                y: 75
            }
            Field.FieldOpen{
                id: fieldOpen
                x: 100
            }
            Field.Flags{
                id: flags
            }
            Field.FlagLatLon{
                id: flagLatLon
            }

            y: 75
        }
    }


    Rectangle {
        id: contextFlag
        objectName: "contextFlag"
        width: Math.max(50, childrenRect.width + 10)
        height: Math.max(50, childrenRect.height + 10)
        color: "#bf163814"
        visible: false
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: theme.buttonSize + 10
        //anchors.topMargin: btnFlag.y
        border.width: 2
        border.color: "#c3ecc0"
        property string icon: "/images/FlagRed.png";
        property double ptlat: 0
        property double ptlon: 0
        property double ptId: 0
        property string ptText: ""


        Grid {
            id: contextFlagGrid
            spacing: 5
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 5

            width: childrenRect.width
            height: childrenRect.height

            columns: 5
            flow: Grid.LeftToRight

            Comp.IconButton {
                id: redFlag
                objectName: "btnRedFlag"
                icon.source: prefix + "/images/FlagRed.png";
                onClicked: aog.redFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            Comp.IconButton {
                id: greenFlag
                objectName: "btnGreenFlag"
                icon.source: prefix + "/images/FlagGrn.png";
                onClicked: aog.greenFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            Comp.IconButton {
                id: yellowFlag
                objectName: "btnYellowFlag"
                icon.source: prefix + "/images/FlagYel.png";
                onClicked: aog.yellowFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            Comp.IconButton {
                id: deleteFlag
                objectName: "btnDeleteFlag"
                icon.source: prefix + "/images/FlagDelete.png"
                //enabled: false
                onClicked: aog.deleteFlag() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
            Comp.IconButton {
                id: deleteAllFlags
                objectName: "btnDeleteAllFlags"
                icon.source: prefix + "/images/FlagDeleteAll.png"
                //enabled: false
                onClicked: aog.deleteAllFlags() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            }
        }
        /********************************dialogs***********************/
        ColorDialog{//color picker
            id: cpSectionColor
            onSelectedColorChanged: {

                //just use the Day setting. AOG has them locked to the same color anyways
                SettingsManager.display_colorSectionsDay = cpSectionColor.selectedColor;

                //change the color on the fly. In AOG, we had to cycle the sections off
                //and back on. This does for us.
                if(btnSectionManual){
                    btnSectionManual.clicked()
                    btnSectionManual.clicked()
                }else if(btnSectionAuto){
                    btnSectionAuto.clicked()
                    btnSectionAuto.clicked()
                }
            }
        }

        CloseAOG{
            id: closeAOG
        }
    }


    AgIOModule.AgIO {
          id: mainWindowAgIO
    }
    AgIOModule.PortMenu{
        id: gnssConfig
        visible: false
        portBaud: SettingsManager.gnss_BaudRate
        portName: SettingsManager.gnss_SerialPort
        moduleType: "Gnss"
    }
    AgIOModule.EthernetConfig {
        id: ethernetConfig
        visible: false
        onVisibleChanged: {
            if(visible)
                ethernetConfig.load_settings()
        }
    }
    AgIOModule.GPSInfo {
        id: gpsInfo
        visible: false
    }
    AgIOModule.NTrip{
        id: ntrip
        visible: false
    }
}

