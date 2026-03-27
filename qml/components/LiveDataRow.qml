import QtQuick
import QtQuick.Layouts

RowLayout {
    property string name: ""
    property var value: 0
    property string format: ""
    
    Text {
        text: name
        color: theme.textColor
        font.pixelSize: 13
        Layout.minimumWidth: 160
    }
    
    Text {
        text: {
            if (typeof value === "number") {
                if (format === "N1") return value.toLocaleString(Qt.locale(), 'f', 1);
                if (format === "N2") return value.toLocaleString(Qt.locale(), 'f', 2);
                if (format === "N3") return value.toLocaleString(Qt.locale(), 'f', 3);
                return String(value);
            }
            return String(value);
        }
        color: theme.textColor
        font.pixelSize: 13
        font.family: "monospace"
        horizontalAlignment: Text.AlignRight
    }
}