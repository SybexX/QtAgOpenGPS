import QtQuick.Layouts
import QtQuick 2.15
import QtQuick.Controls.Fusion
import "interfaces" as Interfaces
import "boundary" as Boundary
import "steerconfig" as SteerConfig
import "config" as ConfigSettings //"Config" causes errors
import "field" as Field
import "tracks" as Tracks
import "components" as Comp

Rectangle {
    width: 800
    height: 600
    color: "grey"

    property string prefix: "/home/torriem/projects/QtAgOpenGPS"

    AOGTheme {
        id: theme
    }

    AOGInterface {
        id: aog
    }

    MockSettings {
        id: settings
    }
    
    UnitConversion {
        id: utils
    }

    MockTracks {
        id: trk

        Component.onCompleted: {
            trk.idx = 0
        }

    }

    Interfaces.TracksInterface {
        id: tracksInterface
    }


/*    Tracks.TracksListView {
        id: tracksListView
        width: 400
        height: 100
        model: trk.model
        clip: true
    }


*/
    Tracks.TrackList {
        id: trackList
        visible: true
    }

}
