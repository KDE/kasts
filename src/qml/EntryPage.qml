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

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page

    property QtObject entry

    title: entry.title

    padding: 0  // needed to get the inline header to fill the page

    Connections {
        target: entry
        function onQueueStatusChanged() {
            if (entry.queueStatus === false) {
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
        GenericListHeader {
            id: infoHeader
            Layout.fillWidth: true
            image: entry.image
            title: entry.title
            subtitle: entry.feed.name
        }

        Controls.Label {
            Layout.margins: Kirigami.Units.gridUnit
            id: text
            text: page.entry.content
            baseUrl: page.entry.baseUrl
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.fillHeight: true
            onLinkActivated: Qt.openUrlExternally(link)
            //onWidthChanged: { text = entry.adjustedContent(width, font.pixelSize) }
        }
    }

    actions.main: Kirigami.Action {
        text: !entry.enclosure ? i18n("Open in Browser") :
            entry.enclosure.status === Enclosure.Downloadable ? i18n("Download") :
            entry.enclosure.status === Enclosure.Downloading ? i18n("Cancel download") :
            !entry.queueStatus ? i18n("Add to Queue") :
            (audio.entry === entry) && audio.playbackState === Audio.PlayingState ? i18n("Play") :
            i18n("Pause")
        icon.name: !entry.enclosure ? "globe" :
            entry.enclosure.status === Enclosure.Downloadable ? "download" :
            entry.enclosure.status === Enclosure.Downloading ? "edit-delete-remove" :
            !entry.queueStatus ? "media-playlist-append" :
            (audio.entry === entry && audio.playbackState === Audio.PlayingState) ? "media-playback-pause" :
            "media-playback-start"
        onTriggered: {
            if(!entry.enclosure) Qt.openUrlExternally(entry.link)
            else if(entry.enclosure.status === Enclosure.Downloadable) entry.enclosure.download()
            else if(entry.enclosure.status === Enclosure.Downloading) entry.enclosure.cancelDownload()
            else if(entry.queueStatus) {
                if(audio.entry === entry && audio.playbackState === Audio.PlayingState) {
                    audio.pause()
                } else {
                    audio.entry = entry
                    audio.play()
                }
            } else {
                entry.queueStatus = true
            }
        }
    }
    actions.left: Kirigami.Action {
        text: !entry.queueStatus ? i18n("Add to queue") : i18n("Remove from Queue")
        icon.name: !entry.queueStatus ? "media-playlist-append" : "list-remove"
        visible: entry.enclosure
        onTriggered: {
            if(!entry.queueStatus) {
                entry.queueStatus = true
            } else {
                // first change to next track if this one is playing
                if (entry === audio.entry) {
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
        visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
    }
    contextualActions: [
        Kirigami.Action {
            text: i18n("Reset play position")
            visible: entry.enclosure && entry.enclosure.playPosition > 1000
            onTriggered: entry.enclosure.playPosition = 0
        },
        Kirigami.Action {
            text: entry.read ? i18n("Unmark as Played") : i18n("Mark as Played")
            onTriggered: {
                if(entry.read) {
                    entry.read = false
                } else {
                    entry.read = true
                }
            }
        },
        Kirigami.Action {
            text: entry.new ? i18n("Unmark as New") : i18n("Mark as New")
            onTriggered: {
                if(entry.new) {
                    entry.new = false
                } else {
                    entry.new = true
                }
            }
        }
    ]
}
