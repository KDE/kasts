/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls

import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.InlineMessage {
    id: inlineMessage
    anchors {
        bottom: parent.bottom
        right: parent.right
        left: parent.left
        margins: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : Kirigami.Units.gridUnit * 4
        bottomMargin: bottomMessageSpacing
    }
    type: Kirigami.MessageType.Error
    showCloseButton: true

    actions: [
        Kirigami.Action {
            icon.name: "error"
            text: i18n("Show Error Log")
            onTriggered: errorOverlay.open()
        }
    ]

    Timer {
        id: hideTimer

        interval: 10000
        repeat: false

        onTriggered: inlineMessage.visible = false;
    }

    Connections {
        target: ErrorLogModel
        function onNewErrorLogged(error) {
            inlineMessage.text = error.description;
            inlineMessage.visible = true;
            hideTimer.start();
        }
    }
}
