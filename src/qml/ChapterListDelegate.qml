/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as Delegates
import org.kde.kirigamiaddons.delegates as AddonDelegates
import org.kde.kmediasession

import org.kde.kasts
import org.kde.kasts.settings

AddonDelegates.RoundedItemDelegate {
    id: root

    property var entry: undefined
    property var overlay: undefined

    property bool streamingButtonVisible: entry != undefined && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && NetworkConnectionManager.streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entry === entry)

    contentItem: RowLayout {
        Delegates.IconTitleSubtitle {
            icon.source: model.chapter.cachedImage
            title: model.title
            subtitle: model.formattedStart
            Layout.fillWidth: true
        }

        Controls.ToolButton {
            icon.name: streamingButtonVisible ? "media-playback-cloud" : "media-playback-start"
            text: i18n("Play")
            enabled: entry != undefined && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded || streamingButtonVisible)
            display: Controls.Button.IconOnly
            onClicked: {
                if (!entry.queueStatus) {
                    entry.queueStatus = true;
                }
                if (AudioManager.entry != entry) {
                    AudioManager.entry = entry;
                }
                if (AudioManager.playbackState !== KMediaSession.PlayingState) {
                    AudioManager.play();
                }
                AudioManager.position = start * 1000;
                if (overlay != undefined) {
                    overlay.close();
                }
            }
        }
    }
}
