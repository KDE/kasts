/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

ListView {
    id: listView
    clip: true
    property bool isQueue: false
    property bool isDownloads: false

    property list<var> selectionForContextMenu: []
    property var singleSelectedEntry: undefined
    property ItemSelectionModel selectionModel: ItemSelectionModel {
        id: selectionModel
        model: listView.model
        onSelectionChanged: {
            selectionForContextMenu = selectedIndexes;
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
        target: listView.model
        function onModelAboutToBeReset(): void {
            listView.selectionForContextMenu = [];
            listView.selectionModel.clear();
            listView.selectionModel.setCurrentIndex(listView.selectionModel.model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
            listView.currentIndex = 0;
        }
    }

    Keys.onPressed: event => {
        if (event.matches(StandardKey.SelectAll)) {
            listView.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
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
                if ((contentY - listView.height) < 0) {
                    contentY = 0;
                } else {
                    contentY -= listView.height;
                }
                returnToBounds();
            }
            return;
        case Qt.Key_PageDown:
            if (!atYEnd) {
                if ((contentY + listView.height) > contentHeight - height) {
                    contentY = contentHeight - height;
                } else {
                    contentY += listView.height;
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
        var nextRow = listView.currentIndex + delta;
        if (nextRow < 0) {
            nextRow = 0;
        }
        if (nextRow >= listView.count) {
            nextRow = listView.count - 1;
        }
        if (append) {
            listView.selectionModel.select(listView.model.createSelection(nextRow, listView.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        } else {
            listView.selectionModel.setCurrentIndex(model.index(nextRow, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
    }

    // For lack of a better place, we put generic entry list actions here so
    // they can be re-used across the different ListViews.

    readonly property Kirigami.Action sortAction: Kirigami.Action {
        id: sortActionRoot
        visible: !isDownloads
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
                                "name": listView.model.getSortName(sortList[i]),
                                "iconName": listView.model.getSortIconName(sortList[i]),
                                "sortType": sortList[i]
                            });
                        }
                    }
                }
            }

            Kirigami.Action {
                visible: sortActionRoot.visible
                icon.name: model.iconName
                text: model.name
                checkable: !isQueue
                checked: !isQueue && (listView.model.sortType === model.sortType)
                Controls.ActionGroup.group: isQueue ? null : sortActionRoot.sortGroup

                onTriggered: {
                    if (isQueue) {
                        DataManager.sortQueue(model.sortType);
                    } else {
                        listView.model.sortType = model.sortType;
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
        visible: !listView.isDownloads && !listView.isQueue
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
                                name: listView.model.getFilterName(filterList[i]),
                                filterType: filterList[i]
                            });
                        }
                    }
                }
            }

            Kirigami.Action {
                visible: filterActionRoot.visible
                text: model.name
                checkable: true
                checked: listView.model.filterType === model.filterType
                Controls.ActionGroup.group: filterActionRoot.filterGroup

                onTriggered: {
                    listView.model.filterType = model.filterType;
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
            listView.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
        }
    }

    readonly property Kirigami.Action selectNoneAction: Kirigami.Action {
        icon.name: "edit-select-none"
        text: KI18n.i18n("Deselect All")
        visible: listView.selectionModel.hasSelection
        onTriggered: {
            listView.selectionModel.clearSelection();
        }
    }

    readonly property Kirigami.Action addToQueueAction: Kirigami.Action {
        text: KI18n.i18n("Add to Queue")
        icon.name: "media-playlist-append"
        visible: listView.selectionModel.hasSelection && !listView.isQueue && (listView.singleSelectedEntry ? !listView.singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection && !listView.isQueue
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(true, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action removeFromQueueAction: Kirigami.Action {
        text: KI18n.i18n("Remove from Queue")
        icon.name: "list-remove"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? listView.singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(false, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markPlayedAction: Kirigami.Action {
        text: KI18n.i18n("Mark as Played")
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? !listView.singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(true, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotPlayedAction: Kirigami.Action {
        text: KI18n.i18n("Mark as Unplayed")
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? listView.singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(false, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNewAction: Kirigami.Action {
        text: KI18n.i18n("Label as \"New\"")
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? !listView.singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(true, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotNewAction: Kirigami.Action {
        text: KI18n.i18n("Remove \"New\" Label")
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? listView.singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(false, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markFavoriteAction: Kirigami.Action {
        text: KI18n.i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
        icon.name: "starred-symbolic"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? !listView.singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(true, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action markNotFavoriteAction: Kirigami.Action {
        text: KI18n.i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites")
        icon.name: "non-starred-symbolic"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? listView.singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(false, listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action downloadEnclosureAction: Kirigami.Action {
        text: KI18n.i18n("Download")
        icon.name: "download"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true)
        onTriggered: {
            downloadOverlay.selection = selectionForContextMenu;
            downloadOverlay.run();
        }
    }

    readonly property Kirigami.Action deleteEnclosureAction: Kirigami.Action {
        text: KI18n.i18ncp("context menu action", "Delete Download", "Delete Downloads", listView.selectionForContextMenu.length)
        icon.name: "delete"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true)
        onTriggered: {
            DataManager.bulkDeleteEnclosuresByIndex(listView.selectionForContextMenu);
        }
    }

    readonly property Kirigami.Action streamAction: Kirigami.Action {
        text: KI18n.i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
        icon.name: "media-playback-cloud"
        visible: listView.selectionModel.hasSelection && (listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : false)
        onTriggered: {
            if (!listView.singleSelectedEntry.queueStatus) {
                singleSelectedEntry.queueStatus = true;
            }
            AudioManager.entryuid = singleSelectedEntry.entryuid;
            AudioManager.play();
        }
    }

    readonly property list<Kirigami.Action> defaultActionList: [sortAction, filterAction, addToQueueAction, removeFromQueueAction, markPlayedAction, markNotPlayedAction, markNewAction, markNotNewAction, markFavoriteAction, markNotFavoriteAction, downloadEnclosureAction, deleteEnclosureAction, streamAction, selectAllAction, selectNoneAction]

    property Controls.Menu contextMenu: Controls.Menu {
        id: contextMenu

        Controls.MenuItem {
            action: listView.addToQueueAction
            visible: !listView.isQueue && (listView.singleSelectedEntry ? !listView.singleSelectedEntry.queueStatus : true)
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.removeFromQueueAction
            visible: listView.singleSelectedEntry ? listView.singleSelectedEntry.queueStatus : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markPlayedAction
            visible: listView.singleSelectedEntry ? !listView.singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotPlayedAction
            visible: listView.singleSelectedEntry ? listView.singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNewAction
            visible: listView.singleSelectedEntry ? !listView.singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotNewAction
            visible: listView.singleSelectedEntry ? listView.singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markFavoriteAction
            visible: listView.singleSelectedEntry ? !listView.singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotFavoriteAction
            visible: listView.singleSelectedEntry ? listView.singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.downloadEnclosureAction
            visible: listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? listView.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.deleteEnclosureAction
            visible: listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? listView.singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.streamAction
            visible: listView.singleSelectedEntry ? (listView.singleSelectedEntry.hasEnclosure ? (listView.singleSelectedEntry.enclosure.status !== Enclosure.Downloaded && NetworkConnectionManager.streamingAllowed) : false) : false
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        onClosed: {
            // reset to normal selection if this context menu is closed
            listView.selectionForContextMenu = listView.selectionModel.selectedIndexes;
        }
    }
}
