// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// UI  colors and sounds.
import QtQuick
import QtQuick.Controls.Fusion
import QtMultimedia

/* This type contains the sounds, colors, and perhaps screen sizes,
  *Sounds will track aogInterface.qml, and react when needed as set
  by the features page.
  *Colors will follow the settings files, and change based on day/night
  mode, so we don't need an if statement in every object.
  *Screen sizes-We'll see.
  */

Item {
    id: theme

    // Qt 6.8 QProperty + BINDABLE: Theme properties to allow setProperty() updates from C++
    property bool isDayMode: true
    property bool soundAutoSteerOn: true
    property bool soundHydLiftOn: true
    property bool soundSectionOn: true
    property bool soundUturnOn: true
    property bool isRTK: false
    property double gpsAgeAlarm: 5.0

    property int defaultHeight: 768
    property int defaultWidth: 1024
    property double scaleHeight: mainWindow.height / defaultHeight
    property double scaleWidth: mainWindow.width / defaultWidth

    property color backgroundColor: "ghostWhite"
    property color textColor: "black"
    property color borderColor: "lightblue"
    property color blackDayWhiteNight: "black"
    property var btnSizes: [100, 100, 100]//clockwise, right bottom left
    property int buttonSize: 100
    function buttonSizesChanged() {
        buttonSize = Math.min(...btnSizes) - 2.5
        //console.log("Button size is now " + buttonSize)
    }
    property color whiteDayBlackNight: "white"


    //curr / default

    Connections{
        target: mainWindow
        function onHeightChanged(){
            scaleHeight = mainWindow.height / defaultHeight
        }
        function onWidthChanged(){
            scaleWidth = mainWindow.width / defaultWidth
        }
    }
    // Qt 6.8 QProperty + BINDABLE: Function to update theme colors
    function updateTheme() {
        // Threading Phase 1: Day/night mode theme switching
        if (isDayMode) {
            backgroundColor = "ghostWhite"
            textColor = "black"
            borderColor = "lightBlue"
            blackDayWhiteNight = "black"
            whiteDayBlackNight = "white"
        } else {
            backgroundColor = "darkgray"
            textColor = "white"
            borderColor = "lightGray"
            blackDayWhiteNight = "white"
            whiteDayBlackNight = "black"
        }
    }

    Component.onCompleted: updateTheme()

    // Threading Phase 1: Monitor SettingsManager changes via signals
    // Note: SettingsManager doesn't provide property change signals
    // Theme updates handled manually via updateTheme() calls
    Item {//button sizes
        width: 600
        enum ScreenSize {
            Phone, // 6" or less
            SmallTablet, //6-10"
            LargeTablet, //10" or larger
            Large //regular computer screen.
        }

        /*	property int screenDiag: Math.sqrt(Screen.width * Screen.width + Screen.height * Screen.height) / Screen.pixelDensity
    property int screenType: screenDiag < 165 ? Sizes.ScreenSize.Phone :
                             screenDiag < 230 ? Sizes.ScreenSize.SmallTablet :
                             screenDiag < 355 ? Sizes.ScreenSize.LargeTablet : Sizes.ScreenSize.Large

    property int buttonSquare: screenType == Sizes.ScreenSize.Phone ? 10 * Screen.pixelDensity :
                               screenType == Sizes.ScreenSize.SmallTablet ? 20 * Screen.pixelDensity :
                               screenType == Sizes.ScreenSize.LargeTablet ? 25 * Screen.pixelDensity : Screen.height / 12*/


    }
//     Connections{//sounds functions for AOGInterface local properties
//         target: aogInterface  // Target the local QML Item, not the C++ context property
//         function onIsBtnAutoSteerOnChanged() {//will need another function for every sound option
//             if(soundAutoSteerOn){//does the user want the sound on?
//                 if(aog.isBtnAutoSteerOn)
//                     engage.play()
//                 else
//                     disEngage.play()
//             }
//         }
//         function onAutoBtnStateChanged(){
//             if(soundSectionOn)
//                 sectionOn.play()
//         }
//         function onManualBtnStateChanged(){
//             if(soundSectionOn)
//                 sectionOff.play()
//         }
//         function onAgeChanged(){
//             if(Backend.fixFrame.age > gpsAgeAlarm)
//                 if(isRTK)
//                     rtkLost.play()
//         }
//         function onDistancePivotToTurnLineChanged(){
//             if(Backend.distancePivotToTurnLine == 20)
//                 if(soundUturnOn)
//                     approachingYouTurn.play()
//         }
//     }

//     Connections{//sounds functions for VehicleInterface properties
//         target: VehicleInterface
//         function onHydLiftDownChanged(){
//             if(soundHydLiftOn){
//                 if(VehicleInterface.hydLiftDown)
//                     hydDown.play()
//                 else
//                     hydUp.play()
//             }
//         }
//     }
//     //region sounds
//     //as far as I can tell, these are all necessary
//     SoundEffect{
//         id: engage
//         source: prefix + "/sounds/SteerOn.wav"
//     }
//     SoundEffect{
//         id: disEngage
//         source: prefix + "/sounds/SteerOff.wav"
//     }
//     SoundEffect{
//         id: hydDown
//         source: prefix + "/sounds/HydDown.wav"
//     }
//     SoundEffect{
//         id: hydUp
//         source: prefix + "/sounds/HydUp.wav"
//     }
//     SoundEffect{
//         id: sectionOff
//         source: prefix + "/sounds/SectionOff.wav"
//     }
//     SoundEffect{
//         id: sectionOn
//         source: prefix + "/sounds/SectionOn.wav"
//     }
//     SoundEffect{
//         id: approachingYouTurn
//         source: prefix + "/sounds/Alarm10.wav"
//     }
//     SoundEffect{
//         id: rtkLost
//         source: prefix + "/sounds/rtk_lost.wav"
//     }
//     SoundEffect{
//         id: youturnFail
//         source: prefix + "/sounds/TF012.wav"
//     }//endregion sounds
 }
