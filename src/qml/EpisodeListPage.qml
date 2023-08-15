/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.ScrollablePage {
    id: episodeListPage
    title: i18n("Episode List")

    property alias episodeList: episodeList

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    Keys.onPressed: (event) => {
        if (event.matches(StandardKey.Find)) {
            searchActionButton.checked = true;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            icon.name: "download"
            text: i18n("Downloads")
            onTriggered: {
                pushPage("DownloadListPage")
            }
        },
        Kirigami.Action {
            icon.name: "view-refresh"
            text: i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
            visible: episodeProxyModel.filterType == AbstractEpisodeProxyModel.NoFilter
        },
        Kirigami.Action {
            id: searchActionButton
            icon.name: "search"
            text: i18nc("@action:intoolbar", "Search")
            checkable: true
        }
    ]

    Component.onCompleted: {
        for (var i in episodeList.defaultActionList) {
            pageActions.push(episodeList.defaultActionList[i]);
        }
    }

    actions: pageActions

    header: Loader {
        anchors.right: parent.right
        anchors.left: parent.left

        active: searchActionButton.checked
        visible: active
        sourceComponent: SearchBar {
            proxyModel: episodeProxyModel
            parentKey: searchActionButton
        }
    }

    GenericEntryListView {
        id: episodeList
        anchors.fill: parent
        reuseItems: true

        Kirigami.PlaceholderMessage {
            visible: episodeList.count === 0

            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent

            text: i18n("No episodes available")
        }

        model: EpisodeProxyModel {
            id: episodeProxyModel
        }

        delegate: GenericEntryDelegate {
            listViewObject: episodeList
        }

        FilterInlineMessage {
            proxyModel: episodeProxyModel
        }
    }
}
