import QtQuick
import QtQuick.Layouts
import AOG

// Interface import removed - now QML_SINGLETON
import "components" as Comp

    Rectangle{
        id: topLine
        color: SettingsManager.display_isDayMode?SettingsManager.display_colorDayFrame:SettingsManager.display_colorNightFrame
        //opacity: 0.5
        height: 50 *theme.scaleHeight
        visible: true

        Comp.IconButtonTransparent {
            id: btnfileMenu
            height: parent.height
            width: 75 * theme.scaleWidth
            icon.source: prefix + "/images/fileMenu.png"
            onClicked: hamburgerMenu.visible = true
        }

        Text{
            anchors.top:parent.top
            anchors.left: parent.left
            anchors.leftMargin: leftColumn.width+20
            text: (Backend.fixFrame.fixQuality === 0 ? "Invalid":
                   Backend.fixFrame.fixQuality ===1 ? "GPS Single":
                   Backend.fixFrame.fixQuality ===2 ? "DGPS":
                   Backend.fixFrame.fixQuality ===3 ? "PPS":
                   Backend.fixFrame.fixQuality ===4 ? "RTK Fix":
                   Backend.fixFrame.fixQuality ===5 ? "RTK Float":
                   Backend.fixFrame.fixQuality ===6 ? "Estimate":
                   Backend.fixFrame.fixQuality ===7 ? "Man IP":
                   Backend.fixFrame.fixQuality ===8 ? "Sim":
                   "Invalid") + ": Age: "+ Math.round(Backend.fixFrame.age * 10)/ 10

            font.pixelSize: 20
            anchors.bottom: parent.verticalCenter
        }

        //        Text {
        //            anchors.top: parent.top
        //            anchors.left: parent.left
        //            anchors.leftMargin: 120
        //            text: qsTr("Field: "+ (Backend.isJobStarted ? Settings.f_currentDir: "None"))
        //            anchors.bottom: parent.verticalCenter
        //            font.bold: true
        //            font.pixelSize: 15
        //        }
        Text {
            id: playText
            property string mainString: ""
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 100
            text: (playTimer.running ? "■ " : "▶ ") + mainString
            anchors.top: parent.verticalCenter
            font.bold: true
            font.pixelSize: 15
        }
        MouseArea{
            anchors.top:playText.top
            anchors.left: playText.left
            height: playText.height
            width: height
            onClicked: playTimer.running = !playTimer.running
        }

        Timer{
            id: playTimer
            property int increment: -1
            /* increment:
              0: Time + Date
              1: Lat + Lon
              2: Vehicle
              3: Field
              4: App
              Else: Line*/
            running: true
            interval: 2000
            repeat: true
            onTriggered: {
                playTimer.restart
                increment++
                if(increment == 0){
                    playText.mainString = Qt.formatDateTime(new Date(), "MM-dd-yyyy HH:mm:ss")
                }else if(increment == 1){
                    playText.mainString = qsTr("Lat: %1 Lon: %2")
                            .arg(Qt.locale().toString(Backend.fixFrame.latitude,'f',7))
                            .arg(Qt.locale().toString(Backend.fixFrame.longitude,'f',7))
                }else if(increment == 2){
                    // Threading Phase 1: Vehicle display information
                    playText.mainString = Utils.m_to_ft_string(SettingsManager.vehicle_toolWidth) + " - " + SettingsManager.vehicle_vehicleName
                    if(!Backend.isJobStarted) //reset
                        increment = -1
                }else if(increment == 3){
                    // Threading Phase 1: Current field directory
                    playText.mainString = qsTr("Field: %1").arg(SettingsManager.f_currentDir)
                }else if(increment == 4) {
                    var percentLeft = ""
                    if (Backend.currentField.areaBoundaryOuterLessInner > 0) {
                        percentLeft = qsTr("%1%").arg(Qt.locale().toString((Backend.currentField.areaBoundaryOuterLessInner - Backend.currentField.workedAreaTotal) / Backend.currentField.areaBoundaryOuterLessInner * 100, 'f', 0))
                    } else {
                        percentLeft = "--"
                    }
                    playText.mainString = qsTr("App: %1 Actual: %2 %3 %4")
                            .arg(Utils.area_to_unit_string(Backend.currentField.workedAreaTotal, 2))
                            .arg(Utils.area_to_unit_string(Backend.currentField.actualAreaCovered, 2))
                            .arg(percentLeft)
                            .arg(Utils.workRateString(VehicleInterface.avgSpeed))
                }
                else {
                    if (TracksInterface.idx > -1) {
                        playText.mainString = qsTr("Track: %1").arg(TracksInterface.currentName)
                    } else {
                        playText.mainString = qsTr("Track: none active")
                    }

                    increment = -1 //reset
                }



            }
        }

        Text {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("ab")
            font.bold: true
            font.pixelSize: 15
        }
        Row{
            id: topRowWindow
            width: childrenRect.width
            height: parent.height
            anchors.top: parent.top
            anchors.right: parent.right
            spacing: 5 * theme.scaleWidth
            Comp.IconButton {
                id: btnFieldInfo
                icon.source: prefix + "/images/FieldStats.png"
                Layout.alignment: Qt.AlignCenter
                implicitWidth: theme.buttonSize
                height:parent.height
                visible: Backend.isJobStarted
                onClicked: {
                    fieldData.visible = !fieldData.visible
                    gpsData.visible = false
                    blockageData.visible = false
                }
            }
            Comp.IconButtonColor{
                id: rtkStatus
                icon.source: prefix + "/images/GPSQuality.png"
                implicitWidth: 75 * theme.scaleWidth
                implicitHeight: parent.height
                color: "yellow"
                onClicked: {
                    gpsData.visible = !gpsData.visible
                    fieldData.visible = false
                    blockageData.visible = false
                }
                Connections{
                    target: Backend
                    function onFixFrameChanged() {
                        if(Backend.fixFrame.fixQuality === 4) rtkStatus.color = "green"
                        else if(Backend.fixFrame.fixQuality === 5) rtkStatus.color = "orange"
                        else if(Backend.fixFrame.fixQuality === 2) rtkStatus.color = "yellow"
                        else rtkStatus.color = "red"
                    }
                }

            }
            Comp.IconButton {
                id: btnBlockageInfo
                icon.source: prefix + "/images/Blockage.png"
                Layout.alignment: Qt.AlignCenter
                implicitWidth: theme.buttonSize
                height:parent.height
                // Threading Phase 1: Blockage monitoring visibility
                visible: AgIOService.blockageConnected && SettingsManager.seed_blockageIsOn
                onClicked: {
                    blockageData.visible = !blockageData.visible
                    gpsData.visible = false
                    fieldData.visible = false
                }
            }
            Comp.IconButton {
                id: btnRateInfo
                icon.source: prefix + "/images/spray2.png"
                Layout.alignment: Qt.AlignCenter
                implicitWidth: theme.buttonSize
                height:parent.height
                visible: AgIOService.rateControlConnected
                onClicked: {
                    rateData.visible = !rateData.visible
                    //gpsData.visible = false
                    //fieldData.visible = false
                    //blockageData.visible = false
                }
            }

            Text{
                id: speed
                anchors.verticalCenter: parent.verticalCenter
                width: 75 * theme.scaleWidth
                height:parent.height
                text: Utils.speed_to_unit_string(VehicleInterface.avgSpeed, 1)
                font.bold: true
                font.pixelSize: 35
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Comp.IconButtonTransparent{
                height: parent.height
                icon.source: prefix + "/images/WindowMinimize.png"
                width: 75 * theme.scaleWidth
                onClicked: mainWindow.showMinimized()
            }
            Comp.IconButtonTransparent{
                id: btnMaximize
                height: parent.height
                icon.source: prefix + "/images/WindowMaximize.png"
                width: 75 * theme.scaleWidth
                onClicked: {
                    console.debug("Visibility is " + mainWindow.visibility)
                    if (mainWindow.visibility == Window.FullScreen){
                        mainWindow.showNormal()
                    }else{
                        // Threading Phase 1: Save window size before fullscreen
                        SettingsManager.window_size = ((mainWindow.width.toString() + ", "+  (mainWindow.height).toString()))
                        mainWindow.showFullScreen()
                    }
                }
            }
            Comp.IconButtonTransparent{
                height: parent.height
                width: 75 * theme.scaleWidth
                icon.source: prefix + "/images/WindowClose.png"
                onClicked: {
                    Backend.applicationClosing = true  // Save vehicle when closing window (Qt 6.8 binding)
                    mainWindow.close()
                }
            }
        }
    }
