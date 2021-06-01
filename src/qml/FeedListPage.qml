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

import org.kde.kasts 1.0

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
            text: i18n("Refresh All Podcasts")
            iconName: "view-refresh"
            onTriggered: refreshing = true
            visible: !Kirigami.Settings.isMobile
        },
        Kirigami.Action {
            text: i18n("Import Podcasts...")
            iconName: "document-import"
            onTriggered: importDialog.open()
        },
        Kirigami.Action {
            text: i18n("Export Podcasts...")
            iconName: "document-export"
            onTriggered: exportDialog.open()
        }
    ]

    actions.main: Kirigami.Action {
        text: i18n("Add Podcast")
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

        text: i18n("No Podcasts Added Yet")
    }

    FileDialog {
        id: importDialog
        title: i18n("Import Podcasts")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [i18n("All Files (*)"), i18n("XML Files (*.xml)"), i18n("OPML Files (*.opml)")]
        onAccepted: DataManager.importFeeds(file)
    }

    FileDialog {
        id: exportDialog
        title: i18n("Export Podcasts")
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
        // In order to account for the scrollbar popping up and creating a
        // binding loop, we calculate the number of columns and card width based
        // on the total width of the page itself rather than the width left for
        // the GridView, and then subtract some space
        property int availableWidth: subscriptionPage.width - !Kirigami.Settings.isMobile * Kirigami.Units.gridUnit * 1.3
        // TODO: get proper width for scrollbar rather than hardcoding it

        property int columns: Math.max(1, Math.floor(availableWidth / (minimumCardSize + 2 * cardMargin)))

        cellWidth: availableWidth / columns
        cellHeight: availableWidth / columns

        model: FeedsModel {
            id: feedsModel
        }

        Component {
            id: feedListDelegate
            FeedListDelegate {
                cardSize: feedList.availableWidth / feedList.columns - 2 * feedList.cardMargin
                cardMargin: feedList.cardMargin
            }
        }

        delegate: Kirigami.DelegateRecycler {
            sourceComponent: feedListDelegate
        }
    }
}
