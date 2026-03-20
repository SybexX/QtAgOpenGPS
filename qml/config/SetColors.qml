// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Window to set the colors of the simulator
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Fusion
import AOG

import ".."
import "../components"

Dialog {
    id: setColors
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: false

    // Убираем стандартный заголовок и отступы
    title: ""
    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    function show() {
        visible = true
    }

    // Основной контент
    contentItem: Rectangle {
        id: root
        color: aogInterface.backgroundColor

        // Верхняя панель (не в лейауте, чтобы избежать конфликта)
        TopLine {
            id: topLine
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            titleText: qsTr("Color Set")
            onBtnCloseClicked: setColors.close()
        }

        // Контейнер для остального содержимого (расположен под TopLine)
        Item {
            anchors.top: topLine.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            // Используем ColumnLayout для вертикального заполнения
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                anchors.margins: 10

                // Основная область: слева изображение, справа кнопки
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    // Изображение-превью
                    Image {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 200
                        source: prefix + "/images/ColorBackGnd.png"
                        fillMode: Image.PreserveAspectFit
                    }

                    // Колонка кнопок выбора цвета
                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 200
                        spacing: 10

                        IconButtonColor {
                            Layout.fillWidth: true
                            icon.source: prefix + "/images/ColourPick.png"
                            onClicked: cpFieldColor.open()
                            text: qsTr("Color Field")
                            color: btnDayNight.checked ? SettingsManager.display_colorFieldNight : SettingsManager.display_colorFieldDay
                        }

                        IconButtonColor {
                            Layout.fillWidth: true
                            icon.source: prefix + "/images/ColourPick.png"
                            onClicked: cpFrameColor.open()
                            text: qsTr("Color Frame")
                            color: btnDayNight.checked ? SettingsManager.display_colorNightFrame : SettingsManager.display_colorDayFrame
                        }

                        IconButtonColor {
                            Layout.fillWidth: true
                            icon.source: prefix + "/images/ColourPick.png"
                            onClicked: cpTextColor.open()
                            text: qsTr("Color Text")
                            color: btnDayNight.checked ? SettingsManager.display_colorTextNight : SettingsManager.display_colorTextDay
                        }

                        IconButtonColor {
                            id: btnDayNight
                            Layout.fillWidth: true
                            text: qsTr("Day/Night")
                            icon.source: prefix + "/images/Config/ConD_AutoDayNight.png"
                            checkable: true
                            color1: "blue"
                            colorChecked1: "black"
                            colorChecked2: "black"
                            colorChecked3: "black"
                            onVisibleChanged: if (visible) btnDayNight.checked = false
                        }
                    }
                }

                // Нижняя панель с кнопками OK/Cancel
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    spacing: 10

                    IconButtonTransparent {
                        icon.source: prefix + "/images/Cancel64.png"
                        onClicked: setColors.visible = false
                    }

                    IconButtonTransparent {
                        icon.source: prefix + "/images/OK64.png"
                        onClicked: {
                            aog.settingsSave()
                            aog.settingsReload()
                            setColors.visible = false
                        }
                    }
                }
            }
        }
    }

    // Цветовые диалоги (остаются без изменений)
    ColorDialog {
        id: cpFieldColor
        onSelectedColorChanged: {
            if (btnDayNight.checked)
                SettingsManager.display_colorFieldNight = cpFieldColor.selectedColor
            else
                SettingsManager.display_colorFieldDay = cpFieldColor.selectedColor
        }
    }

    ColorDialog {
        id: cpFrameColor
        onSelectedColorChanged: {
            if (btnDayNight.checked)
                SettingsManager.display_colorNightFrame = cpFrameColor.selectedColor
            else
                SettingsManager.display_colorDayFrame = cpFrameColor.selectedColor
        }
    }

    ColorDialog {
        id: cpTextColor
        onSelectedColorChanged: {
            if (btnDayNight.checked)
                SettingsManager.display_colorTextNight = cpTextColor.selectedColor
            else
                SettingsManager.display_colorTextDay = cpTextColor.selectedColor
        }
    }
}
