// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// IP Camera popup with video stream

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import AOG

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

    TopLine{
        id: cameraTopLine
        titleText: qsTr("IP Camera")
        onBtnCloseClicked: { camera.close(); mediaPlayer.stop() }
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOutput
        onErrorOccurred: console.log("MediaPlayer error:", mediaPlayer.errorString)
    }

    VideoOutput {
        id: videoOutput
        anchors.top: cameraTopLine.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    MouseArea {
        anchors.top: cameraTopLine.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        onPressAndHold: contextMenu.popup()
        onClicked: contextMenu.visible = !contextMenu.visible

        Menu {
            id: contextMenu
            MenuItem {
                text: qsTr("Open URL")
                onTriggered: urlInputDialog.open()
            }
            MenuItem {
                text: qsTr("Load cam1")
                onTriggered: loadCam1()
            }
            MenuItem {
                text: mediaPlayer.playbackState === MediaPlayer.Playing ? qsTr("Stop") : qsTr("Play")
                onTriggered: {
                    if (mediaPlayer.playbackState === MediaPlayer.Playing) mediaPlayer.stop()
                    else mediaPlayer.play()
                }
            }
            MenuItem {
                text: qsTr("Close")
                onTriggered: { camera.close(); mediaPlayer.stop() }
            }
        }
    }

    Popup {
        id: urlInputDialog
        anchors.centerIn: parent
        ColumnLayout {
            TextField {
                id: urlInput
                placeholderText: qsTr("RTSP URL")
                text: SettingsManager.cam_camLink
                Layout.minimumWidth: 300
            }
            RowLayout {
                Button {
                    text: qsTr("Load")
                    onClicked: {
                        SettingsManager.cam_camLink = urlInput.text
                        loadUrl(urlInput.text)
                        urlInputDialog.close()
                    }
                }
                Button {
                    text: qsTr("Cancel")
                    onClicked: urlInputDialog.close()
                }
            }
        }
    }

    function loadUrl(url) {
        mediaPlayer.stop()
        mediaPlayer.source = url
        mediaPlayer.play()
    }

    function loadCam1() {
        loadUrl(SettingsManager.cam_camLink)
    }

    onVisibleChanged: {
        if (visible) loadCam1()
    }
}
