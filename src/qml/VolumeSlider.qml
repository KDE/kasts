/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kmediasession

import org.kde.kasts

Controls.Slider {
    id: volumeSlider
    height: Kirigami.Units.gridUnit * 7
    Layout.alignment: Qt.AlignHCenter
    Layout.preferredHeight: height
    Layout.maximumHeight: height
    Layout.topMargin: Kirigami.Units.smallSpacing
    orientation: Qt.Vertical
    padding: 0
    enabled: !AudioManager.muted && AudioManager.playbackState != AudioManager.StoppedState && AudioManager.canPlay
    from: 0
    to: 100
    value: AudioManager.volume
    onMoved: AudioManager.volume = value

    onPressedChanged: {
        tooltip.delay = pressed ? 0 : Kirigami.Units.toolTipDelay
    }

    readonly property int wheelEffect: 5

    Controls.ToolTip {
        id: tooltip
        x: volumeSlider.x - volumeSlider.width / 2 - width
        y: volumeSlider.visualPosition * volumeSlider.height - height / 2
        visible: volumeSlider.pressed || sliderMouseArea.containsMouse
        // delay is actually handled in volumeSlider.onPressedChanged, because property bindings aren't immediate
        delay: volumeSlider.pressed ? 0 : Kirigami.Units.toolTipDelay
        closePolicy: Controls.Popup.NoAutoClose
        timeout: -1
        text: i18nc("Volume as a percentage", "%1%", Math.round(volumeSlider.value))
    }

    MouseArea {
        id: sliderMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        onWheel: (wheel) => {
            // Can't use Slider's built-in increase() and decrease() functions here
            // since they go in increments of 0.1 when the slider's stepSize is not
            // defined, which is much too slow. And we don't define a stepSize for
            // the slider because if we do, it gets gets tickmarks which look ugly.
            if (wheel.angleDelta.y > 0) {
                // Increase volume
                volumeSlider.value = Math.min(volumeSlider.to, volumeSlider.value + volumeSlider.wheelEffect);

            } else {
                // Decrease volume
                volumeSlider.value = Math.max(volumeSlider.from, volumeSlider.value - volumeSlider.wheelEffect);
            }
            AudioManager.volume = volumeSlider.value
        }
    }
}

