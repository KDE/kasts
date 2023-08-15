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

import org.kde.kasts

ListView {
    id: listView
    clip: true
    property bool isQueue: false
    property bool isDownloads: false

    property var selectionForContextMenu: []
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
        function onModelAboutToBeReset() {
            selectionForContextMenu = [];
            listView.selectionModel.clear();
            listView.selectionModel.setCurrentIndex(model.index(0, 0), ItemSelectionModel.Current); // Only set current item; don't select it
            currentIndex = 0;
        }
    }

    Keys.onPressed: (event) => {
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
                        contentY = 0
                    } else {
                        contentY -= listView.height
                    }
                    returnToBounds()
                }
                return;
            case Qt.Key_PageDown:
                if (!atYEnd) {
                    if ((contentY + listView.height) > contentHeight - height) {
                        contentY = contentHeight - height
                    } else {
                        contentY += listView.height
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

    readonly property var sortAction: Kirigami.Action {
        id: sortActionRoot
        visible: !isDownloads
        enabled: visible
        icon.name: "view-sort"
        text: i18nc("@action:intoolbar Open menu with options to sort episodes", "Sort")

        tooltip: i18nc("@info:tooltip", "Select how to sort episodes")

        property Controls.ActionGroup sortGroup: Controls.ActionGroup { }

        property Instantiator repeater: Instantiator {
            model: ListModel {
                id: sortModel
                // have to use script because i18n doesn't work within ListElement
                Component.onCompleted: {
                    if (sortActionRoot.visible) {
                        var sortList = [AbstractEpisodeProxyModel.DateDescending,
                                        AbstractEpisodeProxyModel.DateAscending]
                        for (var i in sortList) {
                            sortModel.append({"name": listView.model.getSortName(sortList[i]),
                                            "iconName": listView.model.getSortIconName(sortList[i]),
                                            "sortType": sortList[i]});
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

    readonly property var filterAction: Kirigami.Action {
        id: filterActionRoot
        visible: !isDownloads && !isQueue
        enabled: visible
        icon.name: "view-filter"
        text: i18nc("@action:intoolbar Button to open menu to filter episodes based on their status (played, new, etc.)", "Filter")

        tooltip: i18nc("@info:tooltip", "Filter episodes by status")

        property Controls.ActionGroup filterGroup: Controls.ActionGroup { }


        property Instantiator repeater: Instantiator {
            model: ListModel {
                id: filterModel
                // have to use script because i18n doesn't work within ListElement
                Component.onCompleted: {
                    if (filterActionRoot.visible) {
                        var filterList = [AbstractEpisodeProxyModel.NoFilter,
                                        AbstractEpisodeProxyModel.ReadFilter,
                                        AbstractEpisodeProxyModel.NotReadFilter,
                                        AbstractEpisodeProxyModel.NewFilter,
                                        AbstractEpisodeProxyModel.NotNewFilter,
                                        AbstractEpisodeProxyModel.FavoriteFilter,
                                        AbstractEpisodeProxyModel.NotFavoriteFilter]
                        for (var i in filterList) {
                            filterModel.append({"name": listView.model.getFilterName(filterList[i]),
                                                "filterType": filterList[i]});
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

    readonly property var selectAllAction: Kirigami.Action {
        icon.name: "edit-select-all"
        text: i18n("Select All")
        visible: true
        onTriggered: {
            listView.selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Columns);
        }
    }

    readonly property var selectNoneAction: Kirigami.Action {
        icon.name: "edit-select-none"
        text: i18n("Deselect All")
        visible: listView.selectionModel.hasSelection
        onTriggered: {
            listView.selectionModel.clearSelection();
        }
    }

    readonly property var addToQueueAction: Kirigami.Action {
        text: i18n("Add to Queue")
        icon.name: "media-playlist-append"
        visible: listView.selectionModel.hasSelection && !listView.isQueue && (singleSelectedEntry ? !singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection && !listView.isQueue
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(true, selectionForContextMenu);
        }
    }

    readonly property var removeFromQueueAction: Kirigami.Action {
        text: i18n("Remove from Queue")
        icon.name: "list-remove"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? singleSelectedEntry.queueStatus : true)
        //visible: listView.selectionModel.hasSelection
        onTriggered: {
            DataManager.bulkQueueStatusByIndex(false, selectionForContextMenu);
        }
    }

    readonly property var markPlayedAction: Kirigami.Action {
        text: i18n("Mark as Played")
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? !singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(true, selectionForContextMenu);
        }
    }

    readonly property var markNotPlayedAction: Kirigami.Action {
        text: i18n("Mark as Unplayed")
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? singleSelectedEntry.read : true)
        onTriggered: {
            DataManager.bulkMarkReadByIndex(false, selectionForContextMenu);
        }
    }

    readonly property var markNewAction: Kirigami.Action {
        text: i18n("Label as \"New\"")
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? !singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(true, selectionForContextMenu);
        }
    }

    readonly property var markNotNewAction: Kirigami.Action {
        text: i18n("Remove \"New\" Label")
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? singleSelectedEntry.new : true)
        onTriggered: {
            DataManager.bulkMarkNewByIndex(false, selectionForContextMenu);
        }
    }

    readonly property var markFavoriteAction: Kirigami.Action {
        text: i18nc("@action:intoolbar Button to add a podcast episode as favorite", "Add to Favorites")
        icon.name: "starred-symbolic"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? !singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(true, selectionForContextMenu);
        }
    }

    readonly property var markNotFavoriteAction: Kirigami.Action {
        text: i18nc("@action:intoolbar Button to remove the \"favorite\" property of a podcast episode", "Remove from Favorites")
        icon.name: "non-starred-symbolic"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? singleSelectedEntry.favorite : true)
        onTriggered: {
            DataManager.bulkMarkFavoriteByIndex(false, selectionForContextMenu);
        }
    }

    readonly property var downloadEnclosureAction: Kirigami.Action {
        text: i18n("Download")
        icon.name: "download"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true)
        onTriggered: {
            downloadOverlay.selection = selectionForContextMenu;
            downloadOverlay.run();
        }
    }

    readonly property var deleteEnclosureAction: Kirigami.Action {
        text: i18ncp("context menu action", "Delete Download", "Delete Downloads", selectionForContextMenu.length)
        icon.name: "delete"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true)
        onTriggered: {
            DataManager.bulkDeleteEnclosuresByIndex(selectionForContextMenu);
        }
    }

    readonly property var streamAction: Kirigami.Action {
        text: i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
        icon.name: "media-playback-cloud"
        visible: listView.selectionModel.hasSelection && (singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : false)
        onTriggered: {
            if (!singleSelectedEntry.queueStatus) {
                singleSelectedEntry.queueStatus = true;
            }
            AudioManager.entry = singleSelectedEntry;
            AudioManager.play();
        }
    }

    readonly property var defaultActionList: [sortAction,
                                              filterAction,
                                              addToQueueAction,
                                              removeFromQueueAction,
                                              markPlayedAction,
                                              markNotPlayedAction,
                                              markNewAction,
                                              markNotNewAction,
                                              markFavoriteAction,
                                              markNotFavoriteAction,
                                              downloadEnclosureAction,
                                              deleteEnclosureAction,
                                              streamAction,
                                              selectAllAction,
                                              selectNoneAction]

    property Controls.Menu contextMenu: Controls.Menu {
        id: contextMenu

        Controls.MenuItem {
            action: listView.addToQueueAction
            visible: !listView.isQueue && (singleSelectedEntry ? !singleSelectedEntry.queueStatus : true)
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.removeFromQueueAction
            visible: singleSelectedEntry ? singleSelectedEntry.queueStatus : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markPlayedAction
            visible: singleSelectedEntry ? !singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotPlayedAction
            visible: singleSelectedEntry ? singleSelectedEntry.read : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNewAction
            visible: singleSelectedEntry ? !singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotNewAction
            visible: singleSelectedEntry ? singleSelectedEntry.new : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
         }
        Controls.MenuItem {
            action: listView.markFavoriteAction
            visible: singleSelectedEntry ? !singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.markNotFavoriteAction
            visible: singleSelectedEntry ? singleSelectedEntry.favorite : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
         }
        Controls.MenuItem {
            action: listView.downloadEnclosureAction
            visible: singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.deleteEnclosureAction
            visible: singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloadable : false) : true
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
        }
        Controls.MenuItem {
            action: listView.streamAction
            visible: singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? (singleSelectedEntry.enclosure.status !== Enclosure.Downloaded && NetworkConnectionManager.streamingAllowed) : false) : false
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
         }
        onClosed: {
            // reset to normal selection if this context menu is closed
            listView.selectionForContextMenu = listView.selectionModel.selectedIndexes;
        }
    }
}
