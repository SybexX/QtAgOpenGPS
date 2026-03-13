// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// On main GL
//import QtQuick 2.7
//import QtQuick.Controls 2.3
//import QtMultimedia 5.8

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import ".."
import "../components"
import "../wizards"

MoveablePopup {
    id: camera
    height: 300  * theme.scaleHeight
    width: 500  * theme.scaleWidth
    visible: false
    modal: false
    x: 400 * theme.scaleWidth
    onOpened: menuBar.autoload = true

/*
    TopLine{
        id: cameraTopLine
        titleText: qsTr("Cam1")
        onBtnCloseClicked:  camera.close()
    }

*/
    MediaPlayer {
           id: mediaPlayer
           videoOutput: videoOutput
           onErrorOccurred: { mediaErrorText.text = mediaPlayer.errorString; mediaError.open() }
       }

       PlayerMenuBar {
           id: menuBar
           anchors.left: parent.left
           anchors.right: parent.right
           visible: true
           mediaPlayer: mediaPlayer
           videoOutput: videoOutput
           onClosePlayer: camera.close(), mediaPlayer.stop()
       }


       VideoOutput {
           id: videoOutput
           anchors.top: menuBar.bottom
           anchors.bottom: parent.bottom
           anchors.left: parent.left
           anchors.right: parent.right
       }


}
