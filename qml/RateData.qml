// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import "components" as Comp

Comp.MoveablePopup {
    id: rcDataPopup
    height: 320 * theme.scaleHeight
    width: 240 * theme.scaleWidth
    visible: false
    modal: false

    property int currentProductIndex: 0
    property var rcModel: RateControl.rcModel

    // Свойства для данных текущего продукта
    property double currentSmoothRate: 0
    property double currentActualRate: 0
    property double currentQuantity: 0
    property double currentSetRate: 0
    property string currentProductName: ""
    property bool currentProductActive: false

    // Свойства для активности кнопок
    property bool product1Active: false
    property bool product2Active: false
    property bool product3Active: false
    property bool product4Active: false

    // Функция для обновления всех данных
    function updateAllData() {
        if (!rcModel) return;

        // Обновляем данные текущего продукта
        if (currentProductIndex >= 0 && currentProductIndex < rcModel.count) {
            var data = rcModel.get(currentProductIndex);
            if (data) {
                currentSmoothRate = data.productSmoothRate || 0;
                currentActualRate = data.productActualRate || 0;
                currentQuantity = data.productQuantity || 0;
                currentSetRate = data.productSetRate || 0;
                currentProductName = data.productName || ("Product " + (currentProductIndex + 1));
                currentProductActive = data.productIsActive || false;
            }
        }

        // Обновляем активности кнопок
        for (var i = 0; i < 4 && i < rcModel.count; i++) {
            var productData = rcModel.get(i);
            var isActive = productData ? productData.productIsActive || false : false;

            switch(i) {
            case 0: product1Active = isActive; break;
            case 1: product2Active = isActive; break;
            case 2: product3Active = isActive; break;
            case 3: product4Active = isActive; break;
            }
        }
    }

    // Подписываемся на изменения модели
    Connections {
        target: rcModel
        enabled: rcDataPopup.visible

        function onDataChanged(topLeft, bottomRight, roles) {
            if (topLeft.row <= currentProductIndex && bottomRight.row >= currentProductIndex) {
                updateAllData();
            }
            updateAllData();
        }

        function onCountChanged() {
            updateAllData()
        }
    }

    // Обновляем данные при изменении индекса
    onCurrentProductIndexChanged: updateAllData()

    // Обновляем при показе
    onVisibleChanged: {
        if (visible) {
            currentProductIndex = 0;
            updateAllData();
        }
    }

    Rectangle {
        id: rcData
        width: parent.width
        height: parent.height
        color: "#4d4d4d"

        Comp.TopLine {
            id: rcDataTopLine
            onBtnCloseClicked: rcDataPopup.visible = false
            titleText: currentProductName
        }

        // ButtonGroup для кнопок продуктов
        ButtonGroup {
            id: productButtonGroup
            buttons: [product1, product2, product3, product4]
        }

        // Основная табличная структура
        GridLayout {
            id: mainGrid
            anchors.top: rcDataTopLine.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 5 * theme.scaleWidth

            columns: 4
            rows: 5
            rowSpacing: 5 * theme.scaleHeight
            columnSpacing: 5 * theme.scaleWidth

            // Строка 0: 4 кнопки продуктов
            Comp.IconButtonColor {
                id: product1
                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                checkable: true
                colorChecked: product1Active?"green":"grey"
                icon.source: prefix + "/images/ratec1.png"
                checked: currentProductIndex === 0
                enabled: SettingsManager.rate_confProduct0[2]
                onClicked: currentProductIndex = 0
            }

            Comp.IconButtonColor {
                id: product2
                Layout.row: 0
                Layout.column: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                checkable: true
                colorChecked: product2Active?"green":"grey"
                icon.source: prefix + "/images/ratec2.png"
                checked: currentProductIndex === 1
                enabled: SettingsManager.rate_confProduct1[2]
                onClicked: currentProductIndex = 1
            }

            Comp.IconButtonColor {
                id: product3
                Layout.row: 0
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                checkable: true
                colorChecked: product3Active?"green":"grey"
                icon.source: prefix + "/images/ratec3.png"
                checked: currentProductIndex === 2
                enabled: SettingsManager.rate_confProduct2[2]
                onClicked: currentProductIndex = 2
            }

            Comp.IconButtonColor {
                id: product4
                Layout.row: 0
                Layout.column: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                checkable: true
                colorChecked: product4Active?"green":"grey"
                icon.source: prefix + "/images/ratec4.png"
                checked: currentProductIndex === 3
                enabled: SettingsManager.rate_confProduct3[2]
                onClicked: currentProductIndex = 3
            }

            // Строка 1: Уставка
            // Текст "Уставка" - занимает 2 колонки, текст по центру
            Rectangle {
                Layout.row: 1
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                color: "transparent"

                Text {
                    anchors.fill: parent
                    text: qsTr("Уставка")
                    font.pixelSize: 18 * theme.scaleHeight
                    color: aogInterface.backgroundColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Поле уставки - занимает 2 колонки, текст по центру
            Rectangle {
                id: target
                Layout.row: 1
                Layout.column: 2
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                property bool clicked: false
                color: aogInterface.backgroundColor
                border.color: "black"
                radius: 10

                Text {
                    anchors.fill: parent
                    text: Math.round(currentSetRate)
                    font.pixelSize: 18 * theme.scaleHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: target.clicked = !target.clicked
                }
            }

            // Строка 2: Расход
            // Текст "Расход" - занимает 2 колонки, текст по центру
            Rectangle {
                Layout.row: 2
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                color: "transparent"

                Text {
                    anchors.fill: parent
                    text: qsTr("Расход")
                    font.pixelSize: 18 * theme.scaleHeight
                    color: aogInterface.backgroundColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Поле расхода - занимает 2 колонки, текст по центру
            Rectangle {
                id: applied
                Layout.row: 2
                Layout.column: 2
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                property bool clicked: false
                color: aogInterface.backgroundColor
                border.color: "black"
                radius: 10

                Text {
                    anchors.fill: parent
                    text: applied.clicked ? Math.round(currentActualRate) : Math.round(currentSmoothRate)
                    font.pixelSize: 18 * theme.scaleHeight
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: applied.clicked = !applied.clicked
                }
            }

            // Строка 3: Применено
            // Текст "Применено" - занимает 2 колонки, текст по центру
            Rectangle {
                Layout.row: 3
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                color: "transparent"

                Text {
                    anchors.fill: parent
                    text: qsTr("Применено")
                    font.pixelSize: 18 * theme.scaleHeight
                    color: aogInterface.backgroundColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Поле примененного количества - занимает 2 колонки, текст по центру
            Rectangle {
                id: qtty
                Layout.row: 3
                Layout.column: 2
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                property bool clicked: false
                color: aogInterface.backgroundColor
                border.color: "black"
                radius: 10

                Text {
                    anchors.fill: parent
                    text: qtty.clicked ? Math.round(currentQuantity) : "0 л"
                    font.pixelSize: 18 * theme.scaleHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: qtty.clicked = !qtty.clicked
                }
            }

            // Строка 4: Нижняя панель
            // Имя продукта - 2 колонки, текст по центру
            Rectangle {
                Layout.row: 4
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                color: "transparent"

                Text {
                    anchors.fill: parent
                    text: " "
                    font.pixelSize: 18 * theme.scaleHeight
                    color: aogInterface.backgroundColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }

            // Кнопка уменьшения - 1 колонка
            Comp.IconButtonColor {
                id: rateDown
                Layout.row: 4
                Layout.column: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                icon.source: prefix + "/images/ratec-down.png"
                enabled: currentProductIndex >= 0 && currentProductIndex < 4 && currentProductActive
                onClicked: RateControl.decreaseSetRate(currentProductIndex, 10);
            }

            // Кнопка увеличения - 1 колонка
            Comp.IconButtonColor {
                id: rateUp
                Layout.row: 4
                Layout.column: 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 40 * theme.scaleHeight
                icon.source: prefix + "/images/ratec-up.png"
                enabled: currentProductIndex >= 0 && currentProductIndex < 4 && currentProductActive
                onClicked: RateControl.increaseSetRate(currentProductIndex, 10);
            }
        }
    }
}
