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

import org.kde.kasts 1.0

Kirigami.SwipeListItem {
    alwaysVisibleActions: true

    property var entry: undefined

    ColumnLayout {
        Controls.Label {
            text: title
        }
        Controls.Label {
            opacity: 0.7
            font: Kirigami.Theme.smallFont
            text: formattedStart
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Play")
            icon.name: "media-playback-start"
            enabled: entry != undefined && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
            onTriggered: {
                if (AudioManager.entry != entry) {
                    AudioManager.entry = entry;
                }
                if (AudioManager.playbbackState !== Audio.PlayingState) {
                    AudioManager.play();
                }
                AudioManager.position = start * 1000;
            }
        }
    ]

    //onClicked: {
    //    AudioManager.position = start * 1000;
    //}
}
