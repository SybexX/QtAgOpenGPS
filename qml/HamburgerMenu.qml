// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Top left button menu
import QtQuick
import QtQuick.Controls.Fusion
//import Settings

import "components" as Comp

Drawer{
    id: hamburgerMenuRoot
    width: 270 * theme.scaleWidth
    height: mainWindow.height
    visible: false
    modal: true

    contentItem: Rectangle{
        id: hamburgerMenuContent
        anchors.fill: parent

        // PHASE6-0-20: Gestion des états de langue avec states QML
        states: [
            State {
                name: "english"
                when: SettingsManager.menu_language === "en"
                PropertyChanges { target: btnEnglish; checked: true }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "french"
                when: SettingsManager.menu_language === "fr"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: true }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "russian"
                when: SettingsManager.menu_language === "ru"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: true }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "serbian"
                when: SettingsManager.menu_language === "sr"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: true }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "german"
                when: SettingsManager.menu_language === "de"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: true }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "hindi"
                when: SettingsManager.menu_language === "hi"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: true }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "kazak"
                when: SettingsManager.menu_language === "kz"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false }
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: true }
                PropertyChanges { target: btnBrasil; checked: false }
            },
            State {
                name: "brasil"
                when: SettingsManager.menu_language === "pt"
                PropertyChanges { target: btnEnglish; checked: false }
                PropertyChanges { target: btnFrench; checked: false }
                PropertyChanges { target: btnRussian; checked: false }
                PropertyChanges { target: btnSerbian; checked: false }
                PropertyChanges { target: btnGerman; checked: false}
                PropertyChanges { target: btnHindi; checked: false }
                PropertyChanges { target: btnKazak; checked: false }
                PropertyChanges { target: btnBrasil; checked: true }
            }
        ]
        height: fieldMenu.height
        color: "black"

        Comp.ScrollViewExpandableColumn{
            id: hamburgerMenuColumn
            anchors.fill: parent

            Comp.IconButtonTextBeside{
                text: qsTr("Languages")
                onClicked: languagesMenu.visible = !languagesMenu.visible
                visible: true
                icon.source: prefix + "/images/lang.png"
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Directories")
                onClicked: console.log("")
                visible: false//todo
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Set Colors")
                icon.source: prefix + "/images/ColourPick.png"
                onClicked: {
                    console.log("showing")
                    hamburgerMenuRoot.visible = false
                    setColors.show()
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Section Colors")
                onClicked: {
                hamburgerMenuRoot.visible = false
                sectionColors.show()
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Enter Sim Coords")
                icon.source: prefix + "/images/simCoord.png"
                onClicked: {
                    console.log("showing")
                    hamburgerMenuRoot.visible = false
                    setSimCoords.show()
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Simulator On")
                // Threading Phase 1: Simulator mode toggle - Qt 6.8 QProperty binding
                checkable: true
                checked: SettingsManager.menu_isSimulatorOn
                onCheckedChanged: {
                    SettingsManager.menu_isSimulatorOn = checked
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("FieldView Test (Ctrl+Shift+F)")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    mainWindow.toggleFieldViewTest()
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Reset All")
                onClicked: console.log("")
                visible: false//todo
            }
            Comp.IconButtonTextBeside{
                text: qsTr("HotKeys")
                visible: true
                icon.source: prefix + "/images/hotkey.png"
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    hotKeySettings.show()
                }
            }
            Comp.IconButtonTextBeside{
                text: qsTr("About...")
                onClicked: console.log("")
                visible: false//todo
            }
            Comp.IconButtonTextBeside{
                text: qsTr("Help")
                onClicked: console.log("")
                visible: false//todo
            }
        }
    }
    Drawer {
        id: languagesMenu
        width: 270 * theme.scaleWidth
        height: mainWindow.height
        modal: true

        contentItem: Rectangle{
            id: languagesMenuContent
            anchors.fill: parent
            height: languagesMenu.height
            color: aogInterface.blackDayWhiteNight
        }

        Grid {
            id: grid2
            height: childrenRect.height
            width: childrenRect.width
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 5
            spacing: 10
            flow: Grid.TopToBottom
            rows: 8
            columns: 1

            Comp.IconButtonTextBeside{
                id: btnEnglish
                text: qsTr("English")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    // Threading Phase 1: Language selection - English
                    SettingsManager.menu_language = "en"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnFrench
                text: qsTr("French")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    // Threading Phase 1: Language selection - French
                    SettingsManager.menu_language = "fr"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnRussian
                text: qsTr("Russian")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    // Threading Phase 1: Language selection - Russian
                    SettingsManager.menu_language = "ru"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnSerbian
                text: qsTr("Srpski")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    // Threading Phase 1: Language selection - Serbian
                    SettingsManager.menu_language = "sr"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnGerman
                text: qsTr("Deutch")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    // Threading Phase 1: Language selection - German
                    SettingsManager.menu_language = "de"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnHindi
                text: qsTr("हिंदी")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    SettingsManager.menu_language = "hi"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnKazak
                text: qsTr("Қазақ")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    SettingsManager.menu_language = "kz"
                    aog.settings_save()}
            }
            Comp.IconButtonTextBeside{
                id: btnBrasil
                text: qsTr("Brasil")
                onClicked: {
                    hamburgerMenuRoot.visible = false
                    SettingsManager.menu_language = "pt"
                    aog.settings_save()}
            }
        }
    }
}
