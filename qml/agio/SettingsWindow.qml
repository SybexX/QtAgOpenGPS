import QtQuick
import QtQuick.Controls.Fusion
import "../components"
// 

Drawer{
    id: settingsWindow
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    modal: true

    contentItem: Rectangle{
        id: settingsWindowContent
        anchors.fill: parent
        height: settingsWindow.height
        color: aogInterface.blackDayWhiteNight


    ScrollViewExpandableColumn{
        id: toolsColumn
        anchors.fill: parent

        IconButtonTextBeside{
            //objectName: advancedOptions
            text: qsTr("Advanced")
            icon.source: "../images/Settings48.png"
            onClicked: {
                settingsWindow.close()
                advancedMenu.showMenu()
            }
        }

    }
}
}
