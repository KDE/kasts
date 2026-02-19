/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

GridView {
    id: root
    currentIndex: -1
    clip: true

    required property Kirigami.Action addAction
    required property Kirigami.Action importAction

    Kirigami.PlaceholderMessage {
        id: placeholderMessage
        visible: root.count === 0
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
                onTriggered: (root.Controls.ApplicationWindow.window as Main).pushPage("DiscoverPage")
            }
        }

        Controls.Button {
            id: addButton
            visible: feedsModel.searchFilter === ""
            Layout.preferredWidth: placeholderMessage.buttonSize
            Layout.alignment: Qt.AlignHCenter
            action: root.addAction
        }

        Controls.Button {
            id: importButton
            visible: feedsModel.searchFilter === ""
            Layout.preferredWidth: placeholderMessage.buttonSize
            Layout.alignment: Qt.AlignHCenter
            action: root.importAction
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
                    // not using pushPage here in order to open the sync page directly
                    (root.Controls.ApplicationWindow.window as Main).settingsView.open('Synchronization');
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
    property int availableWidth: root.width - !Kirigami.Settings.isMobile * Kirigami.Units.gridUnit * 1.3

    // TODO: get proper width for scrollbar rather than hardcoding it

    property int columns: Math.max(1, Math.floor(availableWidth / (minimumCardSize + 2 * cardMargin)))

    cellWidth: availableWidth / columns
    cellHeight: availableWidth / columns

    model: FeedsProxyModel {
        id: feedsModel
        sortType: root.Controls.ApplicationWindow.window ? (root.Controls.ApplicationWindow.window as Main).feedSorting : 0
    }

    delegate: FeedListDelegate {
        cardSize: root.availableWidth / root.columns - 2 * root.cardMargin
        cardMargin: root.cardMargin
        listView: root
    }

    property list<var> selectionForContextMenu: []
    property ItemSelectionModel selectionModel: ItemSelectionModel {
        id: selectionModel
        model: root.model
        onSelectionChanged: {
            root.selectionForContextMenu = selectedIndexes;
        }
    }

    // The selection is not updated when the model is reset, so we have to take
    // this into account manually.
    // TODO: Fix the fact that the current item is not highlighted after reset
    Connections {
        target: root.model
        function onModelAboutToBeReset(): void {
            root.selectionForContextMenu = [];
            root.selectionModel.clear();
            root.selectionModel.setCurrentIndex(root.model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
            root.currentIndex = 0;
        }
    }

    Keys.onPressed: event => {
        if (event.matches(StandardKey.SelectAll)) {
            root.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
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
                if ((contentY - root.height) < 0) {
                    contentY = 0;
                } else {
                    contentY -= root.height;
                }
                returnToBounds();
            }
            return;
        case Qt.Key_PageDown:
            if (!atYEnd) {
                if ((contentY + root.height) > contentHeight - height) {
                    contentY = contentHeight - height;
                } else {
                    contentY += root.height;
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
        var nextRow = root.currentIndex + delta;
        if (nextRow < 0) {
            nextRow = root.currentIndex;
        }
        if (nextRow >= root.count) {
            nextRow = root.currentIndex;
        }
        if (append) {
            root.selectionModel.select(root.model.createSelection(nextRow, root.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        } else {
            root.selectionModel.setCurrentIndex(model.index(nextRow, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
    }

    // For lack of a better place, we put generic entry list actions here so
    // they can be re-used across the different ListViews.
    property Kirigami.Action selectAllAction: Kirigami.Action {
        icon.name: "edit-select-all"
        text: KI18n.i18nc("@action:intoolbar", "Select All")
        visible: true
        onTriggered: {
            root.selectionModel.select(root.model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
        }
    }

    property Kirigami.Action selectNoneAction: Kirigami.Action {
        icon.name: "edit-select-none"
        text: KI18n.i18nc("@action:intoolbar", "Deselect All")
        visible: root.selectionModel.hasSelection
        onTriggered: {
            root.selectionModel.clearSelection();
        }
    }

    property Kirigami.Action deleteFeedAction: Kirigami.Action {
        icon.name: "delete"
        text: KI18n.i18ncp("@action:intoolbar", "Remove Podcast", "Remove Podcasts", root.selectionForContextMenu.length)
        visible: root.selectionModel.hasSelection
        onTriggered: {
            // First get an array of pointers to the feeds to be deleted
            // because the selected QModelIndexes will no longer be valid
            // after we start deleting feeds.
            var feeds = [];
            for (var i in root.selectionForContextMenu) {
                feeds[i] = root.model.data(root.selectionForContextMenu[i], FeedsModel.FeedRole);
            }
            var appPageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
            for (var i in feeds) {
                if ((root.Controls.ApplicationWindow.window as Main).lastFeed === feeds[i].url) {
                    while (appPageStack.depth > 1) {
                        appPageStack.pop();
                    }
                }
            }
            DataManager.removeFeeds(feeds);
        }
    }

    property Kirigami.Action feedDetailsAction: Kirigami.Action {
        icon.name: "documentinfo"
        text: KI18n.i18nc("@action:intoolbar Open view with more podcast details", "Podcast Details")
        visible: root.selectionModel.hasSelection && (root.selectionForContextMenu.length == 1)
        onTriggered: {
            var appPageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
            while (appPageStack.depth > 1)
                appPageStack.pop();
            appPageStack.push(Qt.createComponent("org.kde.kasts", "FeedDetailsPage"), {
                feed: root.selectionForContextMenu[0].model.data(root.selectionForContextMenu[0], FeedsModel.FeedRole)
            });
        }
    }

    property list<Kirigami.Action> contextualActionList: [feedDetailsAction, deleteFeedAction, selectAllAction, selectNoneAction]

    property Controls.Menu contextMenu: Controls.Menu {
        id: contextMenu

        Controls.MenuItem {
            action: root.feedDetailsAction
            visible: (root.selectionForContextMenu.length == 1)
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.deleteFeedAction
            visible: true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        onClosed: {
            // reset to normal selection if this context menu is closed
            root.selectionForContextMenu = root.selectionModel.selectedIndexes;
        }
    }
}
