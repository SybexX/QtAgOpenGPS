import QtQuick
import QtQuick.Controls

pragma ComponentBehavior: Bound

ListView {
    id: tracksView
    property int selected: trk.idx > -1 ? trk.idx : -1
    property bool trackVisible: false

    onVisibleChanged: {
        if (visible && trk.idx > -1) {
            tracksView.selected = trk.idx
            tracksView.currentIndex = trk.idx
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollbar
        policy: ScrollBar.AlwaysOn
        active: ScrollBar.AlwaysOn
    }

    delegate: TrackPickDelegate {
        id: control
        checked: control.index === tracksView.currentIndex

        scrollbar_width: scrollbar.width

        onCheckedChanged: {
            tracksView.selected = control.index
            tracksView.trackVisible = control.isVisible
        }
    }
}
