// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Sim controller panel on main screen
import QtQuick
import QtQuick.Controls.Fusion
import AOG
//import Settings
import "components" as Comp
import "../"

Rectangle{
    color: BoundaryInterface.isOutOfBounds ? "darksalmon" : "gray"
    height: 60 * theme.scaleHeight
    width: 650 * theme.scaleWidth
    z: 100
	function changedSteerDir(isRight){
		if(isRight){
			steerSlider.value = steerSlider.value + 10
		}else{
			steerSlider.value = steerSlider.value - 10
		}
	}

    Row{
        spacing: 4 * theme.scaleWidth
        width: childrenRect.width
        height: 50 * theme.scaleHeight
        anchors.centerIn: parent
        Button{
			id: resetButton
            text: qsTr("Reset")
            font.pointSize: 11
            height: parent.height
            width: 65 * theme.scaleWidth
            onClicked: SimInterface.reset()
        }
        Button{
            text: SimInterface.steerAngleActual
            font.pointSize: 11
            height: parent.height
            width: 65 * theme.scaleWidth
            onClicked: SimInterface.steerAngle = 0;
        }
        Comp.SliderCustomized {
            id: steerSlider
            objectName: "simSteer"
            multiplicationValue: 10
            height: 50 * theme.scaleHeight
			width: 200 * theme.scaleWidth
            from: 0
            to: 600
            value: SimInterface.steerAngle * 10 + 300
            onValueChanged: {
                SimInterface.steerAngle = (value - 300) / 10
            }
        }
        Comp.IconButtonTransparent{
            height: parent.height
            width: 65 * theme.scaleWidth
            icon.source: prefix + "/images/DnArrow64.png"
            onClicked: SimInterface.slowdown();
        }
        Comp.IconButtonTransparent{
            height: parent.height
            width: 65 * theme.scaleWidth
            icon.source: prefix + "/images/AutoStop.png"
            onClicked: SimInterface.stop()
        }
        Comp.IconButtonTransparent{
            height: parent.height
            width: 65 * theme.scaleWidth
            icon.source: prefix + "/images/UpArrow64.png"
            onClicked: SimInterface.speedup()
        }
        Comp.IconButtonTransparent{
            height: parent.height
            width: 65 * theme.scaleWidth
            icon.source: prefix + "/images/YouTurn80.png"
            onClicked: {
                SimInterface.rotate()
                MainWindowState.isBtnAutoSteerOn = false; 
            }
        }
    }
}
