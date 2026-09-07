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

    required property int entryuid
    required property int downloaded
    required property int queueStatus
    required property string link
    required property bool isNew
    required property bool read
    required property bool favorite
    required property string image
    required property string entryTitle
    required property string content
    required property bool hasEnclosure
    required property bool enclosureUrl
    required property int playPosition
    required property int feeduid
    required property string feedName

    title: KI18n.i18nc("@title", "Episode Details")

    padding: 0  // needed to get the inline header to fill the page

    onDownloadedChanged: {
        if (downloaded === DataTypes.EnclosureStatus.Downloadable) {
            // this entry has just been deleted on the downloadpage
            const pageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
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

    ColumnLayout {
        spacing: 0

        GenericHeader {
            id: infoHeader
            Layout.fillWidth: true
            image: root.image
            title: root.entryTitle
            subtitle: root.feedName
            subtitleClickable: true

            onSubtitleClicked: (root.Controls.ApplicationWindow.window as Main).openPodcast(root.feeduid)
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
                        visible: !root.hasEnclosure
                        icon.name: "globe"
                        onTriggered: {
                            Qt.openUrlExternally(root.link);
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to start episode download", "Download")
                        visible: root.hasEnclosure && (root.downloaded === DataTypes.EnclosureStatus.Downloadable || root.downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded)
                        icon.name: "download"
                        onTriggered: {
                            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.entryuid = root.entryuid;
                            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.run();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to cancel ongoing download of episode", "Cancel Download")
                        visible: root.hasEnclosure && root.downloaded === DataTypes.EnclosureStatus.Downloading
                        icon.name: "edit-delete-remove"
                        onTriggered: {
                            Fetcher.cancelEnclosureDownload(root.entryuid);
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to pause the playback of the episode", "Pause")
                        visible: root.hasEnclosure && root.queueStatus && (AudioManager.entryuid === root.entryuid && AudioManager.playbackState === KMediaSession.PlayingState)
                        icon.name: "media-playback-pause"
                        onTriggered: {
                            AudioManager.pause();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to start playback of the episode", "Play")
                        visible: root.hasEnclosure && root.downloaded === DataTypes.EnclosureStatus.Downloaded && root.queueStatus && (AudioManager.entryuid !== root.entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-start"
                        onTriggered: {
                            AudioManager.entryuid = root.entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Action to start playback by streaming the episode rather than downloading it first", "Stream")
                        visible: root.hasEnclosure && root.downloaded !== DataTypes.EnclosureStatus.Downloaded && NetworkConnectionManager.streamingAllowed && (AudioManager.entryuid !== root.entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
                        icon.name: "media-playback-cloud"
                        onTriggered: {
                            if (!root.queueStatus) {
                                DataManager.bulkQueueStatus(true, [root.entryuid]);
                            }
                            AudioManager.entryuid = root.entryuid;
                            AudioManager.play();
                        }
                    },
                    Kirigami.Action {
                        text: !root.queueStatus ? KI18n.i18nc("@action:intoolbar Button to add an episode to the play queue", "Add to Queue") : KI18n.i18nc("@action:intoolbar Button to remove an episode from the play queue", "Remove from Queue")
                        icon.name: !root.queueStatus ? "media-playlist-append" : "list-remove"
                        visible: root.hasEnclosure || root.queueStatus
                        onTriggered: {
                            if (!root.queueStatus) {
                                DataManager.bulkQueueStatus(true, [root.entryuid]);
                            } else {
                                // first change to next track if this one is playing
                                if (root.hasEnclosure && root.entryuid === AudioManager.entryuid) {
                                    AudioManager.next();
                                }
                                DataManager.bulkQueueStatus(false, [root.entryuid]);
                            }
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to remove the downloaded episode audio file", "Delete Download")
                        icon.name: "delete"
                        visible: root.hasEnclosure && (root.downloaded === DataTypes.EnclosureStatus.Downloaded || root.downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded)
                        onTriggered: {
                            DataManager.bulkDeleteEnclosures([root.entryuid]);
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to reset the play position of an episode to the start", "Reset Play Position")
                        visible: root.hasEnclosure && root.playPosition > 1000
                        onTriggered: root.hasEnclosure.playPosition = 0
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                    },
                    Kirigami.Action {
                        text: root.read ? KI18n.i18nc("@action:intoolbar Button to mark eposide as not played", "Mark as Unplayed") : KI18n.i18nc("@action:intoolbar Button to mark episode as played", "Mark as Played")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            DataManager.bulkMarkRead(!root.read, [root.entryuid]);
                        }
                    },
                    Kirigami.Action {
                        text: root.isNew ? KI18n.i18nc("@action:intoolbar", "Remove \"New\" Label") : KI18n.i18nc("@action:intoolbar", "Label as \"New\"")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            DataManager.bulkMarkNew(!root.isNew, [root.entryuid]);
                        }
                    },
                    Kirigami.Action {
                        text: root.favorite ? KI18n.i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites") : KI18n.i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
                        icon.name: !root.favorite ? "starred-symbolic" : "non-starred-symbolic"
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            DataManager.bulkMarkFavorite(!root.favorite, [root.entryuid]);
                        }
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:intoolbar Button to open the podcast URL in browser", "Open Podcast")
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: (root.Controls.ApplicationWindow.window as Main).openPodcast(root.feeduid)
                    },
                    Kirigami.Action {
                        text: KI18n.i18nc("@action:button", "Copy Episode Download URL")
                        icon.name: "edit-copy"
                        displayHint: Kirigami.DisplayHint.AlwaysHide

                        onTriggered: {
                            (root.Controls.ApplicationWindow.window as Main).showPassiveNotification(KI18n.i18nc("@info:status", "Link Copied"));
                            enclosureUrl.selectAll();
                            enclosureUrl.copy();
                            enclosureUrl.deselect();
                        }

                        // copy url from this invisible textedit
                        property TextEdit _helper: TextEdit {
                            id: enclosureUrl
                            visible: false
                            readOnly: true
                            textFormat: TextEdit.RichText
                            text: root.hasEnclosure ? root.enclosureUrl : ""
                            color: Kirigami.Theme.textColor
                        }
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
            text: EntryUtils.adjustedContent(width, font.pixelSize, root.content, root.link)
            baseUrl: EntryUtils.baseUrl(root.link)
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            font.pointSize: SettingsManager && !(SettingsManager.articleFontUseSystem) ? SettingsManager.articleFontSize : Kirigami.Theme.defaultFont.pointSize

            onLinkActivated: link => {
                if (link.split("://")[0] === "timestamp") {
                    if (AudioManager.entry && AudioManager.entry.enclosure && root.hasEnclosure && (root.downloaded === DataTypes.EnclosureStatus.Downloaded || SettingsManager.prioritizeStreaming)) {
                        if (AudioManager.entryuid !== root.entryuid) {
                            if (!root.queueStatus) {
                                DataManager.bulkQueueStatus(true, [root.entryuid]);
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
    }
}
