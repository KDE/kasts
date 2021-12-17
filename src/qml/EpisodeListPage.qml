/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {

    title: i18n("Episode List")

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    actions.main: Kirigami.Action {
        iconName: "download"
        text: i18n("Downloads")
        onTriggered: {
            pushPage("DownloadListPage")
            SettingsManager.lastOpenedPage = "DownloadListPage" // for persistency
        }
    }

    actions.left: Kirigami.Action {
        iconName: "view-filter"
        text: i18n("Filter")
        onTriggered: filterTypeOverlay.open();
    }

    actions.right: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh All Podcasts")
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile || (episodeList.count === 0 && episodeProxyModel.filterType == EpisodeProxyModel.NoFilter)
    }

    Kirigami.OverlaySheet {
        id: filterTypeOverlay

        header: Kirigami.Heading {
            text: i18n("Select Filter")
        }

        ListView {
            // TODO: fix automatic width
            implicitWidth: Kirigami.Units.gridUnit * 15
            clip: true

            model: ListModel {
                id: filterModel
                // have to use script because i18n doesn't work within ListElement
                Component.onCompleted: {
                    var filterList = [EpisodeProxyModel.NoFilter,
                                      EpisodeProxyModel.ReadFilter,
                                      EpisodeProxyModel.NotReadFilter,
                                      EpisodeProxyModel.NewFilter,
                                      EpisodeProxyModel.NotNewFilter]
                    for (var i in filterList) {
                        filterModel.append({"name": episodeProxyModel.getFilterName(filterList[i]),
                                            "filterType": filterList[i]});
                    }
                }
            }

            delegate: Kirigami.BasicListItem {
                id: swipeDelegate
                highlighted: filterType === episodeProxyModel.filterType
                text: name
                onClicked: {
                    episodeProxyModel.filterType = filterType;
                    filterTypeOverlay.close();
                }
            }
        }
    }

    Kirigami.InlineMessage {
        z: 2
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: Kirigami.Units.largeSpacing
            bottomMargin: Kirigami.Units.largeSpacing + ( errorNotification.visible ? errorNotification.height + Kirigami.Units.largeSpacing : 0 ) + ( updateNotification.visible ? updateNotification.height + Kirigami.Units.largeSpacing : 0 )
        }
        type: Kirigami.MessageType.Information
        visible: episodeProxyModel.filterType != EpisodeProxyModel.NoFilter
        TextMetrics {
            id: textMetrics
            text: i18n("Filter Active: ") + episodeProxyModel.filterName
        }
        text: textMetrics.text
        width: Math.min(textMetrics.width + 2 * Kirigami.Units.largeSpacing + 10 * Kirigami.Units.gridUnit, parent.width)
        actions: [
            Kirigami.Action {
                id: resetButton
                icon.name: "edit-delete-remove"
                text: i18n("Reset")
                onTriggered: {
                    episodeProxyModel.filterType = EpisodeProxyModel.NoFilter;
                }
            }
        ]
    }

    contextualActions: episodeList.defaultActionList

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Episodes Available")
    }

    Component {
        id: episodeListDelegate
        GenericEntryDelegate {
            listView: episodeList
        }
    }

    EpisodeProxyModel {
        id: episodeProxyModel
    }

    GenericEntryListView {
        id: episodeList
        anchors.fill: parent
        visible: count !== 0
        reuseItems: true

        model: episodeProxyModel
        delegate: episodeListDelegate
    }
}
