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

    required property Entry entry
    required property int entryuid
    property var overlay: undefined

    required property Chapter chapter
    required property string title
    required property string formattedStart
    required property int start

    property bool streamingButtonVisible: entryuid > 0 && entry && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && NetworkConnectionManager.streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entryuid == entryuid)

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
            enabled: root.entryuid > 0 && root.entry && root.entry.enclosure && (root.entry.enclosure.status === Enclosure.Downloaded || root.streamingButtonVisible)
            display: Controls.Button.IconOnly
            onClicked: {
                if (!root.entry.queueStatus) {
                    root.entry.queueStatus = true;
                }
                if (AudioManager.entryuid != root.entryuid) {
                    AudioManager.entryuid = root.entryuid;
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
