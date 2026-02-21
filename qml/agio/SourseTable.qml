// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Loaded by FieldOpen.qml. Contains the list of fields
import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts
import AOG
import "../" //bring in Utils


pragma ComponentBehavior: Bound

ListView {
    id: sourseTable
    //needed for qmlscene testing
    Component.onCompleted: update_model()
    onVisibleChanged: selectPortByName(portName)
    clip: true

    property int sortBy: 1 //1 = name, 2 = portType, negative is reverse
    property string portName: ""
    property string portBaud: ""
    property string currentType: ""

    onCurrentIndexChanged: {
        if (currentIndex >= 0 && currentIndex < portModel.count) {
            var item = portModel.get(currentIndex)
            currentType = item.portType
            currentFieldName = item.name
        } else {
            currentType = ""
            currentFieldName = ""
        }
    }

    onSortByChanged: sort()

    function clear_selection() {
        currentIndex = -1
        currentFieldName = ""
        currentType = ""
    }

    // Добавьте эту функцию в SourseTable
    function selectPortByName(portName) {
        if (!portName || portName === "") {
            clear_selection()
            return false
        }
        for (var i = 0; i < portModel.count; i++) {
            var item = portModel.get(i)
            if (item.name === portName) {
                currentIndex = i
                currentFieldName = item.name
                currentType = item.portType
                return true
            }
        }
        clear_selection()
        return false
    }

    function update_model() {
        var portType = "0.0"

        portModel.clear()

        // Тестовые данные
        var testFields = [
            { name: "ttyHSL0", portType: "SERIAL", portParameter: "115200" },
            { name: "ttyHSL1", portType: "SERIAL", portParameter: "460800" },
            { name: "ttyHSL2", portType: "SERIAL", portParameter: "921600" },
            { name: "ttyHSL3", portType: "SERIAL", portParameter: "115200" },
            { name: "ttyACM0", portType: "USB", portParameter: "115200" },
            { name: "RTK_GNSS", portType: "BT", portParameter: "00:12:50:00" },
            { name: "UDP", portType: "UDP", portParameter: "192.168.0.1" },
            { name: "SIM", portType: "SIM", portParameter: "" },
            { name: "GeoLocation", portType: "GnssIn", portParameter: "" }

        ]

        for( var i=0; i < testFields.length; i++) {
            portModel.append({
                index: i,
                name: testFields[i].name,
                portType: testFields[i].portType,
                portParameter: testFields[i].portParameter
            })
        }
        sort()
    }

    function listModelSort(listModel, compare_function) {
        let indexes = [ ...Array(listModel.count).keys() ]

        indexes.sort( (a, b) => compare_function( listModel.get(a), listModel.get(b) ) )
        let sorted = 0

        while ( sorted < indexes.length && sorted === indexes[sorted] ) sorted++

        if ( sorted === indexes.length ) return

        for ( let i = sorted; i < indexes.length; i++ ) {
            listModel.move( indexes[i], listModel.count - 1, 1 )
            listModel.insert( indexes[i], { } )
        }

        listModel.remove( sorted, indexes.length - sorted )

        //fix stupid indexes
        for( let j = 0; j < portModel.count ; j++) {
            portModel.get(j).index = j
        }
    }

    function sort() {
        if (sortBy === -1) {
            listModelSort( portModel, (a, b) => - a.name.localeCompare(b.name) )
        } else if (sortBy === 1) {
            listModelSort( portModel, (a, b) => a.name.localeCompare(b.name) )
        } else if (sortBy === 2) {
            listModelSort( portModel, (a, b) => - a.portType.localeCompare(b.portType) )
        } else if (sortBy === -2) {
            listModelSort( portModel, (a, b) => a.portType.localeCompare(b.portType) )
        } else if (sortBy === -3) {
            listModelSort( portModel, (a, b) => - a.portParameter.localeCompare(b.portParameter) )
        } else {
            listModelSort( portModel, (a, b) => a.portParameter.localeCompare(b.portParameter) )
        }

        currentIndex = -1
        currentFieldName = ""
    }

    //TODO implement a model sort function

    Connections {
        target: FieldInterface
        function onField_listChanged() {
            //sourseTable.update_model()
        }
    }

    property string currentFieldName: ""
    property int adjustWidth: -10

    //Layout.minimumWidth: 200
    //Layout.minimumHeight: 200
    //Layout.preferredWidth: 400
    //Layout.preferredHeight: 400

    keyNavigationEnabled: true

    model: portModel
    currentIndex: -1
    focus: true
    headerPositioning: ListView.OverlayHeader

    header: Rectangle {
        z: 2
        color: "white"
        implicitWidth: sourseTable.width + sourseTable.adjustWidth
        height: childrenRect.height

        Rectangle {
            id: nameHeaderRect

            anchors.top: parent.top
            anchors.left: parent.left
            width: sourseTable.width * 0.5
            height: nameHeaderText.height + 10

            border.color: "black"
            border.width: 1
            color: "white"

            Text {
                id: nameHeaderText
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 5
                text: qsTr("Port name")
                font.pixelSize: 20
            }
            MouseArea {
                anchors.fill: parent

                onClicked: {
                    if (Math.abs(sourseTable.sortBy) != 1) {
                        sourseTable.sortBy = 1
                    } else {
                        sourseTable.sortBy = -sourseTable.sortBy
                    }
                }
            }
        }

        Rectangle {
            id: portTypeHeaderRect

            anchors.top: parent.top
            anchors.left: nameHeaderRect.right
            height: nameHeaderText.height + 10

            width: sourseTable.width * 0.2
            border.color: "black"
            border.width: 1
            color: "white"

            Text {
                anchors.fill: parent
                anchors.margins: 5
                id: portTypeHeader
                text: qsTr("Type")
                font.pixelSize: 20
            }
            MouseArea {
                anchors.fill: parent

                onClicked: {
                    if (Math.abs(sourseTable.sortBy) != 2) {
                        sourseTable.sortBy = 2
                    } else {
                        sourseTable.sortBy = -sourseTable.sortBy
                    }
                }
            }

        }

        Rectangle {
            id: areaHeaderRect

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: portTypeHeaderRect.right
            border.color: "black"
            border.width: 1
            color: "white"

            height: nameHeaderText.height + 10
            Text {
                anchors.fill: parent
                anchors.margins: 5
                id: areaHeader
                text: qsTr("Parameter")
                font.pixelSize: 20
            }
            MouseArea {
                anchors.fill: parent

                onClicked: {
                    if (Math.abs(sourseTable.sortBy) != 3) {
                        sourseTable.sortBy = 3
                    } else {
                        sourseTable.sortBy = -sourseTable.sortBy
                    }
                }
            }
        }
    }

    spacing: 2

    delegate: Rectangle {
        id: fieldDelegate
        height: 40
        implicitWidth: sourseTable.width + sourseTable.adjustWidth

        required property string portParameter
        required property string portType
        required property string name
        required property int index

        color: ListView.isCurrentItem ? "light blue" : "light grey"


        Text {
            id: fieldName
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.5

            text: fieldDelegate.name
            elide: Text.ElideRight
            font.pixelSize: 18
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            anchors.left: fieldName.right
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.2

            id: portTypeArea
            text: fieldDelegate.portType
            font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
        }

        Loader {
            id: portParameterLoader
            anchors.left: portTypeArea.right
            anchors.leftMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            height: 30
            z: 1

            sourceComponent: {
                if (fieldDelegate.portType === "SERIAL")
                    return comboBoxComponent
                else if (fieldDelegate.portType === "UDP")
                    return textFieldComponent
                else
                    return textComponent
            }
        }

        Component {
            id: textComponent

            Text {
                text: fieldDelegate.portParameter
                font.pixelSize: 16
                verticalAlignment: Text.AlignVCenter
                height: 30
                width: parent.width
            }
        }

        Component {
            id: comboBoxComponent

            ComboBox {
                id: baudRate
                width: parent.width
                height: 30
                font.pixelSize: 16

                model: ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]

                currentIndex: {
                    var index = model.indexOf(fieldDelegate.portParameter)
                    return index !== -1 ? index : 4
                }

                background: Rectangle {
                    color: fieldDelegate.color
                    radius: 3
                    border.color: "#888"
                    border.width: 1
                }

                // Устанавливает скорость из глобального portBaud
                function setBaudFromPortBaud() {
                    var baud = sourseTable.portBaud
                    if (baud && baud !== "") {
                        var idx = model.indexOf(baud)
                        if (idx >= 0 && currentIndex !== idx) {
                            currentIndex = idx
                        }
                    }
                }

                // При создании делегата
                Component.onCompleted: {
                    // Если эта строка является целевой (имя совпадает с portName)
                    if (fieldDelegate.name === sourseTable.portName &&
                        fieldDelegate.portType === "SERIAL" &&
                        sourseTable.currentIndex === fieldDelegate.index) {
                        setBaudFromPortBaud()
                    }
                }

                Connections {
                    target: sourseTable

                    // При изменении выделенной строки
                    function onCurrentIndexChanged() {
                        if (fieldDelegate.name === sourseTable.portName &&
                            fieldDelegate.portType === "SERIAL" &&
                            sourseTable.currentIndex === fieldDelegate.index) {
                            setBaudFromPortBaud()
                        }
                    }

                    // При изменении значения portBaud
                    function onPortBaudChanged() {
                        if (fieldDelegate.name === sourseTable.portName &&
                            fieldDelegate.portType === "SERIAL" &&
                            sourseTable.currentIndex === fieldDelegate.index) {
                            setBaudFromPortBaud()
                        }
                    }
                }

                onActivated: {
                    portModel.setProperty(fieldDelegate.index, "portParameter", model[currentIndex])
                    sourseTable.currentIndex = fieldDelegate.index
                    sourseTable.currentFieldName = fieldDelegate.name
                }
            }
        }

        Component {
            id: textFieldComponent

            TextField {
                id: ipAddressField
                width: parent.width
                height: 30
                font.pixelSize: 16
                text: fieldDelegate.portParameter
                placeholderText: "127.0.0.1:port"

                background: Rectangle {
                    color: fieldDelegate.color
                    radius: 3
                    border.color: "#888"
                    border.width: 1
                }

                // Валидация IP адреса (опционально)
                validator: RegularExpressionValidator {
                    regularExpression: /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):\d{1,5}$/
                }

                onEditingFinished: {
                    // Сохраняем значение при завершении редактирования
                    portModel.setProperty(fieldDelegate.index, "portParameter", text)
                    sourseTable.currentIndex = fieldDelegate.index
                    sourseTable.currentFieldName = fieldDelegate.name
                }

                // Выделяем строку при получении фокуса
                onFocusChanged: {
                    if (focus) {
                        sourseTable.currentIndex = fieldDelegate.index
                        sourseTable.currentFieldName = fieldDelegate.name
                    }
                }
            }
        }

        // MouseArea для левой части (имя и тип)
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: portTypeArea.right
            anchors.bottom: parent.bottom
            z: 0

            onClicked: {
                sourseTable.currentIndex = fieldDelegate.index
                sourseTable.currentFieldName = fieldDelegate.name
            }
        }

        // MouseArea для правой части, только если там Text (не ComboBox и не TextField)
        MouseArea {
            anchors.top: parent.top
            anchors.left: portTypeArea.right
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            enabled: fieldDelegate.portType !== "SERIAL" && fieldDelegate.portType !== "UDP"
            z: 0

            onClicked: {
                sourseTable.currentIndex = fieldDelegate.index
                sourseTable.currentFieldName = fieldDelegate.name
            }
        }
    }
    ListModel {
        id: portModel
    }
}
