import QtQuick
import QtQuick.Layouts

RowLayout {
    property string name: ""
    property var value: 0
    property int decimals: 0
    
    Text {
        text: name
        color: theme.textColor
        font.pixelSize: 14
        Layout.minimumWidth: 100  * theme.scaleWidth
    }
    
    Text {
        text: {
            if (typeof value === "number") {
                return decimals > 0 ? value.toFixed(decimals) : String(value);
            }
            return String(value);
        }
        color: theme.textColor
        font.pixelSize: 14
        font.family: "monospace"
        horizontalAlignment: Text.AlignRight
    }
}
