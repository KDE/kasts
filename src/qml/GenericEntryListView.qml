/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.2
import QtQml.Models 2.15

import org.kde.kirigami 2.13 as Kirigami

import org.kde.kasts 1.0

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

    Keys.onPressed: {
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

    readonly property var defaultActionList: [addToQueueAction,
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
            visible: singleSelectedEntry ? (singleSelectedEntry.hasEnclosure ? singleSelectedEntry.enclosure.status !== Enclosure.Downloaded : false) : false
            height: visible ? implicitHeight : 0 // workaround for qqc2-breeze-style
         }
        onClosed: {
            // reset to normal selection if this context menu is closed
            listView.selectionForContextMenu = listView.selectionModel.selectedIndexes;
        }
    }
}
