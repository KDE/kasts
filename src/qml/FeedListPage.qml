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
import QtQml.Models 2.15

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
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    actions.main: Kirigami.Action {
        text: i18n("Add Podcast")
        iconName: "list-add"
        onTriggered: {
            addSheet.open()
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

    // add the default actions through onCompleted to add them to the ones
    // defined above
    Component.onCompleted: {
        for (var i in feedList.contextualActionList) {
            contextualActions.push(feedList.contextualActionList[i]);
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

        model: FeedsProxyModel {
            id: feedsModel
        }

        delegate: FeedListDelegate {
            cardSize: feedList.availableWidth / feedList.columns - 2 * feedList.cardMargin
            cardMargin: feedList.cardMargin
            listView: feedList
        }

        property var selectionForContextMenu: []
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
            function onModelAboutToBeReset() {
                selectionForContextMenu = [];
                feedList.selectionModel.clear();
                feedList.selectionModel.setCurrentIndex(model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
                currentIndex = 0;
            }
        }

        Keys.onPressed: {
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
                            contentY = 0
                        } else {
                            contentY -= feedList.height
                        }
                        returnToBounds()
                    }
                    return;
                case Qt.Key_PageDown:
                    if (!atYEnd) {
                        if ((contentY + feedList.height) > contentHeight - height) {
                            contentY = contentHeight - height
                        } else {
                            contentY += feedList.height
                        }
                        returnToBounds()
                    }
                    return;
                case Qt.Key_Home:
                    if (!atYBeginning) {
                        contentY = 0
                        returnToBounds()
                    }
                    return;
                case Qt.Key_End:
                    if (!atYEnd) {
                        contentY = contentHeight - height
                        returnToBounds()
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

        function selectRelative(delta, append) {
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
        property var selectAllAction: Kirigami.Action {
            iconName: "edit-select-all"
            text: i18n("Select All")
            visible: true
            onTriggered: {
                feedList.selectionModel.select(feedList.model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
            }
        }

        property var selectNoneAction: Kirigami.Action {
            iconName: "edit-select-none"
            text: i18n("Deselect All")
            visible: feedList.selectionModel.hasSelection
            onTriggered: {
                feedList.selectionModel.clearSelection();
            }
        }

        property var deleteFeedAction: Kirigami.Action {
            iconName: "delete"
            text: i18ncp("context menu action", "Remove Podcast", "Remove Podcasts", feedList.selectionForContextMenu.length)
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

        property var feedDetailsAction: Kirigami.Action {
            iconName: "help-about-symbolic"
            text: i18n("Podcast Details")
            visible: feedList.selectionModel.hasSelection && (feedList.selectionForContextMenu.length == 1)
            onTriggered: {
                while(pageStack.depth > 1)
                    pageStack.pop();
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feedList.selectionForContextMenu[0].model.data(feedList.selectionForContextMenu[0], FeedsModel.FeedRole)});
            }
        }

        property var contextualActionList: [feedDetailsAction,
                                            deleteFeedAction,
                                            selectAllAction,
                                            selectNoneAction]

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
