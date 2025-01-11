import QtQuick
import QtQuick.Controls

ListView {
    property int selected: -1
    model: trk.model

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
}
