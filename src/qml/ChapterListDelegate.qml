/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kasts.solidextras 1.0
import org.kde.kmediasession 1.0

import org.kde.kasts 1.0


Kirigami.BasicListItem {
    id: root

    property var entry: undefined
    property var overlay: undefined

    property bool streamingAllowed: (NetworkStatus.connectivity !== NetworkStatus.No && (SettingsManager.allowMeteredStreaming || NetworkStatus.metered !== NetworkStatus.Yes))
    property bool streamingButtonVisible: entry != undefined && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded) && streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entry === entry)

    text: model.title
    subtitle: model.formattedStart

    leading: ImageWithFallback {
        imageSource: model.chapter.cachedImage
        height: parent.height
        width: height
        fractionalRadius: 1.0 / 8.0
    }

    trailing: Controls.ToolButton {
        icon.name: streamingButtonVisible ? "" : "media-playback-start"
        icon.source: streamingButtonVisible ? "qrc:/media-playback-start-cloud" : ""
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
