/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtMultimedia 5.15
import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page

    property var feed

    title: feed.name
    supportsRefreshing: true

    onRefreshingChanged:
        if(refreshing) {
            feed.refresh()
        }

    Connections {
        target: feed
        function onRefreshingChanged(refreshing) {
            if(!refreshing)
                page.refreshing = refreshing
        }
    }

    contextualActions: [
        Kirigami.Action {
            iconName: "help-about-symbolic"
            text: i18n("Details")
            onTriggered: {
                while(pageStack.depth > 2)
                    pageStack.pop()
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
            }
        }
    ]

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh Feed")
        onTriggered: page.refreshing = true
        visible: !Kirigami.Settings.isMobile || entryList.count === 0
    }

    Kirigami.PlaceholderMessage {
        visible: entryList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: feed.errorId === 0 ? i18n("No Entries available") : i18n("Error (%1): %2", feed.errorId, feed.errorString)
        icon.name: feed.errorId === 0 ? "" : "data-error"
    }

    Component {
        id: entryListDelegate
        GenericEntryDelegate {
            listView: entryList
            entryActions: [  // TODO: put the actions back into GenericEntryDelegate
                Kirigami.Action {
                    text: i18n("Download")
                    icon.name: "download"
                    onTriggered: {
                        entry.queueStatus = true;
                        entry.enclosure.download();
                    }
                    visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloadable
                },
                Kirigami.Action {
                    text: i18n("Cancel download")
                    icon.name: "edit-delete-remove"
                    onTriggered: entry.enclosure.cancelDownload()
                    visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
                },
                Kirigami.Action {
                    text: i18n("Add to queue")
                    icon.name: "media-playlist-append"
                    visible: !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
                    onTriggered: entry.queueStatus = true
                },
                Kirigami.Action {
                    text: i18n("Play")
                    icon.name: "media-playback-start"
                    visible: entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && (audio.entry !== entry || audio.playbackState !== Audio.PlayingState)
                    onTriggered: {
                        audio.entry = entry
                        audio.play()
                    }
                },
                Kirigami.Action {
                    text: i18n("Pause")
                    icon.name: "media-playback-pause"
                    visible: entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && audio.entry === entry && audio.playbackState === Audio.PlayingState
                    onTriggered: audio.pause()
                }
            ]
        }
    }

    ListView {
        id: entryList
        visible: count !== 0
        model: page.feed.entries

        delegate: Kirigami.DelegateRecycler {
            width: entryList.width
            sourceComponent: entryListDelegate
        }

        //onOriginYChanged: contentY = originY // Why is this needed?

        //headerPositioning: ListView.OverlayHeader  // seems broken
        header: GenericListHeader {
            id: headerImage

            image: feed.image
            title: feed.name
            subtitle: page.feed.authors.length === 0 ? "" : i18nc("by <author(s)>", "by") + " " + page.feed.authors[0].name

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    while(pageStack.depth > 2)
                        pageStack.pop()
                    pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
                }
            }
        }

        MouseArea {
            anchors.fill: page.headerImage
            onClicked: {
                while(pageStack.depth > 2)
                    pageStack.pop()
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
            }
        }
    }
}
