import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts
import ".."
import "../components"

Rectangle {
    id: buttonContainer
    implicitWidth: theme.buttonSize
    implicitHeight: theme.buttonSize
    Layout.alignment: Qt.AlignCenter
    color: "transparent"
    border.color: "black"
    border.width: 2
    radius: 10

    signal clicked()

    property int mode: 0 // 0, 1, 2
    property string icon1: ""
    property string icon2: ""
    property string icon3: ""
    property string icon4: ""

    Image {
        anchors.fill: parent
        id: icon
        source: icon1
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            buttonContainer.clicked()
            if (mode < 3) {
                mode++;
            } else {
                mode = 0;
            }
            console.log("Режим:", mode);

            // Обновление отображения
            if (mode === 0) {
                icon.source = icon1
            } else if (mode === 1) {
                icon.source = icon2
            } else if (mode === 2) {
                icon.source = icon3
            } else {
                icon.source = icon4
            }
        }
    }
}
