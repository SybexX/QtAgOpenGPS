import QtQuick

Rectangle {
    id: wasbar
    height: 15 * theme.scaleHeight
    color: "lightGrey"
    border.color: "black"
    border.width: 2
    property int wasvalue: 0

    Rectangle {
        id: wasbar_right
        width: wasvalue > 0 ? (wasvalue > parent.width/2 ? parent.width/2 : wasvalue) : 0
        height: parent.height
        color: "green"
        anchors.left: wasbar.horizontalCenter
    }

    Rectangle {
        id: wasbar_center
        width: parent.height / 3
        height: parent.height * 1.5
        color: "green"
        anchors.centerIn: parent
    }

    Rectangle {
        id: wasbar_left
        width: wasvalue > 0 ? 0 : (wasvalue < parent.width/-2 ? parent.width/2 : -wasvalue)
        height: parent.height
        color: "green"
        anchors.right: wasbar.horizontalCenter
    }
}
