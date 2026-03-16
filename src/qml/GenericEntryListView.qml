/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

ListView {
    id: root
    clip: true
    property bool isQueue: false
    property bool isDownloads: false

    property list<var> selectionForContextMenu: []
    property var singleSelectedEntry: undefined
    property ItemSelectionModel selectionModel: ItemSelectionModel {
        model: root.model
        onSelectionChanged: {
            root.selectionForContextMenu = selectedIndexes;
        }
    }

    topMargin: Math.round(Kirigami.Units.smallSpacing / 2)
    currentIndex: -1

    onSelectionForContextMenuChanged: {
        if (selectionForContextMenu.length === 1) {
            singleSelectedEntry = selectionForContextMenu[0].model.data(selectionForContextMenu[0], AbstractEpisodeModel.EntryRole);
        } else {
            singleSelectedEntry = undefined;
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
            root.selectionModel.setCurrentIndex(root.selectionModel.model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
            root.currentIndex = 0;
        }
    }

    Keys.onPressed: event => {
        if (event.matches(StandardKey.SelectAll)) {
            root.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
            return;
        }
        switch (event.key) {
        case Qt.Key_Up:
            selectRelative(-1, event.modifiers == Qt.ShiftModifier);
            return;
        case Qt.Key_Down:
            selectRelative(1, event.modifiers == Qt.ShiftModifier);
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
            nextRow = 0;
        }
        if (nextRow >= root.count) {
            nextRow = root.count - 1;
        }
        if (append) {
            root.selectionModel.select(root.model.createSelection(nextRow, root.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        } else {
            root.selectionModel.setCurrentIndex(model.index(nextRow, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
    }

    // For lack of a better place, we put generic entry list actions here so
    // they can be re-used across the different ListViews.

    readonly property Kirigami.Action sortAction: Kirigami.Action {
        id: sortActionRoot
        visible: !root.isDownloads
        enabled: visible
        icon.name: "view-sort"
        text: KI18n.i18nc("@action:intoolbar Open menu with options to sort episodes", "Sort")

        tooltip: KI18n.i18nc("@info:tooltip", "Select how to sort episodes")

        property Controls.ActionGroup sortGroup: Controls.ActionGroup {}

        property Instantiator repeater: Instantiator {
            model: ListModel {
                id: sortModel
                // have to use script because KI18n.i18n doesn't work within ListElement
                Component.onCompleted: {
                    if (sortActionRoot.visible) {
                        var sortList = [AbstractEpisodeProxyModel.DateDescending, AbstractEpisodeProxyModel.DateAscending];
                        for (var i in sortList) {
                            sortModel.append({
                                "name": root.model.getSortName(sortList[i]),
                                "iconName": root.model.getSortIconName(sortList[i]),
                                "sortType": sortList[i]
                            });
                        }
                    }
                }
            }

            Kirigami.Action {
                required property string iconName
                required property string name
                required property int sortType

                visible: sortActionRoot.visible
                icon.name: iconName
                text: name
                checkable: !root.isQueue
                checked: !root.isQueue && (root.model.sortType === sortType)
                Controls.ActionGroup.group: root.isQueue ? null : sortActionRoot.sortGroup

                onTriggered: {
                    if (root.isQueue) {
                        QueueModel.sortQueue(sortType);
                    } else {
                        root.model.sortType = sortType;
                    }
                }
            }

            onObjectAdded: (index, object) => {
                sortActionRoot.children.push(object);
            }
        }
    }

    readonly property Kirigami.Action filterAction: Kirigami.Action {
        id: filterActionRoot
        visible: !root.isDownloads && !root.isQueue
        enabled: visible
        icon.name: "view-filter"
        text: KI18n.i18nc("@action:intoolbar Button to open menu to filter episodes based on their status (played, new, etc.)", "Filter")

        tooltip: KI18n.i18nc("@info:tooltip", "Filter episodes by status")

        property Controls.ActionGroup filterGroup: Controls.ActionGroup {}

        property Instantiator repeater: Instantiator {
            model: ListModel {
                id: filterModel
                // have to use script because KI18n.i18n doesn't work within ListElement
                Component.onCompleted: {
                    if (filterActionRoot.visible) {
                        var filterList = [AbstractEpisodeProxyModel.NoFilter, AbstractEpisodeProxyModel.ReadFilter, AbstractEpisodeProxyModel.NotReadFilter, AbstractEpisodeProxyModel.NewFilter, AbstractEpisodeProxyModel.NotNewFilter, AbstractEpisodeProxyModel.FavoriteFilter, AbstractEpisodeProxyModel.NotFavoriteFilter];
                        for (var i in filterList) {
                            filterModel.append({
                                name: root.model.getFilterName(filterList[i]),
                                filterType: filterList[i]
                            });
                        }
                    }
                }
            }

            Kirigami.Action {
                required property string name
                required property int filterType

                visible: filterActionRoot.visible
                text: name
                checkable: true
                checked: root.model.filterType === filterType
                Controls.ActionGroup.group: filterActionRoot.filterGroup

                onTriggered: {
                    root.model.filterType = filterType;
                }
            }

            onObjectAdded: (index, object) => {
                filterActionRoot.children.push(object);
            }
        }
    }

    readonly property Kirigami.Action selectAllAction: Kirigami.Action {
        icon.name: "edit-select-all"
        text: KI18n.i18n("Select All")
        visible: true
        onTriggered: {
            root.selectionModel.select(root.model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
        }
    }

    readonly property Kirigami.Action selectNoneAction: Kirigami.Action {
        icon.name: "edit-select-none"
        text: KI18n.i18n("Deselect All")
        visible: root.selectionModel.hasSelection
        onTriggered: {
            root.selectionModel.clearSelection();
        }
    }

    readonly property Kirigami.Action addToQueueAction: Kirigami.Action {
        text: KI18n.i18n("Add to Queue")
        icon.name: "media-playlist-append"
        visible: root.selectionModel.hasSelection && !root.isQueue && (root.singleSelectedEntry ? !root.singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection && !listView.isQueue
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(true, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action removeFromQueueAction: Kirigami.Action {
        text: KI18n.i18n("Remove from Queue")
        icon.name: "list-remove"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? root.singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(false, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markPlayedAction: Kirigami.Action {
        text: KI18n.i18n("Mark as Played")
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? !root.singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(true, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotPlayedAction: Kirigami.Action {
        text: KI18n.i18n("Mark as Unplayed")
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? root.singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(false, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNewAction: Kirigami.Action {
        text: KI18n.i18n("Label as \"New\"")
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? !root.singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(true, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotNewAction: Kirigami.Action {
        text: KI18n.i18n("Remove \"New\" Label")
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? root.singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(false, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markFavoriteAction: Kirigami.Action {
        text: KI18n.i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
        icon.name: "starred-symbolic"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? !root.singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(true, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotFavoriteAction: Kirigami.Action {
        text: KI18n.i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites")
        icon.name: "non-starred-symbolic"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? root.singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(false, root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action downloadEnclosureAction: Kirigami.Action {
        text: KI18n.i18n("Download")
        icon.name: "download"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? root.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true)
        onTriggered: {
            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.selection = root.selectionForContextMenu;
            (root.Controls.ApplicationWindow.window as Main).downloadOverlay.run();
        }
    }

    readonly property Kirigami.Action deleteEnclosureAction: Kirigami.Action {
        text: KI18n.i18ncp("context menu action", "Delete Download", "Delete Downloads", root.selectionForContextMenu.length)
        icon.name: "delete"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? root.singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true)
        onTriggered: {
            DataManager.bulkDeleteEnclosuresByIndex(root.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action streamAction: Kirigami.Action {
        text: KI18n.i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
        icon.name: "media-playback-cloud"
        visible: root.selectionModel.hasSelection && (root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? root.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : false)
        onTriggered: {
            if (!root.singleSelectedEntry.queueStatus) {
                root.singleSelectedEntry.queueStatus = true;
            }
            AudioManager.entryuid = root.singleSelectedEntry.entryuid;
            AudioManager.play();
        }
    }

    readonly property list<Kirigami.Action> defaultActionList: [sortAction, filterAction, addToQueueAction, removeFromQueueAction, markPlayedAction, markNotPlayedAction, markNewAction, markNotNewAction, markFavoriteAction, markNotFavoriteAction, downloadEnclosureAction, deleteEnclosureAction, streamAction, selectAllAction, selectNoneAction]

    property Controls.Menu contextMenu: Controls.Menu {
        id: contextMenu

        Controls.MenuItem {
            action: root.addToQueueAction
            visible: !root.isQueue && (root.singleSelectedEntry ? !root.singleSelectedEntry.queueStatus : true)
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.removeFromQueueAction
            visible: root.singleSelectedEntry ? root.singleSelectedEntry.queueStatus : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markPlayedAction
            visible: root.singleSelectedEntry ? !root.singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markNotPlayedAction
            visible: root.singleSelectedEntry ? root.singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markNewAction
            visible: root.singleSelectedEntry ? !root.singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markNotNewAction
            visible: root.singleSelectedEntry ? root.singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markFavoriteAction
            visible: root.singleSelectedEntry ? !root.singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.markNotFavoriteAction
            visible: root.singleSelectedEntry ? root.singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.downloadEnclosureAction
            visible: root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? root.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.deleteEnclosureAction
            visible: root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? root.singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: root.streamAction
            visible: root.singleSelectedEntry ? (root.singleSelectedEntry.hasEnclosure ? (root.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded && NetworkConnectionManager.streamingAllowed) : false) : false
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        onClosed: {
            // reset to normal selection if this context menu is closed
            root.selectionForContextMenu = root.selectionModel.selectedIndexes;
        }
    }
}
