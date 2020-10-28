/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

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

    ListView {
        id: entryList
        visible: count !== 0
        model: page.feed.entries

        header: EntryListHeader { }

        delegate: EntryListDelegate { }
    }
}
