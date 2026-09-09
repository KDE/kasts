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
import org.kde.ki18n
import org.kde.coreaddons

import org.kde.kasts

AddonDelegates.RoundedItemDelegate {
    id: root

    required property int entryuid
    required property string title
    required property int start
    required property string image
    required property int enclosureStatus
    required property bool queueStatus

    property var overlay: undefined

    property bool streamingButtonVisible: entryuid > 0 && (enclosureStatus !== DataTypes.EnclosureStatus.NoEnclosure) && (enclosureStatus !== DataTypes.EnclosureStatus.Downloaded) && NetworkConnectionManager.streamingAllowed && (SettingsManager.prioritizeStreaming || AudioManager.entryuid == entryuid)

    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.onPressAction: {
        clicked();
    }

    contentItem: RowLayout {
        Delegates.IconTitleSubtitle {
            icon.source: root.image
            title: root.title
            subtitle: Format.formatDuration(root.start * 1000)
            Layout.fillWidth: true
        }

        Controls.ToolButton {
            icon.name: root.streamingButtonVisible ? "media-playback-cloud" : "media-playback-start"
            text: KI18n.i18n("Play")
            enabled: root.entryuid > 0 && (root.enclosureStatus === DataTypes.EnclosureStatus.Downloaded || root.streamingButtonVisible)
            display: Controls.Button.IconOnly
            onClicked: root.clicked()
        }
    }

    onClicked: {
        if (!root.queueStatus) {
            DataManager.bulkQueueStatus(true, [root.entryuid]);
        }
        if (AudioManager.entryuid != root.entryuid) {
            AudioManager.entryuid = root.entryuid;
        }
        if (AudioManager.playbackState !== KMediaSession.PlayingState) {
            AudioManager.play();
        }
        AudioManager.position = root.start * 1000;
    }
}
