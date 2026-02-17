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
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: episodeListPage
    title: KI18n.i18nc("@title of page with list of podcast episodes", "Episodes")

    property alias episodeList: episodeList

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    supportsRefreshing: true
    onRefreshingChanged: {
        if (refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    Keys.onPressed: event => {
        if (event.matches(StandardKey.Find)) {
            searchActionButton.checked = true;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            icon.name: "download"
            text: KI18n.i18nc("@title of page with list of downloaded episodes", "Downloads")
            onTriggered: {
                pushPage("DownloadListPage");
            }
        },
        Kirigami.Action {
            icon.name: "view-refresh"
            text: KI18n.i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
        },
        Kirigami.Action {
            id: searchActionButton
            icon.name: "search"
            text: KI18n.i18nc("@action:intoolbar", "Search")
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

            text: KI18n.i18n("No episodes available")
        }

        model: EpisodeProxyModel {
            id: episodeProxyModel

            // save and restore filter settings
            filterType: settings.episodeListFilterType
            onFilterTypeChanged: {
                settings.episodeListFilterType = filterType;
            }

            // save and restore sort settings
            sortType: settings.episodeListSortType
            onSortTypeChanged: {
                settings.episodeListSortType = sortType;
            }
        }

        delegate: GenericEntryDelegate {
            listViewObject: episodeList
        }

        FilterInlineMessage {
            proxyModel: episodeProxyModel
        }
    }

    ConnectionCheckAction {
        id: updateAllFeeds
    }
}
