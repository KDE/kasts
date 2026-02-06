/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
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

Kirigami.ScrollablePage {
    id: page

    required property QtObject entry
    property int entryuid: entry ? entry.entryuid : 0

    title: i18nc("@title", "Episode Details")

    padding: 0  // needed to get the inline header to fill the page

    // This function is needed to close the EntryPage if it is opened over the
    // QueuePage when the episode is removed from the queue (e.g. when the
    // episode finishes).
    Connections {
        target: entry
        function onQueueStatusChanged(): void {
            if (!entry.queueStatus) {
                // this entry has just been removed from the queue
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "queuepage") {
                        if (pageStack.get(0).lastEntry) {
                            if (pageStack.get(0).lastEntry === entryuid) {
                                // if this EntryPage was open, then close it
                                pageStack.pop();
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: entry.enclosure
        function onStatusChanged(): void {
            if (entry.enclosure.status === Enclosure.Downloadable) {
                // this entry has just been deleted on the downloadpage
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "downloadpage") {
                        if (pageStack.get(0).lastEntry) {
                            if (pageStack.get(0).lastEntry === entryuid) {
                                // if this EntryPage was open, then close it
                                pageStack.pop();
                            }
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        spacing: 0

        GenericHeader {
            id: infoHeader
            Layout.fillWidth: true
            image: entry.cachedImage
            title: entry.title
            subtitle: entry.feed.name
            subtitleClickable: true

            onSubtitleClicked: openPodcast(entry.feed)
        }

        // header actions
        Controls.Control {
            Layout.fillWidth: true

            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.smallSpacing
            topPadding: Kirigami.Units.smallSpacing

            background: Rectangle {
                color: Kirigami.Theme.alternateBackgroundColor

                Kirigami.Separator {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                }
            }

            contentItem: Kirigami.ActionToolBar {
                alignment: Qt.AlignLeft
                background: Item {}

                actions: [
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to open an episode URL in browser", "Open in Browser")
                        visible: !entry.enclosure
                        icon.name: "globe"
                        onTriggered: {
                            Qt.openUrlExternally(entry.link);
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to start episode download", "Download")
                        visible: entry.enclosure && (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded)
                        icon.name: "download"
                        onTriggered: {
                            downloadOverlay.entry = entry;
                            downloadOverlay.run();
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to cancel ongoing download of episode", "Cancel Download")
                        visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
                        icon.name: "edit-delete-remove"
                        onTriggered: {
                            entry.enclosure.cancelDownload();
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to pause the playback of the episode", "Pause")
                        visible: entry.enclosure && entry.queueStatus && (AudioManager.entryuid === entryuid && AudioManager.playbackState === KMediaSession.PlayingState)
                        icon.name: "media-playback-pause"
                        onTriggered: {
                            AudioManager.pause();
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to start playback of the episode", "Play")
                        visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && entry.queueStatus && (AudioManager.entryuid !== entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-start"
                        onTriggered: {
                            AudioManager.entryuid = entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Action to start playback by streaming the episode rather than downloading it first", "Stream")
                        visible: entry.enclosure && entry.enclosure.status !== Enclosure.Downloaded && NetworkConnectionManager.streamingAllowed && (AudioManager.entryuid !== entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-cloud"
                        onTriggered: {
                            if (!entry.queueStatus) {
                                entry.queueStatus = true;
                            }
                            AudioManager.entryuid = entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: !entry.queueStatus ? i18nc("@action:intoolbar Button to add an episode to the play queue", "Add to Queue") : i18nc("@action:intoolbar Button to remove an episode from the play queue", "Remove from Queue")
                        icon.name: !entry.queueStatus ? "media-playlist-append" : "list-remove"
                        visible: entry.enclosure || entry.queueStatus
                        onTriggered: {
                            if (!entry.queueStatus) {
                                entry.queueStatus = true;
                            } else {
                                // first change to next track if this one is playing
                                if (entry.hasEnclosure && entryuid === AudioManager.entryuid) {
                                    AudioManager.next();
                                }
                                entry.queueStatus = false;
                            }
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to remove the downloaded episode audio file", "Delete Download")
                        icon.name: "delete"
                        visible: entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded || entry.enclosure.status === Enclosure.PartiallyDownloaded)
                        onTriggered: {
                            entry.enclosure.deleteFile();
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to reset the play position of an episode to the start", "Reset Play Position")
                        visible: entry.enclosure && entry.enclosure.playPosition > 1000
                        onTriggered: entry.enclosure.playPosition = 0
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                    },
                    Kirigami.Action {
                        text: entry.read ? i18nc("@action:intoolbar Button to mark eposide as not played", "Mark as Unplayed") : i18nc("@action:intoolbar Button to mark episode as played", "Mark as Played")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            entry.read = !entry.read;
                        }
                    },
                    Kirigami.Action {
                        text: entry.new ? i18nc("@action:intoolbar", "Remove \"New\" Label") : i18nc("@action:intoolbar", "Label as \"New\"")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            entry.new = !entry.new;
                        }
                    },
                    Kirigami.Action {
                        text: entry.favorite ? i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites") : i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
                        icon.name: !entry.favorite ? "starred-symbolic" : "non-starred-symbolic"
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            entry.favorite = !entry.favorite;
                        }
                    },
                    Kirigami.Action {
                        text: i18nc("@action:intoolbar Button to open the podcast URL in browser", "Open Podcast")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: openPodcast(entry.feed)
                    }
                ]
            }
        }

        Kirigami.SelectableLabel {
            id: textLabel
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            // maximumWidth needed because otherwise the actual page will be as
            // wide as the longest unwrapped text and can be scrolled horizontally
            Layout.maximumWidth: parent.width - Layout.rightMargin - Layout.leftMargin
            Layout.fillWidth: true
            Layout.fillHeight: true

            selectByMouse: !Kirigami.Settings.isMobile
            text: page.entry.content
            baseUrl: page.entry.baseUrl
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Theme.defaultFont.pointSize

            onLinkActivated: link => {
                if (link.split("://")[0] === "timestamp") {
                    if (AudioManager.entry && AudioManager.entry.enclosure && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded || SettingsManager.prioritizeStreaming)) {
                        if (AudioManager.entryuid !== entryuid) {
                            if (!entry.queueStatus) {
                                entry.queueStatus = true;
                            }
                            AudioManager.entryuid = entryuid;
                            AudioManager.play();
                        }
                        AudioManager.seek(link.split("://")[1]);
                    }
                } else {
                    Qt.openUrlExternally(link);
                }
            }

            onWidthChanged: {
                text = entry.adjustedContent(width, font.pixelSize);
            }
        }

        ListView {
            visible: count !== 0
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            currentIndex: -1
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            model: ChapterModel {
                entryuid: page.entry.entryuid
            }
            delegate: ChapterListDelegate {}
        }

        Controls.Button {
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            visible: entry.hasEnclosure

            text: i18nc("@action:button", "Copy Episode Download URL")
            height: enclosureUrl.height
            width: enclosureUrl.height
            icon.name: "edit-copy"

            onClicked: {
                applicationWindow().showPassiveNotification(i18nc("@info:status", "Link Copied"));
                enclosureUrl.selectAll();
                enclosureUrl.copy();
                enclosureUrl.deselect();
            }

            // copy url from this invisible textedit
            TextEdit {
                id: enclosureUrl
                visible: false
                readOnly: true
                textFormat: TextEdit.RichText
                text: entry.hasEnclosure ? entry.enclosure.url : ""
                color: Kirigami.Theme.textColor
            }
        }
    }
}
