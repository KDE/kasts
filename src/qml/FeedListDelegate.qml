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

Kirigami.SwipeListItem {

    contentItem: Kirigami.BasicListItem {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        text: model.feed.name
        icon: model.feed.refreshing ? "view-refresh" : model.feed.image === "" ? "rss" : Fetcher.image(model.feed.image)
        subtitle: i18np("%1 unread entry", "%1 unread entries", model.feed.unreadEntryCount)


        onClicked: {
            lastFeed = model.feed.url
            pageStack.push("qrc:/EntryListPage.qml", {"feed": model.feed})
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "delete"
            onTriggered: {
                if(pageStack.depth > 1 && model.feed.url === lastFeed)
                    pageStack.pop()
                feedListModel.removeFeed(index)
            }
        }

    ]

}
