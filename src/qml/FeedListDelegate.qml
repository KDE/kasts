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

Kirigami.SwipeListItem {

    leftPadding: 0
    rightPadding: 0

    contentItem: Kirigami.BasicListItem {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        text: model.feed.name
        icon: model.feed.refreshing ? "view-refresh" : model.feed.image === "" ? "rss" : Fetcher.image(model.feed.image)
        iconSize: Kirigami.Units.iconSizes.large
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
                    while(pageStack.depth > 1)
                        pageStack.pop()
                feedsModel.removeFeed(index)
            }
        }

    ]

}
