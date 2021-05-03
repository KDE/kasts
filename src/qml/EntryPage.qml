/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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

Kirigami.ScrollablePage {
    id: page

    property QtObject entry

    title: entry.title

    padding: 0  // needed to get the inline header to fill the page

    // This function is needed to close the EntryPage if it is opened over the
    // QueuePage when the episode is removed from the queue (e.g. when the
    // episode finishes).
    Connections {
        target: entry
        function onQueueStatusChanged() {
            if (!entry.queueStatus) {
                // this entry has just been removed from the queue
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).lastEntry) {
                        if (pageStack.get(0).lastEntry === entry.id) {
                            // if this EntryPage was open, then close it
                            pageStack.pop()
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        GenericHeader {
            id: infoHeader
            Layout.fillWidth: true
            image: entry.cachedImage
            title: entry.title
            subtitle: entry.feed.name
        }

        Controls.Label {
            id: textLabel
            Layout.margins: Kirigami.Units.gridUnit
            text: page.entry.content
            baseUrl: page.entry.baseUrl
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.fillHeight: true
            onLinkActivated: Qt.openUrlExternally(link)
            onWidthChanged: { text = entry.adjustedContent(width, font.pixelSize) }
            font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Units.fontMetrics.font.pointSize
        }
    }

    actions.main: Kirigami.Action {
        text: !entry.enclosure ? i18n("Open in Browser") :
            entry.enclosure.status === Enclosure.Downloadable ? i18n("Download") :
            entry.enclosure.status === Enclosure.Downloading ? i18n("Cancel download") :
            !entry.queueStatus ? i18n("Delete download") :
            (audio.entry === entry && audio.playbackState === Audio.PlayingState) ? i18n("Pause") :
            i18n("Play")
        icon.name: !entry.enclosure ? "globe" :
            entry.enclosure.status === Enclosure.Downloadable ? "download" :
            entry.enclosure.status === Enclosure.Downloading ? "edit-delete-remove" :
            !entry.queueStatus ? "delete" :
            (audio.entry === entry && audio.playbackState === Audio.PlayingState) ? "media-playback-pause" :
            "media-playback-start"
        onTriggered: {
            if(!entry.enclosure) Qt.openUrlExternally(entry.link)
            else if(entry.enclosure.status === Enclosure.Downloadable) entry.enclosure.download()
            else if(entry.enclosure.status === Enclosure.Downloading) entry.enclosure.cancelDownload()
            else if(!entry.queueStatus) {
                entry.enclosure.deleteFile()
            } else {
                if(audio.entry === entry && audio.playbackState === Audio.PlayingState) {
                    audio.pause()
                } else {
                    audio.entry = entry
                    audio.play()
                }
            }
        }
    }

    actions.left: Kirigami.Action {
        text: !entry.queueStatus ? i18n("Add to queue") : i18n("Remove from queue")
        icon.name: !entry.queueStatus ? "media-playlist-append" : "list-remove"
        visible: entry.enclosure || entry.queueStatus
        onTriggered: {
            if(!entry.queueStatus) {
                entry.queueStatus = true
            } else {
                // first change to next track if this one is playing
                if (entry.hasEnclosure && entry === audio.entry) {
                    audio.next()
                }
                entry.queueStatus = false
            }
        }
    }

    actions.right: Kirigami.Action {
        text: i18n("Delete download")
        icon.name: "delete"
        onTriggered: entry.enclosure.deleteFile()
        visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && entry.queueStatus
    }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Reset play position")
            visible: entry.enclosure && entry.enclosure.playPosition > 1000
            onTriggered: entry.enclosure.playPosition = 0
        },
        Kirigami.Action {
            text: entry.read ? i18n("Mark as unplayed") : i18n("Mark as played")
            onTriggered: {
                entry.read = !entry.read
            }
        },
        Kirigami.Action {
            text: entry.new ? i18n("Remove \"new\" label") : i18n("Label as \"new\"")
            onTriggered: {
                entry.new = !entry.new
            }
        }
    ]
}
