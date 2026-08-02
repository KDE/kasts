/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
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
    id: root

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    required property QtObject feed
    property bool isSubscribed: true
    property var subscribeAction: undefined // this is only used if instantiated from the discoverpage

    property bool showMoreInfo: false

    title: KI18n.i18n("Podcast Details")

    Keys.onPressed: event => {
        if (event.matches(StandardKey.Find)) {
            searchActionButton.checked = true;
        }
    }

    supportsRefreshing: true

    onRefreshingChanged: {
        if (refreshing) {
            updateFeed.run();
        }
    }

    // Overlay dialog box showing options what to do on metered connections
    ConnectionCheckAction {
        id: updateFeed

        function action(): void {
            root.feed.refresh();
        }

        function abortAction(): void {
            root.refreshing = false;
        }
    }

    // Make sure that this feed is also showing as "refreshing" on FeedListPage
    Connections {
        target: root.feed
        function onRefreshingChanged(refreshing: bool): void {
            if (!refreshing)
                root.refreshing = refreshing;
        }
    }

    actions: Kirigami.Action {
        id: searchActionButton
        icon.name: "search"
        text: KI18n.i18nc("@action:intoolbar", "Search")
        checkable: true
        enabled: root.feed.entries ? true : false
        visible: enabled

        // Make sure to show the searchbar if there is still a searchFilter active
        Component.onCompleted: {
            checked = (root.feed.entries ? root.feed.entries.searchFilter != "" : false);
        }
    }

    header: Loader {
        anchors.right: parent.right
        anchors.left: parent.left

        active: searchActionButton.checked
        visible: active

        sourceComponent: SearchBar {
            proxyModel: root.feed.entries ? root.feed.entries : emptyListModel
            parentKey: searchActionButton
        }
    }

    ListModel {
        id: emptyListModel
        readonly property var filterType: AbstractEpisodeProxyModel.NoFilter
    }

    GenericEntryListView {
        id: entryList
        reuseItems: true
        currentIndex: -1

        model: root.feed.entries ? root.feed.entries : emptyListModel
        delegate: GenericEntryDelegate {
            listViewObject: entryList
            // no need to show the podcast image or title on every delegate
            // because we're looking at only one podcast right now
            showFeedImage: false
            showFeedTitle: false
        }

        header: ColumnLayout {
            id: headerColumn
            height: (isSubscribed && entryList.count > 0) ? implicitHeight : entryList.height
            width: entryList.width
            spacing: 0

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window

            GenericHeader {
                id: headerImage
                Layout.fillWidth: true

                property string authors: isSubscribed ? feed.authors : feed.author

                image: isSubscribed ? feed.cachedImage : feed.image
                title: isSubscribed ? feed.name : feed.title
                subtitle: authors ? KI18n.i18nc("by <author(s)>", "by %1", authors) : ""
            }

            // header actions
            Controls.Control {
                Layout.fillWidth: true

                leftPadding: Kirigami.Units.largeSpacing
                rightPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.smallSpacing

                background: Rectangle {
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.colorSet: Kirigami.Theme.Header
                    color: Kirigami.Theme.backgroundColor
                }

                contentItem: Kirigami.ActionToolBar {
                    id: feedToolBar
                    alignment: Qt.AlignLeft
                    background: Item {}

                    // HACK: ActionToolBar loads buttons dynamically, and so the
                    // height calculation changes the position
                    onHeightChanged: entryList.contentY = entryList.originY

                    actions: [
                        Kirigami.Action {
                            visible: isSubscribed
                            icon.name: "view-refresh"
                            text: KI18n.i18n("Refresh Podcast")
                            onTriggered: root.refreshing = true
                        },
                        Kirigami.Action {
                            icon.name: "kt-add-feeds"
                            text: enabled ? KI18n.i18n("Subscribe") : KI18n.i18n("Subscribed")
                            enabled: !DataManager.feedExists(feed.url)
                            visible: !isSubscribed
                            onTriggered: {
                                DataManager.addFeed(feed.url);
                                enabled = false;
                                // Also disable button on discoverpage
                                if (subscribeAction !== undefined) {
                                    subscribeAction.enabled = false;
                                }
                            }
                        }
                    ]

                    // add the default actions through onCompleted to add them
                    // to the ones defined above
                    Component.onCompleted: {
                        if (isSubscribed) {
                            for (var i in entryList.defaultActionList) {
                                feedToolBar.actions.push(entryList.defaultActionList[i]);
                            }
                        }
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                height: visible ? implicitHeight : 0
                visible: entryList.count === 0 && isSubscribed

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent

                    width: Kirigami.Units.gridUnit * 20

                    text: feed.errorId === 0 ? KI18n.i18n("No episodes available") : KI18n.i18n("Error (%1): %2", feed.errorId, feed.errorString)
                    icon.name: feed.errorId === 0 ? "" : "data-error"
                }
            }
        }

        FilterInlineMessage {
            proxyModel: root.feed.entries ? root.feed.entries : emptyListModel
        }
    }
}
