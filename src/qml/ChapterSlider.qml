// SPDX-FileCopyrightText: 2023 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kasts

Control {
    id: root

    property alias model: chapters.model

    property int value: AudioManager.position
    property int duration: AudioManager.duration

    function setPlaybackPosition(x) {
        AudioManager.position = (x - handle.width / 2) / (root.width - handle.width) * duration
    }

    Kirigami.Theme.colorSet: Kirigami.Theme.Button
    Kirigami.Theme.inherit: false

    MouseArea {
        anchors.fill: parent
        onReleased: setPlaybackPosition(mouseX)
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: handle.width / 2
        anchors.rightMargin: handle.width / 2
        spacing: 1
        Repeater {
            id: chapters
            delegate: Rectangle {
                // If we're not dragging, use the more precise method using the AudioManager. If we're dragging, this doesn't work because the AudioManager isn't updated while dragging
                property bool isCurrent: dragArea.drag.active ? (x - 1.01 <= handle.centerX && handle.centerX < x + width) : (model.start * 1000 <= AudioManager.position && (model.start + model.duration) * 1000 > AudioManager.position)
                Layout.preferredWidth: model.duration * 1000 / root.duration * (layout.width - chapters.count + 1)
                Layout.preferredHeight: root.height / 2
                Layout.alignment: Qt.AlignVCenter
                radius: height / 2

                z: 1
                color: isCurrent ? Kirigami.Theme.alternateBackgroundColor : Kirigami.Theme.backgroundColor
                border {
                    width: 1
                    color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.3)
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    z: 0
                    ToolTip {
                        text: model.title
                        visible: parent.containsMouse
                    }
                    onReleased: setPlaybackPosition(mouseX + parent.x)
                }
            }
        }
    }

    Rectangle {
        color: Kirigami.Theme.backgroundColor
        visible: chapters.count === 0
        border {
            width: 1
            color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.3)
        }
        width: parent.width
        height: Kirigami.Units.gridUnit / 2
        radius: height / 2
        anchors.centerIn: parent
    }

    Rectangle {
        id: handle

        property int centerX: x + width / 2

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Button

        height: root.height
        width: height
        radius: width / 2
        anchors.verticalCenter: root.verticalCenter
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: dragArea.pressed || dragArea.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.3)
        x: dragArea.drag.active ? 0 : root.value / root.duration * 1000 * (root.width - handle.width)
        z: 2
        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            drag {
                target: handle
                axis: Drag.XAxis
                minimumX: 0
                maximumX: root.width - handle.width
                threshold: 0
            }
            onReleased: setPlaybackPosition(handle.x + handle.width / 2)
        }
    }
}
