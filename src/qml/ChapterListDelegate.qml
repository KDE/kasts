/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami.delegates as Delegates
import org.kde.kirigamiaddons.delegates as AddonDelegates
import org.kde.kmediasession

import org.kde.kasts

AddonDelegates.RoundedItemDelegate {
    id: root

    property var entry: undefined
    property var overlay: undefined

    required property Chapter chapter
    required property string title
    required property string formattedStart
    required property int start

    property bool streamingButtonVisible: entry != undefined && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && NetworkConnectionManager.streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entry === entry)

    contentItem: RowLayout {
        Delegates.IconTitleSubtitle {
            icon.source: root.chapter ? root.chapter.cachedImage : ""
            title: root.title
            subtitle: root.formattedStart
            Layout.fillWidth: true
        }

        Controls.ToolButton {
            icon.name: root.streamingButtonVisible ? "media-playback-cloud" : "media-playback-start"
            text: i18n("Play")
            enabled: root.entry != undefined && root.entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded || streamingButtonVisible)
            display: Controls.Button.IconOnly
            onClicked: {
                if (!root.entry.queueStatus) {
                    entry.queueStatus = true;
                }
                if (AudioManager.entry != entry) {
                    AudioManager.entry = entry;
                }
                if (AudioManager.playbackState !== KMediaSession.PlayingState) {
                    AudioManager.play();
                }
                AudioManager.position = root.start * 1000;
                if (root.overlay != undefined) {
                    root.overlay.close();
                }
            }
        }
    }
}
