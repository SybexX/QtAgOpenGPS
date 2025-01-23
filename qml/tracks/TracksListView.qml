import QtQuick
import QtQuick.Controls

ListView {
    property int selected: -1

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AlwaysOn
        active: ScrollBar.AlwaysOn
    }

    delegate: TrackPickDelegate {
        id: control
        onCheckedChanged: {
            selected = model.index
        }
    }
    /*delegate: Text {
        id: t
        text: name
    }*/
}
