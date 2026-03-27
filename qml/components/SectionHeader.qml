import QtQuick

Text {
    property string title: ""
    text: title
    font.pixelSize: 16
    font.bold: true
    color: theme.textColor
    topPadding: 10
    bottomPadding: 5
}