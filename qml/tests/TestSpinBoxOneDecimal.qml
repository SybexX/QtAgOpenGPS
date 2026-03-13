import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 800
    height: 600

    MockSettings {
        id: settings
    }
    
    UnitConversion {
        id: utils
    }

    SpinBoxOneDecimal {
        id: test
        
        from: 0 
        to: 24 
        text: "test it"

        value: 2.3

        onValueModified: {
            console.debug("value in metres is " + test.value + " which is local units " + utils.cm_to_unit(test.value))
        }
    }

    Button {
        id: test_button
        anchors.top: test.bottom
        text: "Click to change number"
        onClicked: test.setValue(0.5) //metres
    }
}
