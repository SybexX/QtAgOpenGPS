import QtQuick
import QtQuick.Layouts
//import Settings
// Interface import removed - now QML_SINGLETON
import AOG
import "components" as Comp

Item {
    id: rightColumn
    width: visible ? column.implicitWidth + 2 * padding : 0
    height: parent.height
    visible: Backend.isJobStarted

    readonly property real padding: 5 * theme.scaleWidth
    property bool statAutoSteer: false

    onHeightChanged: {

        if (column.children.length > 0) {
            theme.btnSizes[0] = height / column.children.length
            theme.buttonSizesChanged()
        }
        column.positionButtons();
    }

    Rectangle {
        id: backgroundRect
        x: column.x - 3
        y: column.y - 3
        width: column.width + 6
        height: column.height + 6
        color: SettingsManager.display_isDayMode?SettingsManager.display_colorDayFrame:SettingsManager.display_colorNightFrame
        opacity: 0.5
        radius: 10
        z: -1
    }

    Column {
        id: column
        spacing: 10  * theme.scaleHeight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        y: padding + 3

        function positionButtons() {
            var totalHeight = 0;
            var visibleCount = 0;
            for (var i = 0; i < children.length; i++) {
                if (children[i].visible) {
                    totalHeight += children[i].height;
                    visibleCount++;
                }
            }
            totalHeight += (visibleCount - 1) * spacing;
            anchors.verticalCenterOffset = 0;
            var availableHeight = parent.height;
            if (totalHeight < availableHeight) {
                anchors.horizontalCenterOffset = 0;
            }
        }


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
        isChecked: MainWindowState.isBtnAutoSteerOn
        enabled: (TracksInterface.idx > -1 || MainWindowState.isContourBtnOn) && Backend.isJobStarted
        //Is remote activation of autosteer enabled? //todo. Eliminated in 6.3.3
        // Threading Phase 1: Auto steer mode display
        buttonText: (SettingsManager.as_isAutoSteerAutoOn ? "R" : "M")

        onClicked: {
            if ((TracksInterface.idx > -1 || btnContour.isChecked) &&
                    (VehicleInterface.avgSpeed >= SettingsManager.as_minSteerSpeed &&
                     VehicleInterface.avgSpeed <= SettingsManager.as_maxSteerSpeed) ) {
                MainWindowState.isBtnAutoSteerOn = !MainWindowState.isBtnAutoSteerOn
            } else {
                MainWindowState.isBtnAutoSteerOn = false
            }
        }
    }
}
}
