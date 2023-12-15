/**
 * SPDX-FileCopyrightText: 2023 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

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

    Kirigami.Theme.colorSet: Kirigami.Theme.Button
    Kirigami.Theme.inherit: false

    implicitHeight: handle.height
    implicitWidth: 200

    // metrics used by the default font
    property var fontMetrics: FontMetrics {
        property real fullWidthCharWidth: fontMetrics.tightBoundingRect('＿').width
    }

    // align with the Slider implementations in the major styles
    readonly property bool desktopStyle: styleName === "org.kde.desktop"
    readonly property color inactiveGrooveColor: desktopStyle ? Kirigami.ColorUtils.scaleColor(inactiveGrooveBorderColor, {alpha: -50}) : Kirigami.Theme.backgroundColor
    readonly property color activeGrooveColor: desktopStyle ? Kirigami.ColorUtils.scaleColor(activeGrooveBorderColor, {alpha: -50}) : Kirigami.Theme.alternateBackgroundColor
    readonly property color inactiveGrooveBorderColor: desktopStyle ? Kirigami.ColorUtils.scaleColor(Kirigami.Theme.textColor, {alpha: -80}) : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.3)
    readonly property color activeGrooveBorderColor: desktopStyle ? Kirigami.Theme.highlightColor : Kirigami.Theme.focusColor
    readonly property color handleColor: Kirigami.Theme.backgroundColor
    readonly property color handleBorderColor: dragArea.pressed || dragArea.containsMouse ? Kirigami.Theme.focusColor : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.4)

    readonly property int grooveSize: {
        if (desktopStyle) {
            return 6;
        } else {
            let h = Math.floor(fontMetrics.height / 3);
            h += h % 2;
            return h;
        }
    }
    readonly property int handleSize: desktopStyle ? 20 : fontMetrics.height

    function setPlaybackPosition(x) {
        AudioManager.position = (Math.max(handle.width / 2, Math.min(x, root.width - handle.width / 2)) - handle.width / 2) / (root.width - handle.width) * duration
    }

    MouseArea {
        anchors.fill: parent
        onReleased: {
            setPlaybackPosition(mouseX)
        }
        // TODO: handle scrollwheel
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: handle.width / 2
        anchors.rightMargin: handle.width / 2
        spacing: 0
        Repeater {
            id: chapters
            delegate: Rectangle {
                // If we're not dragging, use the more precise method using the AudioManager. If we're dragging, this doesn't work because the AudioManager isn't updated while dragging
                readonly property bool isCurrent: dragArea.drag.active ? (x - 1.01 <= handle.centerX && handle.centerX < x + width) : (model.start * 1000 <= AudioManager.position && (model.start + model.duration) * 1000 > AudioManager.position)
                readonly property bool isPrevious: dragArea.drag.active ? (x + width < handle.centerX) : ((model.start + model.duration) * 1000 < AudioManager.position)
                Layout.preferredWidth: model.duration * 1000 / root.duration * (layout.width - chapters.count + 1)
                Layout.preferredHeight: grooveSize
                Layout.alignment: Qt.AlignVCenter
                radius: height / 2

                z: 1
                color: inactiveGrooveColor
                border {
                    width: 1
                    color: inactiveGrooveBorderColor
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    z: 0
                    ToolTip {
                        text: model.title
                        visible: parent.containsMouse
                    }
                    onReleased: {
                        setPlaybackPosition(mouseX + parent.x + handle.width / 2)
                    }
                }
                Rectangle {
                    visible: isCurrent || isPrevious
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: isCurrent ? handle.centerX - parent.x : parent.width
                    radius: parent.height / 2
                    color: activeGrooveColor
                    border {
                        width: 1
                        color: activeGrooveBorderColor
                    }
                }
            }
        }
    }

    Rectangle {
        color: inactiveGrooveColor
        visible: chapters.count === 0
        border {
            width: 1
            color: inactiveGrooveBorderColor
        }
        width: parent.width - handle.width
        height: grooveSize
        radius: height / 2
        anchors.centerIn: parent
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: handle.centerX - parent.x
            radius: parent.height / 2
            color: activeGrooveColor
            border {
                width: 1
                color: activeGrooveBorderColor
            }
        }
    }

    Rectangle {
        id: handle

        property int centerX: x + width / 2

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Button

        height: root.handleSize
        width: height
        radius: width / 2
        anchors.verticalCenter: root.verticalCenter
        color: handleColor
        border.width: 1
        border.color: handleBorderColor
        x: dragArea.drag.active ? 0 : root.value / root.duration * (root.width - handle.width)
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
