/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.alligator 1.0

Flickable {
    id: footerBar

    property bool portrait: (contentZone.height/contentZone.width) > 0.7

    property bool isMaximized: contentY == contentHeight / 2

    boundsBehavior: Flickable.StopAtBounds

    NumberAnimation on contentY {
        id: toOpen
        from: contentY
        to: contentHeight / 2
        duration: Kirigami.Units.longDuration * 2
        easing.type: Easing.OutCubic
        running: false
    }
    NumberAnimation on contentY {
        id: toClose
        from: contentY
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
        onReleased: footerBar.resetToBounds()
    }

    function resetToBounds() {
        if (!atYBeginning && !atYEnd) {
            if (footerBar.verticalVelocity > 0) {
                toOpen.restart();
            } else if (footerBar.verticalVelocity < 0) {
                toClose.restart();
            }
        }
    }

    onMovementStarted: {
        toOpen.stop();
        toClose.stop();
    }
    onFlickStarted: resetToBounds()
    onMovementEnded: resetToBounds()

    Item {
        id: background
        anchors.fill: contentZone

        // a cover for content underneath the panel
        Rectangle {
            id: coverUnderneath
            color: Kirigami.Theme.backgroundColor
            anchors.fill: parent
        }

    }

    ColumnLayout {
        id: contentZone

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.height + footerLoader.minimizedSize
        spacing: 0

        MinimizedPlayerControls {
            id: playControlItem

            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 2.5
            Layout.alignment: Qt.AlignTop
            focus: true
        }

        PlayerControls {
            id: mobileTrackPlayer
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Kirigami.Units.largeSpacing * 2
        }
    }
}
