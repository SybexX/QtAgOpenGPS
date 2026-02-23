import QtQuick
import QtQuick.Layouts
//import Settings
// Interface import removed - now QML_SINGLETON
import AOG
import "components" as Comp

ColumnLayout {
    id: rightColumn //buttons

    visible: Backend.isJobStarted

    property bool statAutoSteer: false

    Connections {
        target: mainWindow
        function onHotKeyPressed(index) {
            switch (index) {
            case 8: // верх
            {   statAutoSteer = !statAutoSteer
                btnAutoSteer.clicked()
                btnAutoSteer.checked = statAutoSteer
            }
            break
            case 9: // верх
                btnAutoTrack.isChecked = !btnAutoTrack.isChecked
                break
            }
        }
    }


    onHeightChanged: {
        theme.btnSizes[0] = height / (children.length)
        theme.buttonSizesChanged()
    }
    onVisibleChanged: if(visible === false)
                          width = 0
                      else
                          width = children.width

    Comp.MainWindowBtns {
        property bool isContourLockedByUser //store if user locked
        id: btnContourLock
        isChecked: MainWindowState.btnIsContourLocked
        visible: btnContour.checked
        checkable: true
        icon.source: prefix + "/images/ColorUnlocked.png"
        iconChecked: prefix + "/images/ColorLocked.png"
        buttonText: qsTr("Lock")
        onClicked: {
            Backend.contourLock() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            if (MainWindowState.btnIsContourLocked)
                isContourLockedByUser = true
        }
        // Connections{
        //     target: aogInterface
        //     function onBtnIsContourLockedChanged() {
        //         btnContourLock.checked = MainWindowState.btnIsContourLocked
        //         if(btnContourLock.isContourLockedByUser)
        //             btnContourLock.isContourLockedByUser = false
        //     }
        //     // function onIsBtnAutoSteerOnChanged() {
        //     //     if (!btnContourLock.isContourLockedByUser && btnContour.checked === true){
        //     //         if(btnContourLock.checked !== aog.isBtnAutoSteerOn){
        //     //             Backend.contourLock() // Qt 6.8 MODERN: Direct Q_INVOKABLE call
        //     //         }
        //     //     }
        //     // }
        // }
    }
    Comp.MainWindowBtns {
        id: btnContour
        isChecked: MainWindowState.isContourBtnOn // Qt 6.8 MODERN: Q_PROPERTY access
        checkable: true
        icon.source: prefix + "/images/ContourOff.png"
        iconChecked: prefix + "/images/ContourOn.png"
        buttonText: qsTr("Contour")
        onClicked: Backend.toggleContour()
        onCheckedChanged: { //gui logic
            btnTrackCycle.visible = !checked
            btnTrackCycleBk.visible = !checked
            btnContourPriority.visible = checked
        }
    }

    Comp.IconButton{
        id: btnTrackCycle
        icon.source: prefix + "/images/ABLineCycle.png"
        Layout.alignment: Qt.AlignCenter
        implicitWidth: theme.buttonSize
        implicitHeight: theme.buttonSize

        onClicked: {
            if (TracksInterface.idx > -1) {
                TracksInterface.next()
            }
        }
    }
    Comp.IconButton{
        id: btnTrackCycleBk
        icon.source: prefix + "/images/ABLineCycleBk.png"
        Layout.alignment: Qt.AlignCenter
        implicitWidth: theme.buttonSize
        implicitHeight: theme.buttonSize

        onClicked: {
            if (TracksInterface.idx > -1) {
                TracksInterface.prev()
            }
        }

    }
    Comp.IconButton{
        id: btnAutoTrack
        checkable: true
        isChecked: TracksInterface.isAutoTrack
        icon.source: prefix + "/images/AutoTrackOff.png"
        iconChecked: prefix + "/images/AutoTrack.png"
        Layout.alignment: Qt.AlignCenter
        implicitWidth: theme.buttonSize
        implicitHeight: theme.buttonSize
        onCheckedChanged: TracksInterface.isAutoTrack = checked
    }

    Comp.MainWindowBtns {
        id: btnSectionManual
        isChecked: MainWindowState.manualBtnState === SectionState.On
        checkable: true
        icon.source: prefix + "/images/ManualOff.png"
        iconChecked: prefix + "/images/ManualOn.png"
        buttonText: qsTr("Manual")
        onCheckedChanged: {
            //if tow between we work with tool #1, otherwise tool 0
            var whichTool = SettingsManager.tool_isTBT ? 1 : 0;

            if (checked) {
                btnSectionAuto.checked = false;
                Tools.toolsProperties.setAllSectionButtonsToState(whichTool, SectionState.On);
                MainWindowState.manualBtnState = SectionState.On

            } else {
                Tools.toolsProperties.setAllSectionButtonsToState(whichTool, SectionState.Off);
                MainWindowState.manualBtnState = SectionState.Off
            }
        }
    }

    Comp.MainWindowBtns {
        id: btnSectionAuto
        isChecked: MainWindowState.autoBtnState == SectionState.Auto
        checkable: true
        icon.source: prefix + "/images/SectionMasterOff.png"
        iconChecked: prefix + "/images/SectionMasterOn.png"
        buttonText: qsTr("Auto")
        onCheckedChanged: {
            //if tow between we work with tool #1, otherwise tool 0
            var whichTool = SettingsManager.tool_isTBT ? 1 : 0;

            if (checked) {
                btnSectionManual.checked = false;
                Tools.toolsProperties.setAllSectionButtonsToState(whichTool,SectionState.Auto);
                MainWindowState.autoBtnState = SectionState.Auto
            } else {
                Tools.toolsProperties.setAllSectionButtonsToState(whichTool,SectionState.Off);
                MainWindowState.autoBtnState = SectionState.Off
            }
        }
    }

    Comp.MainWindowBtns {
        id: btnAutoYouTurn
        isChecked: MainWindowState.isYouTurnBtnOn // Qt 6.8 MODERN: Q_PROPERTY access
        checkable: true
        icon.source: prefix + "/images/YouTurnNo.png"
        iconChecked: prefix + "/images/YouTurn80.png"
        buttonText: qsTr("AutoUturn")
        visible: TracksInterface.idx > -1
        onClicked: Backend.yt.toggleAutoYouTurn()
    }


    Comp.MainWindowBtns {
        id: btnAutoSteer
        icon.source: prefix + "/images/AutoSteerOff.png"
        iconChecked: prefix + "/images/AutoSteerOn.png"
        checkable: true
        isChecked: MainWindowState.isBtnAutoSteerOn  // ⚡ PHASE 6.0.20 FIX: Use isChecked for bidirectional binding (sync with C++ protection)
        // ⚡ PHASE 6.0.20 FIX: Require ACTIVE line (not just in memory) - currentABLine/Curve check mode === AB/Curve
        enabled: ((aogInterface.currentABLine > -1 || aogInterface.currentABCurve > -1) || MainWindowState.isContourBtnOn) && Backend.isJobStarted
        //Is remote activation of autosteer enabled? //todo. Eliminated in 6.3.3
        // Threading Phase 1: Auto steer mode display
        buttonText: (SettingsManager.as_isAutoSteerAutoOn ? "R" : "M")

        onClicked: {
            // ⚡ PHASE 6.0.20 FIX: Check ACTIVE line (not just in memory)
            if (((aogInterface.currentABLine > -1 || aogInterface.currentABCurve > -1) || btnContour.isChecked) &&
                    (VehicleInterface.avgSpeed >= SettingsManager.as_minSteerSpeed &&
                     VehicleInterface.avgSpeed <= SettingsManager.as_maxSteerSpeed) ) {
                MainWindowState.isBtnAutoSteerOn = !MainWindowState.isBtnAutoSteerOn; // Qt 6.8 MODERN: Q_PROPERTY assignment
            } else {
                // No active line or contour: don't allow AutoSteer
                MainWindowState.isBtnAutoSteerOn = false; // Qt 6.8 MODERN: Q_PROPERTY assignment
            }
        }
    }
}
