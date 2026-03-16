/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.InlineMessage {
    id: root
    anchors {
        bottom: parent.bottom
        right: parent.right
        left: parent.left
        margins: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : Kirigami.Units.gridUnit * 4
        bottomMargin: (root.Controls.ApplicationWindow.window as Main).bottomMessageSpacing
    }
    type: Kirigami.MessageType.Error
    showCloseButton: true

    actions: [
        Kirigami.Action {
            icon.name: "error"
            text: KI18n.i18n("Show Error Log")
            onTriggered: (root.Controls.ApplicationWindow.window as Main).errorOverlay.open()
        }
    ]

    Timer {
        id: hideTimer

        interval: 10000
        repeat: false

        onTriggered: root.visible = false
    }

    Connections {
        target: ErrorLogModel
        function onNewErrorLogged(error: Error): void {
            root.text = error.description;
            root.visible = true;
            hideTimer.start();
        }
    }
}
