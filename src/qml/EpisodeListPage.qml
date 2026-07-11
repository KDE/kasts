/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtCore

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: root
    title: KI18n.i18nc("@title of page with list of podcast episodes", "Episodes")

    property alias episodeList: episodeList

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
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
            visible: Kirigami.Settings.isMobile
            icon.name: "download"
            text: KI18n.i18nc("@title of page with list of downloaded episodes", "Downloads")
            onTriggered: {
                (root.Controls.ApplicationWindow.window as Main).pushPage("DownloadListPage");
            }
        },
        Kirigami.Action {
            icon.name: "view-refresh"
            text: KI18n.i18n("Refresh All Podcasts")
            onTriggered: root.refreshing = true
        },
        Kirigami.Action {
            id: searchActionButton
            icon.name: "search"
            text: KI18n.i18nc("@action:intoolbar", "Search")
            checkable: true
        }
    ]

    Component.onCompleted: {
        for (let i in episodeList.defaultActionList) {
            pageActions.push(episodeList.defaultActionList[i]);
        }
    }

    Component.onDestruction: {
        KastsState.episodeListFilterType = episodeProxyModel.filterType;
        KastsState.episodeListSortType = episodeProxyModel.sortType;
        KastsState.save();
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
            filterType: KastsState.episodeListFilterType

            // save and restore sort settings
            sortType: KastsState.episodeListSortType
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
