/**
 * SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects

import org.kde.kirigami as Kirigami

import org.kde.kasts

Flickable {
    id: root

    property bool portrait: (contentZone.height / contentZone.width) > 0.7

    property bool isMaximized: contentY === contentHeight / 2

    property int contentToPlayerSpacing: 0

    boundsBehavior: Flickable.StopAtBounds

    NumberAnimation on contentY {
        id: toOpen
        from: root.contentY
        to: root.contentHeight / 2
        duration: Kirigami.Units.longDuration * 2
        easing.type: Easing.OutCubic
        running: false
    }
    NumberAnimation on contentY {
        id: toClose
        from: root.contentY
        to: 0
        duration: Kirigami.Units.longDuration * 2
        easing.type: Easing.OutCubic
        running: false
    }

    // snap to end
    MouseArea {
        anchors.fill: contentZone
        propagateComposedEvents: true
        onPressed: {
            toOpen.stop();
            toClose.stop();
            propagateComposedEvents = true;
        }
        onReleased: root.resetToBoundsOnFlick()
    }

    function close(): void {
        toClose.restart();
    }

    function resetToBoundsOnFlick(): void {
        if (!atYBeginning || !atYEnd) {
            if (root.verticalVelocity > 0) {
                toOpen.restart();
            } else if (root.verticalVelocity < 0) {
                toClose.restart();
            } else {
                // i.e. when verticalVelocity === 0
                if (contentY > contentHeight / 4) {
                    toOpen.restart();
                } else {
                    toClose.restart();
                }
            }
        }
    }

    function resetToBoundsOnResize(): void {
        if (contentY > contentHeight / 4) {
            contentY = contentHeight / 2;
        } else {
            contentY = 0;
        }
    }

    onMovementStarted: {
        toOpen.stop();
        toClose.stop();
    }
    onFlickStarted: resetToBoundsOnFlick()
    onMovementEnded: resetToBoundsOnFlick()
    onHeightChanged: resetToBoundsOnResize()

    Item {
        id: background
        anchors.fill: contentZone

        // a cover for content underneath the panel
        Rectangle {
            id: coverUnderneath
            anchors.fill: parent

            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            color: Kirigami.Theme.backgroundColor
        }
    }

    ColumnLayout {
        id: contentZone

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: (root.Controls.ApplicationWindow.window as Main).height + (root.Controls.ApplicationWindow.window as Main).miniplayerSize + root.contentToPlayerSpacing
        spacing: 0

        Controls.Control {
            implicitHeight: (root.Controls.ApplicationWindow.window as Main).miniplayerSize + root.contentToPlayerSpacing
            Layout.fillWidth: true
            padding: 0

            background: MultiEffect {
                source: backgroundImage
                anchors.fill: parent
                opacity: GraphicsInfo.api === GraphicsInfo.Software ? 0.05 : 0.2

                brightness: 0.3
                saturation: 2
                contrast: -0.7
                blurMax: 64
                blur: 1.0
                blurEnabled: true
                autoPaddingEnabled: false

                Image {
                    id: backgroundImage
                    source: AudioManager.entry.cachedImage
                    asynchronous: true
                    visible: GraphicsInfo.api === GraphicsInfo.Software
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                }
            }

            MinimizedPlayerControls {
                id: playControlItem
                height: (root.Controls.ApplicationWindow.window as Main).miniplayerSize
                focus: true
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
            }
        }

        MobilePlayerControls {
            id: mobileTrackPlayer
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
