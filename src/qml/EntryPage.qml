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
import org.kde.ki18n

import org.kde.kmediasession
import org.kde.kasts

Kirigami.ScrollablePage {
    id: root

    property Entry entry: DataManager.getEntry(entryuid)
    required property int entryuid

    title: KI18n.i18nc("@title", "Episode Details")

    padding: 0  // needed to get the inline header to fill the page

    // This function is needed to close the EntryPage if it is opened over the
    // QueuePage when the episode is removed from the queue (e.g. when the
    // episode finishes).
    Connections {
        target: root.entry
        function onQueueStatusChanged(): void {
            if (!root.entry.queueStatus) {
                // this entry has just been removed from the queue
                var pageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "queuepage") {
                        if (pageStack.get(0).lastEntry) {
                            if (pageStack.get(0).lastEntry === root.entryuid) {
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
        target: root.entry.enclosure
        function onStatusChanged(): void {
            if (root.entry.enclosure.status === Enclosure.Downloadable) {
                // this entry has just been deleted on the downloadpage
                var pageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "downloadpage") {
                        if (pageStack.get(0).lastEntry) {
                            if (pageStack.get(0).lastEntry === root.entryuid) {
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
            image: root.entry.cachedImage
            title: root.entry.title
            subtitle: root.entry.feed.name
            subtitleClickable: true

            onSubtitleClicked: (root.Controls.ApplicationWindow.window as Main).openPodcast(root.entry.feeduid)
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
                        text: KI18n.i18nc("@action:intoolbar Button to open an episode URL in browser", "Open in Browser")
                        visible: !root.entry.enclosure
                        icon.name: "globe"
                        onTriggered: {
                            Qt.openUrlExternally(root.entry.link);
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to start episode download", "Download")
                        visible: root.entry.enclosure && (root.entry.enclosure.status === Enclosure.Downloadable || root.entry.enclosure.status === Enclosure.PartiallyDownloaded)
                        icon.name: "download"
                        onTriggered: {
                            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.entry = root.entry;
                            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.run();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to cancel ongoing download of episode", "Cancel Download")
                        visible: root.entry.enclosure && root.entry.enclosure.status === Enclosure.Downloading
                        icon.name: "edit-delete-remove"
                        onTriggered: {
                            root.entry.enclosure.cancelDownload();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to pause the playback of the episode", "Pause")
                        visible: root.entry.enclosure && root.entry.queueStatus && (AudioManager.entryuid === root.entryuid && AudioManager.playbackState === KMediaSession.PlayingState)
                        icon.name: "media-playback-pause"
                        onTriggered: {
                            AudioManager.pause();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to start playback of the episode", "Play")
                        visible: root.entry.enclosure && root.entry.enclosure.status === Enclosure.Downloaded && root.entry.queueStatus && (AudioManager.entryuid !== root.entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-start"
                        onTriggered: {
                            AudioManager.entryuid = root.entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Action to start playback by streaming the episode rather than downloading it first", "Stream")
                        visible: root.entry.enclosure && root.entry.enclosure.status !== Enclosure.Downloaded && NetworkConnectionManager.streamingAllowed && (AudioManager.entryuid !== root.entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-cloud"
                        onTriggered: {
                            if (!root.entry.queueStatus) {
                                root.entry.queueStatus = true;
                            }
                            AudioManager.entryuid = root.entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: !root.entry.queueStatus ? KI18n.i18nc("@action:intoolbar Button to add an episode to the play queue", "Add to Queue") : KI18n.i18nc("@action:intoolbar Button to remove an episode from the play queue", "Remove from Queue")
                        icon.name: !root.entry.queueStatus ? "media-playlist-append" : "list-remove"
                        visible: root.entry.enclosure || root.entry.queueStatus
                        onTriggered: {
                            if (!root.entry.queueStatus) {
                                root.entry.queueStatus = true;
                            } else {
                                // first change to next track if this one is playing
                                if (root.entry.hasEnclosure && root.entryuid === AudioManager.entryuid) {
                                    AudioManager.next();
                                }
                                root.entry.queueStatus = false;
                            }
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to remove the downloaded episode audio file", "Delete Download")
                        icon.name: "delete"
                        visible: root.entry.enclosure && (root.entry.enclosure.status === Enclosure.Downloaded || root.entry.enclosure.status === Enclosure.PartiallyDownloaded)
                        onTriggered: {
                            root.entry.enclosure.deleteFile();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to reset the play position of an episode to the start", "Reset Play Position")
                        visible: root.entry.enclosure && root.entry.enclosure.playPosition > 1000
                        onTriggered: root.entry.enclosure.playPosition = 0
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                    },
                    Kirigami.Action {
                        text: root.entry.read ? KI18n.i18nc("@action:intoolbar Button to mark eposide as not played", "Mark as Unplayed") : KI18n.i18nc("@action:intoolbar Button to mark episode as played", "Mark as Played")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            root.entry.read = !root.entry.read;
                        }
                    },
                    Kirigami.Action {
                        text: root.entry.new ? KI18n.i18nc("@action:intoolbar", "Remove \"New\" Label") : KI18n.i18nc("@action:intoolbar", "Label as \"New\"")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            root.entry.new = !root.entry.new;
                        }
                    },
                    Kirigami.Action {
                        text: root.entry.favorite ? KI18n.i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites") : KI18n.i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
                        icon.name: !root.entry.favorite ? "starred-symbolic" : "non-starred-symbolic"
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            root.entry.favorite = !root.entry.favorite;
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to open the podcast URL in browser", "Open Podcast")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: (root.Controls.ApplicationWindow.window as Main).openPodcast(root.entry.feeduid)
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
            text: root.entry.content
            baseUrl: root.entry.baseUrl
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Theme.defaultFont.pointSize

            onLinkActivated: link => {
                if (link.split("://")[0] === "timestamp") {
                    if (AudioManager.entry && AudioManager.entry.enclosure && root.entry.enclosure && (root.entry.enclosure.status === Enclosure.Downloaded || SettingsManager.prioritizeStreaming)) {
                        if (AudioManager.entryuid !== root.entryuid) {
                            if (!root.entry.queueStatus) {
                                root.entry.queueStatus = true;
                            }
                            AudioManager.entryuid = root.entryuid;
                            AudioManager.play();
                        }
                        AudioManager.seek(link.split("://")[1]);
                    }
                } else {
                    Qt.openUrlExternally(link);
                }
            }

            onWidthChanged: {
                text = root.entry.adjustedContent(width, font.pixelSize);
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
                entryuid: root.entryuid
            }
            delegate: ChapterListDelegate {}
        }

        Controls.Button {
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            visible: root.entry.hasEnclosure

            text: KI18n.i18nc("@action:button", "Copy Episode Download URL")
            height: enclosureUrl.height
            width: enclosureUrl.height
            icon.name: "edit-copy"

            onClicked: {
                (Controls.ApplicationWindow.window as Main).showPassiveNotification(KI18n.i18nc("@info:status", "Link Copied"));
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
                text: root.entry.hasEnclosure ? root.entry.enclosure.url : ""
                color: Kirigami.Theme.textColor
            }
        }
    }
}
