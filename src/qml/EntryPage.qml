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

    title: i18n("Episode Details")

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
                    if (pageStack.get(0).pageName === "queuepage") {
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
    }

    Connections {
        target: entry.enclosure
        function onStatusChanged() {
            if (entry.enclosure.status === Enclosure.Downloadable) {
                // this entry has just been deleted on the downloadpage
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "downloadpage") {
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
    }

    ColumnLayout {
        GenericHeader {
            id: infoHeader
            Layout.fillWidth: true
            image: entry.cachedImage
            title: entry.title
            subtitle: entry.feed.name
        }

        TextEdit {
            id: textLabel
            Layout.margins: Kirigami.Units.gridUnit
            readOnly: true
            selectByMouse: true
            text: page.entry.content
            baseUrl: page.entry.baseUrl
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.fillHeight: true
            onLinkActivated: Qt.openUrlExternally(link)
            onWidthChanged: { text = entry.adjustedContent(width, font.pixelSize) }
            font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Theme.defaultFont.pointSize
        }
        ListView {
            Layout.fillWidth: true
            height: contentHeight
            interactive: false
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            model: ChapterModel {
                enclosureId: entry.id
            }
            delegate: ChapterListDelegate {
                entry: page.entry
            }
        }
    }

    actions.main: Kirigami.Action {
        text: !entry.enclosure ? i18n("Open in Browser") :
            (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) ? i18n("Download") :
            entry.enclosure.status === Enclosure.Downloading ? i18n("Cancel Download") :
            !entry.queueStatus ? i18n("Delete Download") :
            (AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState) ? i18n("Pause") :
            i18n("Play")
        icon.name: !entry.enclosure ? "globe" :
            (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) ? "download" :
            entry.enclosure.status === Enclosure.Downloading ? "edit-delete-remove" :
            !entry.queueStatus ? "delete" :
            (AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState) ? "media-playback-pause" :
            "media-playback-start"
        onTriggered: {
            if (!entry.enclosure) {
                Qt.openUrlExternally(entry.link)
            } else if (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) {
                downloadOverlay.entry = entry;
                downloadOverlay.run();
            } else if (entry.enclosure.status === Enclosure.Downloading) {
                entry.enclosure.cancelDownload()
            } else if (!entry.queueStatus) {
                entry.enclosure.deleteFile()
            } else {
                if(AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState) {
                    AudioManager.pause()
                } else {
                    AudioManager.entry = entry
                    AudioManager.play()
                }
            }
        }
    }

    actions.left: Kirigami.Action {
        text: !entry.queueStatus ? i18n("Add to Queue") : i18n("Remove from Queue")
        icon.name: !entry.queueStatus ? "media-playlist-append" : "list-remove"
        visible: entry.enclosure || entry.queueStatus
        onTriggered: {
            if(!entry.queueStatus) {
                entry.queueStatus = true
            } else {
                // first change to next track if this one is playing
                if (entry.hasEnclosure && entry === AudioManager.entry) {
                    AudioManager.next()
                }
                entry.queueStatus = false
            }
        }
    }

    actions.right: Kirigami.Action {
        text: i18n("Delete Download")
        icon.name: "delete"
        onTriggered: entry.enclosure.deleteFile();
        visible: entry.enclosure && ((entry.enclosure.status === Enclosure.Downloaded && entry.queueStatus) || entry.enclosure.status === Enclosure.PartiallyDownloaded)
    }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Reset Play Position")
            visible: entry.enclosure && entry.enclosure.playPosition > 1000
            onTriggered: entry.enclosure.playPosition = 0
        },
        Kirigami.Action {
            text: entry.read ? i18n("Mark as Unplayed") : i18n("Mark as Played")
            onTriggered: {
                entry.read = !entry.read
            }
        },
        Kirigami.Action {
            text: entry.new ? i18n("Remove \"New\" Label") : i18n("Label as \"New\"")
            onTriggered: {
                entry.new = !entry.new
            }
        }
    ]
}
