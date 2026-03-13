// BlockageRow.qml
import QtQuick

Rectangle {
    id: block
    width: 15 * theme.scaleWidth
    height: 100 * theme.scaleHeight
    border.width: 1
    border.color: "black"
    property string buttonText: ""
    property color textColor: "black"
    property bool useColorBasedAnchors: true // Новое свойство для переключения режима
    radius: 5 * theme.scaleWidth

    // Важно: Добавляем проверку на наличие parent
    property bool parentAvailable: parent !== null

    // Начальная позиция - используем только если parent доступен
    anchors.bottom: parentAvailable ? parent.bottom : undefined

    // Состояния для разных цветов
    states: [
        State {
            name: "redState"
            when: block.useColorBasedAnchors && (
                block.color.toString() === "#ff0000" ||
                block.color.toString() === "#800000" ||
                block.color === Qt.rgba(1, 0, 0, 1) ||
                String(block.color).includes("red")
            )
            AnchorChanges {
                target: block
                anchors.top: parent.verticalCenter
                anchors.bottom: undefined
                anchors.verticalCenter: undefined
            }
        },
        State {
            name: "yellowState"
            when: block.useColorBasedAnchors && (
                block.color.toString() === "#ffff00" ||
                block.color.toString() === "#b8860b" ||
                block.color === Qt.rgba(1, 1, 0, 1) ||
                String(block.color).includes("yellow")
            )
            AnchorChanges {
                target: block
                anchors.bottom: parent.verticalCenter
                anchors.top: undefined
                anchors.verticalCenter: undefined
            }
        },
        State {
            name: "greenState"
            when: block.useColorBasedAnchors && (
                block.color.toString() === "#00ff00" ||
                block.color.toString() === "#228b22" ||
                block.color === Qt.rgba(0, 1, 0, 1) ||
                String(block.color).includes("lime")
            )
            AnchorChanges {
                target: block
                anchors.verticalCenter: parent.verticalCenter
                anchors.top: undefined
                anchors.bottom: undefined
            }
        },
        State {
            name: "defaultState"
            when: !block.useColorBasedAnchors && block.parentAvailable
            AnchorChanges {
                target: block
                anchors.bottom: parent.bottom
                anchors.top: undefined
                anchors.verticalCenter: undefined
            }
        }
    ]

    // Обновляем parentAvailable при изменении parent
    onParentChanged: {
        parentAvailable = (parent !== null)
    }

    Text {
        id: label
        color: textColor
        text: block.buttonText
        anchors.bottom: parent.bottom
        font.pointSize: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
