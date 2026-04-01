// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

import "components" as Comp
import "../"

Dialog {
    id: hotKeys
    visible: false
    height: 500 * theme.scaleHeight
    width: 700 * theme.scaleWidth
    anchors.centerIn: parent
    modal: false

    property bool notSaved: false
    property var awaitingButton: null
    property bool awaitingKey: false
    property string oldButtonText: ""
    property var defaults: ["1","2","3","4","5","6","7","8","A","C","G","M","N","P","T","Y"]
    property var key_hotKeys: ["1","2","3","4","5","6","7","8","A","C","G","M","N","P","T","Y"]

    function show() {
        // Загружаем строки из настроек (если там ещё числа, можно преобразовать)
        var loaded = SettingsManager.key_hotKeys;
        if (loaded && loaded.length > 0) {
            // Если loaded[0] – число, преобразуем все элементы в строки
            if (typeof loaded[0] === "number") {
                key_hotKeys = loaded.map(function(code) { return keyToText(code); });
            } else {
                key_hotKeys = loaded.slice(); // уже строки
            }
        } else {
            key_hotKeys = defaults.slice();
        }
        visible = true;
    }

    function startWaiting(wrapper) {

        if (awaitingButton && awaitingButton !== wrapper) {
            awaitingButton.isWaiting = false;
            awaitingButton = null;
            awaitingKey = false;
        }

        if (awaitingButton === wrapper)
            return;

        awaitingButton = wrapper;
        awaitingKey = true;
        oldButtonText = wrapper.buttonText;
        wrapper.isWaiting = true;
        focus = true;
    }

    Item {
        anchors.fill: parent
        focus: true

        Component.onCompleted: forceActiveFocus()

        onActiveFocusChanged: {
            if (!activeFocus) {
                forceActiveFocus()
            }
        }

        Keys.onPressed: function(event) {
            keyPressed(event.key)
            if (awaitingKey && awaitingButton) {
                notSaved = true

                if (event.key === Qt.Key_Escape) {
                    awaitingButton.isWaiting = false
                    awaitingKey = false
                    awaitingButton = null
                    return
                }

                var newArray = key_hotKeys.slice()
                newArray[awaitingButton.hotKeyIndex] = keyToText(event.key)
                key_hotKeys = newArray

                awaitingButton.isWaiting = false
                awaitingKey = false
                awaitingButton = null
            }
        }
    }


        function onKeyPressed(code) {
            if (awaitingKey && awaitingButton) {
                notSaved = true

                if (code === Qt.Key_Escape) {
                    awaitingButton.isWaiting = false
                    awaitingKey = false
                    awaitingButton = null
                    return
                }

                var newArray = key_hotKeys.slice()
                newArray[awaitingButton.hotKeyIndex] = code
                key_hotKeys = newArray

                awaitingButton.isWaiting = false
                awaitingKey = false
                awaitingButton = null
            }
        }


    function keyToDisplay(key) {
        if (key >= 0x20 && key <= 0x7E && key !== 0x20)
            return String.fromCharCode(key)
        switch (key) {
        case Qt.Key_Up: return "↑"
        case Qt.Key_Down: return "↓"
        case Qt.Key_Left: return "←"
        case Qt.Key_Right: return "→"
        case Qt.Key_Return: return "Enter"
        case Qt.Key_Escape: return "Esc"
        case Qt.Key_Space: return "Space"
        case Qt.Key_F1: return "F1"
        case Qt.Key_F2: return "F2"
        case Qt.Key_F3: return "F3"
        case Qt.Key_F4: return "F4"
        case Qt.Key_F5: return "F5"
        case Qt.Key_F6: return "F6"
        case Qt.Key_F7: return "F7"
        case Qt.Key_F8: return "F8"
        case Qt.Key_F9: return "F9"
        case Qt.Key_F10: return "F10"
        case Qt.Key_F11: return "F11"
        case Qt.Key_F12: return "F12"
        case Qt.Key_Shift: return "Shift"
        case Qt.Key_Control: return "Ctrl"
        case Qt.Key_Alt: return "Alt"
        default: return "Key(" + key + ")"
        }
    }
    function keyToText(key) {
        // Для печатных символов (буквы, цифры) возвращаем сам символ
        if (key >= Qt.Key_A && key <= Qt.Key_Z)
            return String.fromCharCode(key);
        if (key >= Qt.Key_0 && key <= Qt.Key_9)
            return String.fromCharCode(key);

        switch (key) {
        case Qt.Key_Up: return "Up";
        case Qt.Key_Down: return "Down";
        case Qt.Key_Left: return "Left";
        case Qt.Key_Right: return "Right";
        case Qt.Key_Return: return "Enter";
        case Qt.Key_Escape: return "Esc";
        case Qt.Key_Space: return "Space";
        case Qt.Key_F1: return "F1";
        case Qt.Key_F2: return "F2";
        case Qt.Key_F3: return "F3";
        case Qt.Key_F4: return "F4";
        case Qt.Key_F5: return "F5";
        case Qt.Key_F6: return "F6";
        case Qt.Key_F7: return "F7";
        case Qt.Key_F8: return "F8";
        case Qt.Key_F9: return "F9";
        case Qt.Key_F10: return "F10";
        case Qt.Key_F11: return "F11";
        case Qt.Key_F12: return "F12";
        case Qt.Key_Shift: return "Shift";
        case Qt.Key_Control: return "Ctrl";
        case Qt.Key_Alt: return "Alt";
        default: return "Key(" + key + ")";
        }
    }

    function textToKey(text) {
        // Обратное преобразование: по строке получаем код клавиши
        if (text.length === 1 && text.match(/[A-Z0-9]/i))
            return text.charCodeAt(0); // Для одиночных букв/цифр

        switch (text) {
        case "Up": return Qt.Key_Up;
        case "Down": return Qt.Key_Down;
        case "Left": return Qt.Key_Left;
        case "Right": return Qt.Key_Right;
        case "Enter": return Qt.Key_Return;
        case "Esc": return Qt.Key_Escape;
        case "Space": return Qt.Key_Space;
        case "F1": return Qt.Key_F1;
        case "F2": return Qt.Key_F2;
        case "F3": return Qt.Key_F3;
        case "F4": return Qt.Key_F4;
        case "F5": return Qt.Key_F5;
        case "F6": return Qt.Key_F6;
        case "F7": return Qt.Key_F7;
        case "F8": return Qt.Key_F8;
        case "F9": return Qt.Key_F9;
        case "F10": return Qt.Key_F10;
        case "F11": return Qt.Key_F11;
        case "F12": return Qt.Key_F12;
        case "Shift": return Qt.Key_Shift;
        case "Ctrl": return Qt.Key_Control;
        case "Alt": return Qt.Key_Alt;
        default:
            // Попытка извлечь число из "Key(123)"
            var match = text.match(/^Key\((\d+)\)$/);
            if (match) return parseInt(match[1], 10);
            return 0; // Неизвестная клавиша
        }
    }

    Comp.TopLine{
        id: topLine
        titleText: qsTr("Hot Keys")
        onBtnCloseClicked: hotKeys.close()
    }

    GridLayout {
        id: mainGrid
        anchors.top: topLine.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomRow.top
        anchors.margins: 5 * theme.scaleWidth
        columns: 4
        rows: 9
        rowSpacing: 5 * theme.scaleHeight
        columnSpacing: 5 * theme.scaleWidth

        Rectangle {
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 70 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Touch button and push new Key")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
        Rectangle {
            Layout.row: 0
            Layout.column: 2
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 70 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Section or Zone")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle {
            Layout.row: 1; Layout.column: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Auto Steer Toggle")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper0
            property int hotKeyIndex: 8
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 1; Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: "" // не используем встроенный текст
                onClicked: hotKeys.startWaiting(wrapper0)
                Text {
                    anchors.fill: parent
                    text: wrapper0.isWaiting ? "..." : wrapper0.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 1; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("1")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper8
            property int hotKeyIndex: 0
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 1; Layout.column: 3
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper8)
                Text {
                    anchors.fill: parent
                    text: wrapper8.isWaiting ? "..." : wrapper8.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 2; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Cycle Lines")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper1
            property int hotKeyIndex: 9
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 2; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper1)
                Text {
                    anchors.fill: parent
                    text: wrapper1.isWaiting ? "..." : wrapper1.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 2; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("2")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper9
            property int hotKeyIndex: 1
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 2; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper9)
                Text {
                    anchors.fill: parent
                    text: wrapper9.isWaiting ? "..." : wrapper9.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 3; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("New Flag")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper2
            property int hotKeyIndex: 10
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 3; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper2)
                Text {
                    anchors.fill: parent
                    text: wrapper2.isWaiting ? "..." : wrapper2.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 3; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("3")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper10
            property int hotKeyIndex: 2
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 3; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper10)
                Text {
                    anchors.fill: parent
                    text: wrapper10.isWaiting ? "..." : wrapper10.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 4; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Zoom Out")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper3
            property int hotKeyIndex: 11
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 4; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper3)
                Text {
                    anchors.fill: parent
                    text: wrapper3.isWaiting ? "..." : wrapper3.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 4; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("4")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper11
            property int hotKeyIndex: 3
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 4; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper11)
                Text {
                    anchors.fill: parent
                    text: wrapper11.isWaiting ? "..." : wrapper11.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 5; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Zoom In")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper4
            property int hotKeyIndex: 12
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 5; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper4)
                Text {
                    anchors.fill: parent
                    text: wrapper4.isWaiting ? "..." : wrapper4.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 5; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("5")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper12
            property int hotKeyIndex: 4
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 5; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper12)
                Text {
                    anchors.fill: parent
                    text: wrapper12.isWaiting ? "..." : wrapper12.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 6; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Snap to Pivot")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper5
            property int hotKeyIndex: 13
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 6; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper5)
                Text {
                    anchors.fill: parent
                    text: wrapper5.isWaiting ? "..." : wrapper5.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 6; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("6")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper13
            property int hotKeyIndex: 5
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 6; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper13)
                Text {
                    anchors.fill: parent
                    text: wrapper13.isWaiting ? "..." : wrapper13.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 7; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Move Line Left")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper6
            property int hotKeyIndex: 14
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 7; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper6)
                Text {
                    anchors.fill: parent
                    text: wrapper6.isWaiting ? "..." : wrapper6.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 7; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("7")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper14
            property int hotKeyIndex: 6
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 7; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper14)
                Text {
                    anchors.fill: parent
                    text: wrapper14.isWaiting ? "..." : wrapper14.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 8; Layout.column: 0
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("Move line Right")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper7
            property int hotKeyIndex: 15
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 8; Layout.column: 1
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper7)
                Text {
                    anchors.fill: parent
                    text: wrapper7.isWaiting ? "..." : wrapper7.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Rectangle {
            Layout.row: 8; Layout.column: 2
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: 40 * theme.scaleHeight
            color: "transparent"
            Text {
                anchors.fill: parent
                text: qsTr("8")
                font.pixelSize: 18 * theme.scaleHeight
                color: "black"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            id: wrapper15
            property int hotKeyIndex: 7
            property string buttonText: key_hotKeys[hotKeyIndex]
            property bool isWaiting: false

            Layout.row: 8; Layout.column: 3
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.preferredHeight: 40 * theme.scaleHeight

            Comp.IconButtonTextBeside {
                anchors.centerIn: parent
                width: hotKeys.width * 0.2
                height: parent.height
                text: ""
                onClicked: hotKeys.startWaiting(wrapper15)
                Text {
                    anchors.fill: parent
                    text: wrapper15.isWaiting ? "..." : wrapper15.buttonText
                    font.pixelSize: 20 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    RowLayout {
        id: bottomRow
        spacing: 20 * theme.scaleWidth
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: btnPinsSave.height + 20 * theme.scaleHeight

        Comp.IconButtonTransparent{
            Layout.topMargin: 20 * theme.scaleHeight
            Layout.bottomMargin: 5 * theme.scaleHeight
            Layout.leftMargin: 20 * theme.scaleHeight
            icon.source: prefix + "/images/UpArrow64.png"
            onClicked: {
                key_hotKeys = defaults.slice();
                notSaved = true
            }
        }

        Item { Layout.fillWidth: true }

        Comp.IconButtonTransparent{
            id: btnPinsSave
            Layout.topMargin: 20 * theme.scaleHeight
            Layout.bottomMargin: 5 * theme.scaleHeight
            enabled: false
            icon.source: notSaved?prefix + "/images/ToolAcceptNotSaved.png":prefix + "/images/ToolAcceptChange.png"
            Text{
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.left
                anchors.rightMargin: 5
                text: qsTr("Send + Save")
            }
            onClicked: {
                SettingsManager.key_hotKeys = key_hotKeys
                notSaved = false
            }
        }

        Comp.IconButtonTransparent{
            id: saveAndClose
            Layout.topMargin: 20 * theme.scaleHeight
            Layout.bottomMargin: 5 * theme.scaleHeight
            Layout.rightMargin: 20 * theme.scaleHeight
            icon.source: prefix + "/images/OK64.png"
            onClicked: {
                hotKeys.close()
                notSaved = false
            }
        }
    }
}
