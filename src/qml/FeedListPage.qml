/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import Qt.labs.platform 1.1
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: subscriptionPage
    title: i18n("Subscriptions")

    anchors.margins: 0
    padding: 0

    property var lastFeed: ""

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing)  {
            Fetcher.fetchAll()
            refreshing = false
        }
    }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Refresh all feeds")
            iconName: "view-refresh"
            onTriggered: refreshing = true
            visible: !Kirigami.Settings.isMobile
        },
        Kirigami.Action {
            text: i18n("Import Feeds...")
            iconName: "document-import"
            onTriggered: importDialog.open()
        },
        Kirigami.Action {
            text: i18n("Export Feeds...")
            iconName: "document-export"
            onTriggered: exportDialog.open()
        }
    ]

    actions.main: Kirigami.Action {
        text: i18n("Add feed")
        iconName: "list-add"
        onTriggered: {
            addSheet.open()
        }
    }

        AddFeedSheet {
        id: addSheet
    }

    Kirigami.PlaceholderMessage {
        visible: feedList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Feeds added yet")
    }

    FileDialog {
        id: importDialog
        title: i18n("Import Feeds")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [i18n("All Files (*)"), i18n("XML Files (*.xml)"), i18n("OPML Files (*.opml)")]
        onAccepted: DataManager.importFeeds(file)
    }

    FileDialog {
        id: exportDialog
        title: i18n("Export Feeds")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [i18n("All Files")]
        onAccepted: DataManager.exportFeeds(file)
        fileMode: FileDialog.SaveFile
    }


    mainItem: GridView {
        id: feedList
        visible: count !== 0

        property int minimumCardSize: 150
        property int cardMargin: Kirigami.Units.largeSpacing

        property int columns: Math.floor(width / (minimumCardSize + 2 * cardMargin))

        cellWidth: width / columns
        cellHeight: width / columns

        model: FeedsModel {
            id: feedsModel
        }

        Component {
            id: feedListDelegate
            FeedListDelegate {
                cardSize: feedList.width / feedList.columns - 2 * feedList.cardMargin
                cardMargin: feedList.cardMargin
            }
        }

        delegate: Kirigami.DelegateRecycler {
            sourceComponent: feedListDelegate
        }
    }
}
