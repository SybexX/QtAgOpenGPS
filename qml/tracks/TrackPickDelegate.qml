import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RadioDelegate {
    id: control
    text: model.name
    checked: model.index === trk.idx

    Component.onCompleted: {
        console.debug(width)
        controlWidth = width
    }

    property double controlWidth: width

    contentItem: RowLayout {
        anchors.left: parent.left
        //anchors.right: parent.right
        //width: control.width

            Component.onCompleted: {
                console.debug(control.width)
                console.debug(width)
            }

        Image {
            id: trackType
            width: 50
            height: 50
            anchors.margins: 2

            source: (model.mode === 2) ? prefix + "/images/TrackLine.png" :
                    (model.mode === 4) ? prefix + "/images/TrackCurve.png" :
                    (model.mode === 64) ? prefix + "/images/TrackPivot.png" :
                                       prefix + "/images/HelpSmall.png"
        }

        Rectangle {
            width: controlWidth - 50 - trackname.height - 200 //control.width - 50 - trackname.height
            height: trackname.height
            color: "grey"
            Text {
                id: trackname
                width: parent.width - 50 - trackname.height - 100

                //rightPadding: control.indicator.width + control.spacing
                text: "Testing"
                font: control.font
                opacity: enabled ? 1.0 : 0.3
                color: "light blue" //control.checked ? "white" : "black"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Component.onCompleted: {
                console.debug(control.width)
                console.debug(width)
            }
        }

        Button {
            id: isTrackVisible
            background: Rectangle {
                implicitWidth: trackname.height
                implicitHeight: trackname.height
                color: model.isVisible ? "green" : "red"
            }
            onClicked: {
                tracksInterface.setVisible(model.index, !model.isVisible)
                model.isVisible = !model.isVisible
            }
        }
    }

    indicator: Rectangle {
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        visible: control.down || control.highlighted || control.checked
        color: control.checked ? aog.blackDayWhiteNight : aog.backgroundColor
    }
}

