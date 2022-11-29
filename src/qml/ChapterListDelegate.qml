/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kasts.solidextras 1.0

import org.kde.kasts 1.0

Kirigami.SwipeListItem {
    id: root
    alwaysVisibleActions: true

    property var entry: undefined
    property var overlay: undefined

    property bool streamingAllowed: (NetworkStatus.connectivity !== NetworkStatus.No && (SettingsManager.allowMeteredStreaming || NetworkStatus.metered !== NetworkStatus.Yes))
    property bool streamingButtonVisible: entry != undefined && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entry === entry)

    contentItem: ColumnLayout {
        Controls.Label {
            Layout.fillWidth: true
            text: title
            elide: Text.ElideRight
        }
        Controls.Label {
            Layout.fillWidth: true
            opacity: 0.7
            font: Kirigami.Theme.smallFont
            text: formattedStart
            elide: Text.ElideRight
         }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Play")
            icon.name: streamingButtonVisible ? ":/media-playback-start-cloud" : "media-playback-start"
            enabled: entry != undefined && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded || streamingButtonVisible)
            onTriggered: {
                if (!entry.queueStatus) {
                    entry.queueStatus = true;
                }
                if (AudioManager.entry != entry) {
                    AudioManager.entry = entry;
                }
                if (AudioManager.playbackState !== Audio.PlayingState) {
                    AudioManager.play();
                }
                AudioManager.position = start * 1000;
                if (overlay != undefined) {
                    overlay.close();
                }
            }
        }
    ]
}
