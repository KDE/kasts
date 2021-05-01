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
import org.kde.kirigami 2.14 as Kirigami

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
        },
        Kirigami.Action {
            iconName: "delete"
            text: i18n("Remove feed")
            onTriggered: {
                while(pageStack.depth > 1)
                    pageStack.pop()
                DataManager.removeFeed(feed)
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

        // OverlayHeader looks nicer, but seems completely broken when flicking the list
        // headerPositioning: ListView.OverlayHeader
        header: GenericHeader {
            id: headerImage

            image: feed.cachedImage
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
    }
}
