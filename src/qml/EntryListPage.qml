/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: page

    property var feed

    title: i18n("Episode List")
    supportsRefreshing: true

    onRefreshingChanged: {
        if(refreshing) {
            updateFeed.run()
        }
    }

    // Overlay dialog box showing options what to do on metered connections
    ConnectionCheckAction {
        id: updateFeed

        function action() {
            feed.refresh()
        }

        function abortAction() {
            page.refreshing = false
        }
    }

    // Make sure that this feed is also showing as "refreshing" on FeedListPage
    Connections {
        target: feed
        function onRefreshingChanged(refreshing) {
            if(!refreshing)
                page.refreshing = refreshing
        }
    }

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh Podcast")
        onTriggered: page.refreshing = true
    }

    contextualActions: [
        Kirigami.Action {
            iconName: "help-about-symbolic"
            text: i18n("Podcast Details")
            onTriggered: {
                while(pageStack.depth > 2)
                    pageStack.pop()
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
            }
        }
    ]

    // add the default actions through onCompleted to add them to the ones
    // defined above
    Component.onCompleted: {
        for (var i in entryList.defaultActionList) {
            contextualActions.push(entryList.defaultActionList[i]);
        }
    }

    Kirigami.PlaceholderMessage {
        visible: entryList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: feed.errorId === 0 ? i18n("No Episodes Available") : i18n("Error (%1): %2", feed.errorId, feed.errorString)
        icon.name: feed.errorId === 0 ? "" : "data-error"
    }

    Component {
        id: entryListDelegate
        GenericEntryDelegate {
            listView: entryList
        }
    }

    GenericEntryListView {
        id: entryList
        visible: count !== 0
        reuseItems: true

        model: page.feed.entries
        delegate: entryListDelegate

        // OverlayHeader looks nicer, but seems completely broken when flicking the list
        // headerPositioning: ListView.OverlayHeader
        header: GenericHeader {
            id: headerImage

            image: feed.cachedImage
            title: feed.name
            subtitle: page.feed.authors.length === 0 ? "" : i18nc("by <author(s)>", "by %1", page.feed.authors[0].name)

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
