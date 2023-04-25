/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

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

    Keys.onPressed: {
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
            id: searchActionButton
            icon.name: "search"
            text: i18nc("@action:intoolbar", "Search and Filter")
            checkable: true
            onToggled: {
                if (!checked) {
                    episodeProxyModel.filterType = AbstractEpisodeProxyModel.NoFilter;
                    episodeProxyModel.searchFilter = "";
                }
            }
        },
        Kirigami.Action {
            icon.name: "view-refresh"
            text: i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
            visible: episodeProxyModel.filterType == AbstractEpisodeProxyModel.NoFilter
        }
    ]

    Component.onCompleted: {
        for (var i in episodeList.defaultActionList) {
            pageActions.push(episodeList.defaultActionList[i]);
        }
    }

    // TODO: KF6 replace contextualActions with actions
    contextualActions: pageActions

    header: Loader {
        anchors.right: parent.right
        anchors.left: parent.left

        active: searchActionButton.checked
        visible: active
        sourceComponent: SearchFilterBar {
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

            text: i18n("No Episodes Available")
        }

        model: EpisodeProxyModel {
            id: episodeProxyModel
        }

        delegate: Component {
            id: episodeListDelegate
            GenericEntryDelegate {
                listView: episodeList
            }
        }

        FilterInlineMessage {
            proxyModel: episodeProxyModel
        }
    }
}
