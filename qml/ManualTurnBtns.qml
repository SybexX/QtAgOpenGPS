import QtQuick
import AOG
//import Settings
import "components" as Comp

Grid{
    id: gridTurnBtns //lateral turn and manual Uturn
    spacing: 10
    rows: 2
    columns: 2
    flow: Grid.LeftToRight
    visible: MainWindowState.isBtnAutoSteerOn
    Comp.IconButtonTransparent{
        implicitHeight: 65 * theme.scaleHeight
        implicitWidth: 85 * theme.scaleWidth
        imageFillMode: Image.Stretch
        // Threading Phase 1: U-turn feature visibility
        visible: SettingsManager.feature_isYouTurnOn
        icon.source: prefix + "/images/qtSpecific/z_TurnManualL.png"
        onClicked: {
            // Threading Phase 1: Check speed limit for manual operations
            if (SettingsManager.as_functionSpeedLimit > VehicleInterface.avgSpeed) {
                console.debug("limit ", SettingsManager.as_functionSpeedLimit, " speed ", VehicleInterface.avgSpeed)
                Backend.yt.manualUTurn(false) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            } else
                timedMessage.addMessage(2000,qsTr("Too Fast"), qsTr("Slow down below") + " " +
                                        Utils.speed_to_unit_string(SettingsManager.as_functionSpeedLimit,1) + " " + Utils.speed_unit())
        }

    }

    Comp.IconButtonTransparent{
        implicitHeight: 65 * theme.scaleHeight
        implicitWidth: 85 * theme.scaleWidth
        imageFillMode: Image.Stretch
        // Threading Phase 1: U-turn feature visibility
        visible: SettingsManager.feature_isYouTurnOn
        icon.source: prefix + "/images/qtSpecific/z_TurnManualR.png"
        onClicked: {
            if (SettingsManager.as_functionSpeedLimit > VehicleInterface.avgSpeed) // Threading Phase 1: Function speed limit check
                Backend.yt.manualUTurn(true) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            else
                timedMessage.addMessage(2000,qsTr("Too Fast"), qsTr("Slow down below") + " " +
                                        Utils.speed_to_unit_string(SettingsManager.as_functionSpeedLimit,1) + " " + Utils.speed_unit())
        }
    }
    Comp.IconButtonTransparent{
        implicitHeight: 65 * theme.scaleHeight
        implicitWidth: 85 * theme.scaleWidth
        imageFillMode: Image.Stretch
        icon.source: prefix + "/images/qtSpecific/z_LateralManualL.png"
        // Threading Phase 1: Lateral turn feature visibility
        visible: SettingsManager.feature_isLateralOn
        onClicked: {
            if (SettingsManager.as_functionSpeedLimit > VehicleInterface.avgSpeed) // Threading Phase 1: Function speed limit check
                Backend.yt.lateral(false) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            else
                timedMessage.addMessage(2000,qsTr("Too Fast"), qsTr("Slow down below") + " " +
                                        Utils.speed_to_unit_string(SettingsManager.as_functionSpeedLimit,1) + " " + Utils.speed_unit())
        }
    }
    Comp.IconButtonTransparent{
        implicitHeight: 65 * theme.scaleHeight
        implicitWidth: 85 * theme.scaleWidth
        imageFillMode: Image.Stretch
        // Threading Phase 1: Lateral turn feature visibility
        visible: SettingsManager.feature_isLateralOn
        icon.source: prefix + "/images/qtSpecific/z_LateralManualR.png"
        onClicked: {
            if (SettingsManager.as_functionSpeedLimit > VehicleInterface.avgSpeed) // Threading Phase 1: Function speed limit check
                Backend.yt.lateral(true) // Qt 6.8 MODERN: Direct Q_INVOKABLE call
            else
                timedMessage.addMessage(2000,qsTr("Too Fast"), qsTr("Slow down below") + " " +
                                        Utils.speed_to_unit_string(SettingsManager.as_functionSpeedLimit,1) + " " + Utils.speed_unit())
        }
    }
}
