/**
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
            onTriggered: pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
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

        text: feed.errorId === 0 ? i18n("No Entries available.") : i18n("Error (%1): %2", feed.errorId, feed.errorString)
        icon.name: feed.errorId === 0 ? "" : "data-error"
    }

    ListView {
        id: entryList
        visible: count !== 0
        model: EntryListModel {
            id: entryListModel
            feed: page.feed
        }

        header: EntryListHeader { }

        delegate: EntryListDelegate { }
    }
}
