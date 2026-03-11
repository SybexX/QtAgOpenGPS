import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import AOG
import "components" as Comp

Item {
    id: leftColumn
    width: visible ? column.implicitWidth + 2 * padding : 0
    height: parent.height
    readonly property real padding: 5 * theme.scaleWidth

    onHeightChanged: {

        if (column.children.length > 0) {
            theme.btnSizes[2] = height / (column.children.length)
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


        Button {
            id: btnAcres
            implicitWidth: theme.buttonSize
            implicitHeight: theme.buttonSize
            Layout.alignment: Qt.AlignCenter
            onClicked: {
                Backend.currentFieldSetDistanceUser(0)
                Backend.currentFieldSetWorkedAreaTotalUser(0)
            }

            background: Rectangle{
                anchors.fill: parent
                color: aogInterface.backgroundColor
                radius: 10
                Text{
                    anchors.top: parent.top
                    anchors.bottom: parent.verticalCenter
                    anchors.margins: 5
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Utils.m_to_unit_string(Backend.currentField.distanceUser, 2)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: parent.height * .33
                }
                Text{
                    anchors.top: parent.verticalCenter
                    anchors.bottom: parent.bottom
                    anchors.margins: 5
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Utils.area_to_unit_string(Backend.currentField.workedAreaTotalUser, 2)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: parent.height * .33
                }
            }
        }

        Comp.MainWindowBtns {
            id: btnnavigationSettings
            buttonText: qsTr("Display")
            icon.source: prefix + "/images/NavigationSettings.png"
            onClicked: displayButtons.visible = !displayButtons.visible
        }

        Comp.MainWindowBtns {
            id: btnSettings
            buttonText: qsTr("Settings")
            icon.source: prefix + "/images/Settings48.png"
            onClicked: config.open()
        }

        Comp.MainWindowBtns {
            id: btnTools
            buttonText: qsTr("Tools")
            icon.source: prefix + "/images/SpecialFunctions.png"
            onClicked: toolsMenu.visible = true
        }

        Comp.MainWindowBtns{
            id: btnFieldMenu
            buttonText: qsTr("Field")
            icon.source: prefix + "/images/JobActive.png"
            onClicked: fieldMenu.visible = true
        }

        Comp.MainWindowBtns{
            id: btnFieldTools
            buttonText: qsTr("Field Tools")
            icon.source: prefix + "/images/FieldTools.png"
            onClicked: fieldTools.visible = true
            enabled: Backend.isJobStarted ? true : false
        }

        Comp.MainWindowBtns {
            id: btnAgIO
            buttonText: qsTr("AgIO")
            icon.source: prefix + "/images/AgIO.png"
            onClicked: {
                mainWindowAgIO.visible = !mainWindowAgIO.visible
            }
        }

        Comp.MainWindowBtns {
            id: btnautoSteerConf
            buttonText: qsTr("Steer config")
            icon.source: prefix + "/images/AutoSteerConf.png"
            onClicked: {
                steerConfigWindow.visible = true
                steerConfigWindow.show()
            }
        }
    }
}
