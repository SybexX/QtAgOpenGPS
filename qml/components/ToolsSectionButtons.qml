// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
import QtQuick
import QtQuick.Controls.Fusion
import AOG

Item {
    id: toolsSectionButtons

    width: 600
    height: Math.min(toolsListView.contentHeight, 80)

    // Temporary manual model - list of tool indices
    property var toolsModel: [0]

    ListView {
        id: toolsListView
        anchors.fill: parent
        orientation: Qt.Vertical
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        model: toolsSectionButtons.toolsModel

        delegate: Component {
            SectionButtons {
                width: toolsListView.width
                toolIndex: modelData
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: toolsListView.contentHeight > toolsListView.height
                    ? ScrollBar.AlwaysOn
                    : ScrollBar.AsNeeded
        }
    }
}
