import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// Interface import removed - now QML_SINGLETON

import ".."
import "../components"

MoveablePopup {
    id: trackAddName
    width: 200 * theme.scaleWidth
    height: 300 * theme.scaleHeight

    function show(textString) {
        trackAddName.visible = true
        trackName.text = textString
    }

    GridLayout {
        id: trackAddNameLayout
        anchors.centerIn: parent
        width: parent.width *.9
        height: parent.height *.9
        flow: Grid.LeftToRight
        rows: 4
        columns: 2
        Text {
            text: qsTr("Enter Track Name:")
            Layout.columnSpan: 2
        }
        TextField {
            id: trackName
            Layout.columnSpan: 2
            //text: trackAddName.defaultText //?
            text: "1"
        }
        IconButtonTransparent{
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignCenter
            icon.source: prefix + "/images/JobNameTime.png"
            Text{
                anchors.right: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "+"
            }
            onClicked: {
                var time = new Date().toLocaleTimeString(Qt.locale())
                trackName.text += " " + time
            }
        }

        IconButtonTransparent {
            icon.source: prefix + "/images/Cancel64.png"
            onClicked: {
                trackAddName.visible = false
            }
        }
        IconButtonTransparent {
            icon.source: prefix + "/images/OK64.png"
            onClicked: {
                trackAddName.visible = false
                TracksInterface.finish_new(trackName.text)
            }
        }
    }
}

