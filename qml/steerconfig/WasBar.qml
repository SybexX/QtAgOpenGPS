import QtQuick 2.15
import QtQuick.Controls.Fusion
import QtQuick.Layouts

Rectangle {
    id:wasbar
    height: 15 * theme.scaleHeight
    color: "lightGrey"
    border.color: black
    border.width: 2
    property int wasvalue: 0

    Rectangle {
        id: wasbar_right
        width: wasvalue > 0 ? (wasvalue>parent.width/2 ? parent.width/2:wasvalue) : 0
        height: parent.height
        color: "green"
        anchors.left: wasbar.horizontalCenter
        Layout.maximumWidth: parent.width/2
}
    Rectangle {
        id: wasbar_left
        width: wasvalue > 0 ? 0 : (wasvalue<parent.width/-2 ? parent.width/2:wasvalue*-1)
        height: parent.height
        color: "green"
        anchors.right: wasbar.horizontalCenter
        Layout.maximumWidth: parent.width/2
}
}
