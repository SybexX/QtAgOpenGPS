// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window to set the colors of the sections
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Fusion
import AOG

import ".."
import "../components"

Dialog {
    id: sectionColors
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: false

    function show() {
        visible = true
    }

    Rectangle {
        id: root
        anchors.fill: parent
        color: aogInterface.backgroundColor

        property int selectedStandardIndex: -1

        property color color01: SettingsManager.color_sec01
        property color color02: SettingsManager.color_sec02
        property color color03: SettingsManager.color_sec03
        property color color04: SettingsManager.color_sec04
        property color color05: SettingsManager.color_sec05
        property color color06: SettingsManager.color_sec06
        property color color07: SettingsManager.color_sec07
        property color color08: SettingsManager.color_sec08
        property color color09: SettingsManager.color_sec09
        property color color10: SettingsManager.color_sec10
        property color color11: SettingsManager.color_sec11
        property color color12: SettingsManager.color_sec12
        property color color13: SettingsManager.color_sec13
        property color color14: SettingsManager.color_sec14
        property color color15: SettingsManager.color_sec15
        property color color16: SettingsManager.color_sec16

        property string customColorString: SettingsManager.display_customSectionColors

        ListModel { id: sectionModel }
        ListModel { id: customColorModel }

        function updateModel() {
            sectionModel.clear()
            sectionModel.append({ "color": root.color01 })
            sectionModel.append({ "color": root.color02 })
            sectionModel.append({ "color": root.color03 })
            sectionModel.append({ "color": root.color04 })
            sectionModel.append({ "color": root.color05 })
            sectionModel.append({ "color": root.color06 })
            sectionModel.append({ "color": root.color07 })
            sectionModel.append({ "color": root.color08 })
            sectionModel.append({ "color": root.color09 })
            sectionModel.append({ "color": root.color10 })
            sectionModel.append({ "color": root.color11 })
            sectionModel.append({ "color": root.color12 })
            sectionModel.append({ "color": root.color13 })
            sectionModel.append({ "color": root.color14 })
            sectionModel.append({ "color": root.color15 })
            sectionModel.append({ "color": root.color16 })
        }

        function updateCustomColorModel() {
            customColorModel.clear()
            if (!root.customColorString) return

            let colors = root.customColorString.split(',')
            for (let i = 0; i < 16; ++i) {
                let colorValue = "gray"
                if (i < colors.length) {
                    let num = parseInt(colors[i].trim(), 10)
                    if (!isNaN(num)) {
                        let r = (num >> 16) & 0xFF
                        let g = (num >> 8) & 0xFF
                        let b = num & 0xFF
                        colorValue = Qt.rgba(r/255, g/255, b/255, 1.0)
                    }
                }
                customColorModel.append({ "color": colorValue })
            }
        }

        function copyColorToStandard(customIndex) {
            if (!SettingsManager.color_isMultiColorSections || root.selectedStandardIndex === -1) return

            let newColor = customColorModel.get(customIndex).color
            switch (root.selectedStandardIndex) {
                case 0: SettingsManager.color_sec01 = newColor; break
                case 1: SettingsManager.color_sec02 = newColor; break
                case 2: SettingsManager.color_sec03 = newColor; break
                case 3: SettingsManager.color_sec04 = newColor; break
                case 4: SettingsManager.color_sec05 = newColor; break
                case 5: SettingsManager.color_sec06 = newColor; break
                case 6: SettingsManager.color_sec07 = newColor; break
                case 7: SettingsManager.color_sec08 = newColor; break
                case 8: SettingsManager.color_sec09 = newColor; break
                case 9: SettingsManager.color_sec10 = newColor; break
                case 10: SettingsManager.color_sec11 = newColor; break
                case 11: SettingsManager.color_sec12 = newColor; break
                case 12: SettingsManager.color_sec13 = newColor; break
                case 13: SettingsManager.color_sec14 = newColor; break
                case 14: SettingsManager.color_sec15 = newColor; break
                case 15: SettingsManager.color_sec16 = newColor; break
            }
            root.selectedStandardIndex = -1
        }

        Component.onCompleted: {
            root.updateModel()
            root.updateCustomColorModel()
        }

        Connections {
            target: SettingsManager
            function onColor_sec01Changed() { root.updateModel() }
            function onColor_sec02Changed() { root.updateModel() }
            function onColor_sec03Changed() { root.updateModel() }
            function onColor_sec04Changed() { root.updateModel() }
            function onColor_sec05Changed() { root.updateModel() }
            function onColor_sec06Changed() { root.updateModel() }
            function onColor_sec07Changed() { root.updateModel() }
            function onColor_sec08Changed() { root.updateModel() }
            function onColor_sec09Changed() { root.updateModel() }
            function onColor_sec10Changed() { root.updateModel() }
            function onColor_sec11Changed() { root.updateModel() }
            function onColor_sec12Changed() { root.updateModel() }
            function onColor_sec13Changed() { root.updateModel() }
            function onColor_sec14Changed() { root.updateModel() }
            function onColor_sec15Changed() { root.updateModel() }
            function onColor_sec16Changed() { root.updateModel() }
        }

        onCustomColorStringChanged: root.updateCustomColorModel()

        TopLine {
            id: topLine
            titleText: qsTr("Section Color Set")
            onBtnCloseClicked: sectionColors.close()
        }

        Rectangle {
            id: standardRect
            anchors.top: topLine.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 100 * theme.scaleHeight
            anchors.margins: 50 * theme.scaleHeight
            enabled: SettingsManager.color_isMultiColorSections

            GridView {
                anchors.fill: parent
                model: sectionModel
                cellWidth: standardRect.width / 8
                cellHeight: standardRect.height / 2
                interactive: false
                clip: true

                delegate: Item {
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        color: model.color
                        border.width: root.selectedStandardIndex === index ? 3 : 1
                        border.color: root.selectedStandardIndex === index ? "yellow" : "black"

                        Text {
                            anchors.centerIn: parent
                            text: index + 1
                            color: "black"
                            font.pixelSize: 20
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.selectedStandardIndex = index
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: customRect
            anchors.top: standardRect.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 100 * theme.scaleHeight
            anchors.margins: 50 * theme.scaleHeight
            enabled: SettingsManager.color_isMultiColorSections

            GridView {
                anchors.fill: parent
                model: customColorModel
                cellWidth: customRect.width / 8
                cellHeight: customRect.height / 2
                interactive: false
                clip: true

                delegate: Item {
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        color: model.color
                        border.width: 1
                        border.color: "black"

                        Text {
                            anchors.centerIn: parent
                            text: index + 1
                            color: "black"
                            font.pixelSize: 20
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.copyColorToStandard(index)
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            id: buttons
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 10
            height: children.height
            width: parent.width * 0.3

            IconButtonTransparent {
                checkable: true
                checked: SettingsManager.color_isMultiColorSections
                text: qsTr("Multi Color")
                icon.source: prefix + "/images/ColourPick.png"
                onClicked: {
                    SettingsManager.color_isMultiColorSections = !SettingsManager.color_isMultiColorSections
                    if (!SettingsManager.color_isMultiColorSections)
                        root.selectedStandardIndex = -1
                }
            }

            IconButtonTransparent {
                icon.source: prefix + "/images/Cancel64.png"
                onClicked: sectionColors.visible = false
            }

            IconButtonTransparent {
                icon.source: prefix + "/images/OK64.png"
                onClicked: {
                    aog.settingsSave()
                    aog.settingsReload()
                    sectionColors.visible = false
                }
            }
        }
    }
}
