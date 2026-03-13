import QtQuick
import QtQuick.Controls.Fusion
// 
import QtMultimedia

/* This type contains the sounds, colors, and perhaps screen sizes,
 *Sounds will track AgIOService.qml, and react when needed as set
 by the features page.
 *Colors will follow the AgIOService files, and change based on day/night
 mode, so we don't need an if statement in every object.
 *Screen sizes-We'll see.
 */

Item {
	id: agioTheme
	AgIOInterface {
		id: agio
		objectName: "agio"
	}

	property int defaultHeight: 500
	property int defaultWidth: 350
    property double scaleHeight: mainWindowAgIO.height / defaultHeight
    property double scaleWidth: mainWindowAgIO.width / defaultWidth

	property color backgroundColor: "ghostWhite"
	property color textColor: "black"
	property color borderColor: "lightblue"
	property color blackDayWhiteNight: "black"
	property color whiteDayBlackNight: "white"
	Connections{
        target: mainWindowAgIO
		function onHeightChanged(){
            scaleHeight = mainWindowAgIO.height / defaultHeight
		}
		function onWidthChanged(){
            scaleWidth = mainWindowAgIO.width / defaultWidth
		}
	}

	//sound functions go here
	/*Connections {
		target: agio
		function onConnectionLost(){
			console.log("connection lost")
		}
	}*/
}
