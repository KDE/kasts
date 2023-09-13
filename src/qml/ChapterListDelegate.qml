/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kmediasession

import org.kde.kasts


Kirigami.BasicListItem {
    id: root

    property var entry: undefined
    property var overlay: undefined

    property bool streamingButtonVisible: entry != undefined && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && NetworkConnectionManager.streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entry === entry)

    text: model.title
    subtitle: model.formattedStart
    separatorVisible: true

    leading: ImageWithFallback {
        imageSource: model.chapter.cachedImage
        height: parent.height
        width: height
        fractionalRadius: 1.0 / 8.0
    }

    trailing: Controls.ToolButton {
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
