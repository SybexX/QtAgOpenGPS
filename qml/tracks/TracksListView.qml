import QtQuick
import QtQuick.Controls
// Interface import removed - now QML_SINGLETON

pragma ComponentBehavior: Bound

ListView {
    id: tracksView
    property int selected: -1
    property bool trackVisible: false

    Component.onCompleted: {
        console.log("=== TracksListView DEBUG ===")
        console.log("TracksInterface available:", typeof TracksInterface !== 'undefined')
        if (typeof TracksInterface !== 'undefined') {
            console.log("TracksInterface.idx:", TracksInterface.idx)
            console.log("TracksInterface.count:", TracksInterface.count)
            console.log("TracksInterface.model available:", TracksInterface.model !== undefined)
        } else {
            console.log("ERROR: TracksInterface is undefined in TracksListView!")
        }
        console.log("=== END TracksListView DEBUG ===")
    }

    onVisibleChanged: {
        if (visible && TracksInterface.idx > -1) {
            console.debug("turning on track ",TracksInterface.idx)
            //tracksView.selected = TracksInterface.idx
            tracksView.currentIndex = TracksInterface.idx
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollbar
        policy: ScrollBar.AlwaysOn
        active: ScrollBar.AlwaysOn
    }

    keyNavigationEnabled: true

    delegate: TrackPickDelegate {
        id: control
        checked: control.index == tracksView.currentIndex

        scrollbar_width: scrollbar.width

        onCheckedChanged: {
            //tracksView.selected = control.index
            tracksView.currentIndex = control.index
            tracksView.trackVisible = control.isVisible
            console.debug("checked event... track selected is ",tracksView.currentIndex)
        }

        //Component.onCompleted: {
        //    if (control.index == tracksView.currentIndex)
        //        checked = true
        //}
    }
}
