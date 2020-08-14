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
    title: "Alligator"

    property var lastFeed: ""

    supportsRefreshing: true
    onRefreshingChanged:
        if(refreshing)  {
            Fetcher.fetchAll()
            refreshing = false
        }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Refresh all feeds")
            iconName: "view-refresh"
            onTriggered: refreshing = true
            visible: !Kirigami.Settings.isMobile
        }
    ]

    AddFeedSheet {
        id: addSheet
    }

    actions.main: Kirigami.Action {
        text: i18n("Add feed")
        iconName: "list-add"
        onTriggered: {
            addSheet.open()
        }
    }

    Kirigami.PlaceholderMessage {
        visible: feedList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Feeds added yet.")
    }

    ListView {
        id: feedList
        visible: count !== 0
        anchors.fill: parent
        model: FeedListModel {
            id: feedListModel
        }

        delegate: FeedListDelegate { }
    }
}
