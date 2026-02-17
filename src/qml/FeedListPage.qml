/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import Qt.labs.platform
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: subscriptionPage
    title: KI18n.i18nc("@title of page with list of podcast subscriptions", "Subscriptions")

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    anchors.margins: 0
    padding: 0

    property string lastFeed: ""

    supportsRefreshing: true
    onRefreshingChanged: {
        if (refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            visible: Kirigami.Settings.isMobile
            text: KI18n.i18nc("@title of page allowing to search for new podcasts online", "Discover")
            icon.name: "search"
            onTriggered: {
                applicationWindow().pageStack.push(Qt.createComponent("org.kde.kasts", "DiscoverPage"));
            }
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:intoolbar", "Refresh All Podcasts")
            icon.name: "view-refresh"
            onTriggered: refreshing = true
        },
        Kirigami.Action {
            id: addAction
            text: KI18n.i18nc("@action:intoolbar", "Add Podcast…")
            icon.name: "list-add"
            onTriggered: {
                addSheet.open();
            }
        },
        Kirigami.Action {
            id: sortActionRoot
            icon.name: "view-sort"
            text: KI18n.i18nc("@action:intoolbar Open menu with options to sort subscriptions", "Sort")

            tooltip: KI18n.i18nc("@info:tooltip", "Select how to sort subscriptions")

            property Controls.ActionGroup sortGroup: Controls.ActionGroup {}

            property Instantiator repeater: Instantiator {
                model: ListModel {
                    id: sortModel
                    // have to use script because KI18n.i18n doesn't work within ListElement
                    Component.onCompleted: {
                        if (sortActionRoot.visible) {
                            var sortList = [FeedsProxyModel.UnreadDescending, FeedsProxyModel.UnreadAscending, FeedsProxyModel.NewDescending, FeedsProxyModel.NewAscending, FeedsProxyModel.FavoriteDescending, FeedsProxyModel.FavoriteAscending, FeedsProxyModel.TitleAscending, FeedsProxyModel.TitleDescending];
                            for (var i in sortList) {
                                sortModel.append({
                                    name: feedsModel.getSortName(sortList[i]),
                                    iconName: feedsModel.getSortIconName(sortList[i]),
                                    sortType: sortList[i]
                                });
                            }
                        }
                    }
                }

                Kirigami.Action {
                    visible: sortActionRoot.visible
                    icon.name: model.iconName
                    text: model.name
                    checkable: true
                    checked: kastsMainWindow.feedSorting === model.sortType
                    Controls.ActionGroup.group: sortActionRoot.sortGroup

                    onTriggered: {
                        kastsMainWindow.feedSorting = model.sortType;
                    }
                }

                onObjectAdded: (index, object) => {
                    sortActionRoot.children.push(object);
                }
            }
        },
        Kirigami.Action {
            id: searchActionButton
            icon.name: "search"
            text: KI18n.i18nc("@action:intoolbar", "Search")
            checkable: true
        },
        Kirigami.Action {
            id: importAction
            text: KI18n.i18nc("@action:intoolbar", "Import Podcasts…")
            icon.name: "document-import"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: importDialog.open()
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:intoolbar", "Export Podcasts…")
            icon.name: "document-export"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: exportDialog.open()
        }
    ]

    // add the default actions through onCompleted to add them to the ones
    // defined above
    Component.onCompleted: {
        for (var i in feedList.contextualActionList) {
            pageActions.push(feedList.contextualActionList[i]);
        }
    }

    actions: pageActions

    header: Loader {
        anchors.right: parent.right
        anchors.left: parent.left

        active: searchActionButton.checked
        visible: active
        sourceComponent: SearchBar {
            proxyModel: feedsModel
            parentKey: searchActionButton
            showSearchFilters: false
        }
    }

    AddFeedSheet {
        id: addSheet
    }

    FileDialog {
        id: importDialog
        title: KI18n.i18nc("@title:window", "Import Podcasts")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [KI18n.i18nc("@label:listbox File filter option in file dialog", "OPML Files (*.opml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "XML Files (*.xml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "All Files (*)")]
        onAccepted: DataManager.importFeeds(file)
    }

    FileDialog {
        id: exportDialog
        title: KI18n.i18nc("@title:window", "Export Podcasts")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [KI18n.i18nc("@label:listbox File filter option in file dialog", "OPML Files (*.opml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "All Files (*)")]
        onAccepted: DataManager.exportFeeds(file)
        fileMode: FileDialog.SaveFile
    }

    ConnectionCheckAction {
        id: updateAllFeeds
    }

    GridView {
        id: feedList
        currentIndex: -1
        clip: true

        Kirigami.PlaceholderMessage {
            id: placeholderMessage
            visible: feedList.count === 0
            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent
            type: feedsModel.searchFilter === "" ? Kirigami.PlaceholderMessage.Actionable : Kirigami.PlaceholderMessage.Informational
            text: feedsModel.searchFilter === "" ? KI18n.i18nc("@info Placeholder message for empty podcast list", "No podcasts added yet") : KI18n.i18nc("@info Placeholder message for podcast list when no podcast matches the search criteria", "No podcasts found")
            explanation: feedsModel.searchFilter === "" ? KI18n.i18nc("@info:tipoftheday", "Get started by adding podcasts:") : null

            readonly property int buttonSize: Math.max(discoverButton.implicitWidth, addButton.implicitWidth, importButton.implicitWidth, syncButton.implicitWidth)

            // These actions are also in the toolbar, but duplicating some here
            // to give them more descriptive names
            Controls.Button {
                id: discoverButton
                visible: feedsModel.searchFilter === ""
                Layout.preferredWidth: placeholderMessage.buttonSize
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.gridUnit
                action: Kirigami.Action {
                    icon.name: "search"
                    text: KI18n.i18nc("@action:button", "Search Online")
                    onTriggered: pushPage("DiscoverPage")
                }
            }

            Controls.Button {
                id: addButton
                visible: feedsModel.searchFilter === ""
                Layout.preferredWidth: placeholderMessage.buttonSize
                Layout.alignment: Qt.AlignHCenter
                action: addAction
            }

            Controls.Button {
                id: importButton
                visible: feedsModel.searchFilter === ""
                Layout.preferredWidth: placeholderMessage.buttonSize
                Layout.alignment: Qt.AlignHCenter
                action: importAction
            }

            Controls.Button {
                id: syncButton
                visible: feedsModel.searchFilter === ""
                Layout.preferredWidth: placeholderMessage.buttonSize
                Layout.alignment: Qt.AlignHCenter
                action: Kirigami.Action {
                    text: KI18n.i18nc("@action:button", "Synchronize")
                    icon.name: "state-sync"
                    onTriggered: {
                        // not using pushPage here in order to open the sync page
                        // directly
                        settingsView.open('Synchronization');
                    }
                }
            }
        }

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

        model: FeedsProxyModel {
            id: feedsModel
            sortType: kastsMainWindow.feedSorting
        }

        delegate: FeedListDelegate {
            cardSize: feedList.availableWidth / feedList.columns - 2 * feedList.cardMargin
            cardMargin: feedList.cardMargin
            listView: feedList
        }

        property list<var> selectionForContextMenu: []
        property ItemSelectionModel selectionModel: ItemSelectionModel {
            id: selectionModel
            model: feedList.model
            onSelectionChanged: {
                feedList.selectionForContextMenu = selectedIndexes;
            }
        }

        // The selection is not updated when the model is reset, so we have to take
        // this into account manually.
        // TODO: Fix the fact that the current item is not highlighted after reset
        Connections {
            target: feedList.model
            function onModelAboutToBeReset(): void {
                feedList.selectionForContextMenu = [];
                feedList.selectionModel.clear();
                feedList.selectionModel.setCurrentIndex(feedList.model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
                currentIndex = 0;
            }
        }

        Keys.onPressed: event => {
            if (event.matches(StandardKey.SelectAll)) {
                feedList.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
                return;
            }
            switch (event.key) {
            case Qt.Key_Left:
                selectRelative(-1, event.modifiers == Qt.ShiftModifier);
                return;
            case Qt.Key_Right:
                selectRelative(1, event.modifiers == Qt.ShiftModifier);
                return;
            case Qt.Key_Up:
                selectRelative(-columns, event.modifiers == Qt.ShiftModifier);
                return;
            case Qt.Key_Down:
                selectRelative(columns, event.modifiers == Qt.ShiftModifier);
                return;
            case Qt.Key_PageUp:
                if (!atYBeginning) {
                    if ((contentY - feedList.height) < 0) {
                        contentY = 0;
                    } else {
                        contentY -= feedList.height;
                    }
                    returnToBounds();
                }
                return;
            case Qt.Key_PageDown:
                if (!atYEnd) {
                    if ((contentY + feedList.height) > contentHeight - height) {
                        contentY = contentHeight - height;
                    } else {
                        contentY += feedList.height;
                    }
                    returnToBounds();
                }
                return;
            case Qt.Key_Home:
                if (!atYBeginning) {
                    contentY = 0;
                    returnToBounds();
                }
                return;
            case Qt.Key_End:
                if (!atYEnd) {
                    contentY = contentHeight - height;
                    returnToBounds();
                }
                return;
            default:
                break;
            }
        }

        onActiveFocusChanged: {
            if (activeFocus && !selectionModel.hasSelection) {
                selectionModel.clear();
                selectionModel.setCurrentIndex(model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
            }
        }

        function selectRelative(delta: int, append: bool): void {
            var nextRow = feedList.currentIndex + delta;
            if (nextRow < 0) {
                nextRow = feedList.currentIndex;
            }
            if (nextRow >= feedList.count) {
                nextRow = feedList.currentIndex;
            }
            if (append) {
                feedList.selectionModel.select(feedList.model.createSelection(nextRow, feedList.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            } else {
                feedList.selectionModel.setCurrentIndex(model.index(nextRow, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            }
        }

        // For lack of a better place, we put generic entry list actions here so
        // they can be re-used across the different ListViews.
        property Kirigami.Action selectAllAction: Kirigami.Action {
            icon.name: "edit-select-all"
            text: KI18n.i18nc("@action:intoolbar", "Select All")
            visible: true
            onTriggered: {
                feedList.selectionModel.select(feedList.model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
            }
        }

        property Kirigami.Action selectNoneAction: Kirigami.Action {
            icon.name: "edit-select-none"
            text: KI18n.i18nc("@action:intoolbar", "Deselect All")
            visible: feedList.selectionModel.hasSelection
            onTriggered: {
                feedList.selectionModel.clearSelection();
            }
        }

        property Kirigami.Action deleteFeedAction: Kirigami.Action {
            icon.name: "delete"
            text: KI18n.i18ncp("@action:intoolbar", "Remove Podcast", "Remove Podcasts", feedList.selectionForContextMenu.length)
            visible: feedList.selectionModel.hasSelection
            onTriggered: {
                // First get an array of pointers to the feeds to be deleted
                // because the selected QModelIndexes will no longer be valid
                // after we start deleting feeds.
                var feeds = [];
                for (var i in feedList.selectionForContextMenu) {
                    feeds[i] = feedList.model.data(feedList.selectionForContextMenu[i], FeedsModel.FeedRole);
                }
                for (var i in feeds) {
                    if (lastFeed === feeds[i].url) {
                        while (pageStack.depth > 1) {
                            pageStack.pop();
                        }
                    }
                }
                DataManager.removeFeeds(feeds);
            }
        }

        property Kirigami.Action feedDetailsAction: Kirigami.Action {
            icon.name: "documentinfo"
            text: KI18n.i18nc("@action:intoolbar Open view with more podcast details", "Podcast Details")
            visible: feedList.selectionModel.hasSelection && (feedList.selectionForContextMenu.length == 1)
            onTriggered: {
                while (pageStack.depth > 1)
                    pageStack.pop();
                pageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
                    feed: feedList.selectionForContextMenu[0].model.data(feedList.selectionForContextMenu[0], FeedsModel.FeedRole)
                });
            }
        }

        property list<Kirigami.Action> contextualActionList: [feedDetailsAction, deleteFeedAction, selectAllAction, selectNoneAction]

        property Controls.Menu contextMenu: Controls.Menu {
            id: contextMenu

            Controls.MenuItem {
                action: feedList.feedDetailsAction
                visible: (feedList.selectionForContextMenu.length == 1)
                height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
            }
            Controls.MenuItem {
                action: feedList.deleteFeedAction
                visible: true
                height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
            }
            onClosed: {
                // reset to normal selection if this context menu is closed
                feedList.selectionForContextMenu = feedList.selectionModel.selectedIndexes;
            }
        }
    }
}
