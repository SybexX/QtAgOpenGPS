// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// RateControl config page
import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

import ".."
import "../components"

Rectangle{
    id: configRateControl
    anchors.fill: parent
    color: aogInterface.backgroundColor
    visible: false

    onVisibleChanged: {
        if (visible) loadCurrentProduct();
    }

    onProdIDChanged: {
    if (ratePWMUP.enabled)
    {manPWM.text = qsTr("Manual PWM 0");}
    }

    property int prodID: 0;

    property var products: [
        {
            id: 0,
            settings: SettingsManager.rate_confProduct0,
            name: SettingsManager.rate_productName0,
        },
        {
            id: 1,
            settings: SettingsManager.rate_confProduct1,
            name: SettingsManager.rate_productName1,
        },
        {
            id: 2,
            settings: SettingsManager.rate_confProduct2,
            name: SettingsManager.rate_productName2,
        },
        {
            id: 3,
            settings: SettingsManager.rate_confProduct3,
            name: SettingsManager.rate_productName3,
        }
    ]

    function loadCurrentProduct() {
        var product = products[prodID];

        moduleID.value = product.settings[0];
        prodDensityBox.value = product.settings[1];
        cboxIsRateControlOn.checked = product.settings[2] > 0;
        rateKP.value = product.settings[3];
        rateKI.value = product.settings[4];
        rateKD.value = product.settings[5];
        rateMinPWM.value = product.settings[6];
        rateMaxPWM.value = product.settings[7];
        ratePIDscale.value = product.settings[8];
        rateSensor.value = product.settings[9];
        setRate.value = product.settings[10];
        cboxRateMode.currentIndex = product.settings[11];
        cboxRateControlType.currentIndex = product.settings[12];
        cboxRateCoverageUnits.currentIndex = product.settings[13];
        minSpeed.value = product.settings[14];
        minUPM.value = product.settings[15];

        mandatory.visible = false;
    }

    function saveCurrentProduct() {
        var product = products[prodID];

        product.settings[0] = moduleID.value;
        product.settings[1] = prodDensityBox.value;
        product.settings[2] = cboxIsRateControlOn.checked ? 1 : 0;
        product.settings[3] = rateKP.value;
        product.settings[4] = rateKI.value;
        product.settings[5] = rateKD.value;
        product.settings[6] = rateMinPWM.value;
        product.settings[7] = rateMaxPWM.value;
        product.settings[8] = ratePIDscale.value;
        product.settings[9] = rateSensor.value;
        product.settings[10] = setRate.value;
        product.settings[11] = Number(cboxRateMode.currentIndex);
        product.settings[12] = Number(cboxRateControlType.currentIndex);
        product.settings[13] = Number(cboxRateCoverageUnits.currentIndex);
        product.settings[14] = minSpeed.value;
        product.settings[15] = minUPM.value;

        SettingsManager.rate_productName0 = products[0].name;
        SettingsManager.rate_productName1 = products[1].name;
        SettingsManager.rate_productName2 = products[2].name;
        SettingsManager.rate_productName3 = products[3].name;

        mandatory.visible = false;
        ModuleComm.modulesSend242()
    }

    function getCurrentPWM() {
        if (!RateControl.rcModel || prodID < 0 || prodID >= RateControl.rcModel.count)
            return 0;

        var data = RateControl.rcModel.get(prodID);
        return data ? data.productPWM || 0 : 0;
    }

    Connections {
        target: RateControl.rcModel
        onDataChanged: {
            if (topLeft.row <= prodID && bottomRight.row >= prodID) {
                manPWM.text = qsTr("Manual PWM: ") + getCurrentPWM();
            }
        }

        onPwmChanged: {
            manPWM.text = qsTr("Manual PWM: ") + getCurrentPWM();

        }
    }

    Label{
        id: top
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 10
        font.bold: true
        text: qsTr("Rate Control");
    }

    RowLayout {
        id: topRateButtons
        anchors.right: parent.right
        anchors.rightMargin: 20 * theme.scaleWidth
        anchors.left: parent.left
        anchors.leftMargin: 20 * theme.scaleWidth
        anchors.top: top.bottom
        anchors.bottomMargin: 10 * theme.scaleHeight
        height: children.height

        IconButton {
            id: rateProductPrev
            Layout.alignment: Qt.AlignVCenter
            icon.source: prefix + "/images/ArrowLeft.png"
            onClicked: {
                prodID = (prodID - 1 + products.length) % products.length;
                loadCurrentProduct();
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.preferredWidth: parent.width * 0.5
            Layout.alignment: Qt.AlignCenter
            height: childrenRect.height

            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: productName.height + 8
                color: "transparent"

                TextField {
                    id: productName
                    anchors.centerIn: parent
                    width: parent.width
                    selectByMouse: true
                    placeholderText: qsTr("Product Name")
                    text: products[prodID].name
                    onTextChanged: products[prodID].name = text
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        IconButtonTransparent {
            id: rateProductNext
            Layout.alignment: Qt.AlignVCenter
            icon.source: prefix + "/images/ArrowRight.png"
            onClicked: {
                prodID = (prodID + 1) % products.length;
                loadCurrentProduct();
            }
        }
    }
    Rectangle{
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topRateButtons.bottom
        anchors.bottom: btnPinsSave.top
        anchors.margins: 20 * theme.scaleWidth
        color: "transparent"

    GridLayout {
        id: mainGrid
        anchors.fill: parent
        columns: 5
        rows: 4
        columnSpacing: 10 * theme.scaleWidth
        rowSpacing: 15 * theme.scaleHeight

        Item {
            Layout.row: 0
            Layout.column: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: moduleID
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Module ID: ")
            }
        }

        Item {
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateKP
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("PID KP: ")
            }
        }

        Item {
            Layout.row: 0
            Layout.column: 2
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateKI
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("PID KI: ")
            }
        }

        Item {
            Layout.row: 0
            Layout.column: 3
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateKD
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("PID KD: ")
            }
        }

        Item {
            Layout.row: 0
            Layout.column: 4
            Layout.fillWidth: true
            Layout.fillHeight: true

            ComboBoxCustomized {
                id: cboxRateControlType
                anchors.centerIn: parent
                enabled: cboxIsRateControlOn.checked
                editable: false
                model: ListModel {
                    ListElement { text: qsTr("Standard"); }
                    ListElement { text: qsTr("Combo Close"); }
                    ListElement { text: qsTr("Motor"); }
                    ListElement { text: qsTr("Combo Timed"); }
                    ListElement { text: qsTr("Fan"); }
                }
                text: qsTr("Control Type")
                onActivated: mandatory.visible = true
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: prodDensityBox
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("ProdDensity: ")
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateMinPWM
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Min PWM")
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 2
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateMaxPWM
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Max PWM")
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 3
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: ratePIDscale
                anchors.centerIn: parent
                from: 0
                to: 255
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("PID scale")
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 4
            Layout.fillWidth: true
            Layout.fillHeight: true

            ComboBoxCustomized {
                id: cboxRateMode
                anchors.centerIn: parent
                enabled: cboxIsRateControlOn.checked
                editable: false
                model: ListModel {
                    ListElement { text: qsTr("Section UPM"); }
                    ListElement { text: qsTr("Constant UPM"); }
                    ListElement { text: qsTr("Applied rate"); }
                    ListElement { text: qsTr("Target rate"); }
                }
                text: qsTr("Mode")
                onActivated: mandatory.visible = true
            }
        }

        Item {
            Layout.row: 2
            Layout.column: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: setRate
                anchors.centerIn: parent
                from: 0
                to: 1000
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Rate SET")
            }
        }

        Item {
            Layout.row: 2
            Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: rateSensor
                anchors.centerIn: parent
                from: 0
                to: 1000
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Sensor Count")
            }
        }

        Item {
            Layout.row: 2
            Layout.column: 2
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: minSpeed
                anchors.centerIn: parent
                from: 0
                to: 10
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Min Speed")
            }
        }

        Item {
            Layout.row: 2
            Layout.column: 3
            Layout.fillWidth: true
            Layout.fillHeight: true

            SpinBoxCustomized {
                id: minUPM
                anchors.centerIn: parent
                from: 0
                to: 100
                editable: true
                enabled: cboxIsRateControlOn.checked
                onValueModified: mandatory.visible = true
                text: qsTr("Min UPM")
            }
        }

        Item {
            Layout.row: 2
            Layout.column: 4
            Layout.fillWidth: true
            Layout.fillHeight: true

            ComboBoxCustomized {
                id: cboxRateCoverageUnits
                anchors.centerIn: parent
                enabled: cboxIsRateControlOn.checked
                editable: false
                model: ListModel {
                    ListElement { text: qsTr("Acres"); }
                    ListElement { text: qsTr("Hectare"); }
                    ListElement { text: qsTr("Minutes"); }
                    ListElement { text: qsTr("Hours"); }
                }
                text: qsTr("Coverage Units")
                onActivated: mandatory.visible = true
            }
        }
    }
}

    IconButtonTransparent{
        id: back
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        enabled: cboxIsRateControlOn.checked
        icon.source: prefix + "/images/back-button.png"
        onClicked: {
            rateMinPWM.boundValue = 100;
            rateMaxPWM.boundValue = 255;
            rateKP.boundValue = 10;
            rateKI.boundValue = 0;
            rateKD.boundValue = 0;
            ratePIDscale.boundValue = 0;
            rateSensor.boundValue = 600;
            setRate.boundValue = 100;
            minSpeed.boundValue = 0;
            minUPM.boundValue = 0;
        }
    }

    IconButtonTransparent{
        id: loadSetBlockage
        anchors.bottom: parent.bottom
        anchors.left: back.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        enabled: cboxIsRateControlOn.checked
        icon.source: prefix + "/images/UpArrow64.png"
        onClicked: {
            loadCurrentProduct();
            mandatory.visible = true;
        }
    }

    IconButtonColor{
        id: cboxIsRateControlOn
        height: loadSetBlockage.height
        anchors.bottom: parent.bottom
        anchors.left: loadSetBlockage.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/SwitchOff.png"
        iconChecked: prefix + "/images/SwitchOn.png"
        checkable: true
        onClicked: mandatory.visible = true
    }

    IconButtonTransparent{
        id: ratePWMauto
        height: loadSetBlockage.height
        anchors.bottom: parent.bottom
        anchors.left: cboxIsRateControlOn.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/AutoStop.png"
        onClicked: RateControl.rate_pwm_auto(prodID)
        enabled: cboxIsRateControlOn.checked
    }

    IconButtonTransparent{
        id: ratePWMUP
        height: loadSetBlockage.height
        anchors.bottom: parent.bottom
        anchors.left: ratePWMauto.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/UpArrow64.png"
        onClicked: RateControl.rate_bump(true, prodID)
        enabled: cboxIsRateControlOn.checked

        Label{
            id: manPWM
            anchors.bottom: parent.top
            anchors.bottomMargin: 20 * theme.scaleHeight
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Manual PWM ") + getCurrentPWM()
        }
    }

    IconButtonTransparent{
        id: ratePWMDN
        height: loadSetBlockage.height
        anchors.bottom: parent.bottom
        anchors.left: ratePWMUP.right
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        icon.source: prefix + "/images/DnArrow64.png"
        onClicked: RateControl.rate_bump(false, prodID)
        enabled: cboxIsRateControlOn.checked
    }

    IconButtonTransparent{
        id: btnPinsSave
        anchors.right: mandatory.left
        anchors.topMargin: 20 * theme.scaleHeight
        anchors.bottomMargin: 20 * theme.scaleHeight
        anchors.rightMargin: 20 * theme.scaleHeight
        anchors.leftMargin: 20 * theme.scaleHeight
        anchors.bottom: parent.bottom
        icon.source: prefix + "/images/ToolAcceptChange.png"
        Text{
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.left
            anchors.rightMargin: 5
            text: qsTr("Send + Save");
        }
        onClicked: saveCurrentProduct()
    }

    Image{
        id: mandatory
        anchors.right: parent.right
        anchors.verticalCenter: btnPinsSave.verticalCenter
        Layout.preferredWidth: 30 * theme.scaleWidth
        Layout.preferredHeight: 30 * theme.scaleHeight
        visible: false
        source: prefix + "/images/Config/ConSt_Mandatory.png"
        fillMode: Image.PreserveAspectFit
    }
}
