/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kasts

/*
 * This visually mimics the Kirigami.InlineMessage due to the
 * BusyIndicator, which is not supported by the InlineMessage.
 * Consider implementing support for the BusyIndicator within
 * the InlineMessage in the future.
 */
Rectangle {
    id: rootComponent

    required property string text
    property bool showAbortButton: false

    z: 2

    anchors {
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        margins: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : Kirigami.Units.gridUnit * 4
        bottomMargin: bottomMessageSpacing + ( errorNotification.visible ? errorNotification.height + Kirigami.Units.largeSpacing : 0 )
    }

    color: Kirigami.Theme.activeTextColor

    implicitWidth: feedUpdateCountLabel.implicitWidth + 3 * Kirigami.Units.largeSpacing + indicator.implicitWidth + (showAbortButton ? abortButton.implicitWidth + Kirigami.Units.largeSpacing : 0)
    implicitHeight: Math.max(Math.max(indicator.implicitHeight, feedUpdateCountLabel.implicitHeight), showAbortButton ? abortButton.implicitHeight + Kirigami.Units.largeSpacing : 0)

    visible: opacity > 0
    opacity: 0

    radius: Kirigami.Units.smallSpacing / 2

    Rectangle {
        id: bgFillRect

        anchors.fill: parent
        anchors.margins: 1

        color: Kirigami.Theme.backgroundColor

        radius: rootComponent.radius * 0.60
    }

    Rectangle {
        anchors.fill: bgFillRect
        color: rootComponent.color
        opacity: 0.20
        radius: bgFillRect.radius
    }

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        Controls.BusyIndicator {
            id: indicator
            Layout.alignment: Qt.AlignVCenter
        }

        Controls.Label {
            id: feedUpdateCountLabel
            text: rootComponent.text
            color: Kirigami.Theme.textColor
            wrapMode: Text.WordWrap

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }

        Controls.Button {
            id: abortButton
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: showAbortButton
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            Controls.ToolTip.text: i18n("Abort")
            text: i18n("Abort")
            icon.name: "edit-delete-remove"
            onClicked: abortAction();
        }
    }

    Timer {
        id: hideTimer

        interval: 2000
        repeat: false

        onTriggered:
        {
            rootComponent.opacity = 0
        }
    }

    Behavior on opacity {
        NumberAnimation {
            easing.type: Easing.InOutQuad
            duration: Kirigami.Units.longDuration
        }
    }

    function open() {
        hideTimer.stop();
        opacity = 1;
    }

    function close() {
        hideTimer.start();
    }

    // if the abort button is enabled (showAbortButton = true), this function
    // needs to be implemented/overriden to call the correct underlying
    // method/function
    function abortAction() {}
}
