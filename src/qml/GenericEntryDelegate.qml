/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as AddonDelegates

import org.kde.kmediasession
import org.kde.kasts
import org.kde.kasts.settings

AddonDelegates.RoundedItemDelegate {
    id: listItem

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    visible: entry ? true : false

    property bool isQueue: false
    property bool isDownloads: false
    property QtObject listViewObject: undefined
    property bool selected: false
    property int row: model ? model.row : -1

    property bool showRemoveFromQueueButton: entry ? (!entry.enclosure && entry.queueStatus) : false
    property bool showDownloadButton: entry ? (entry.enclosure && (!isDownloads || entry.enclosure.status === Enclosure.PartiallyDownloaded) && (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) && (!NetworkConnectionManager.streamingAllowed || !SettingsManager.prioritizeStreaming || isDownloads) && !(AudioManager.entry === entry && AudioManager.playbackState === KMediaSession.PlayingState)) : false
    property bool showCancelDownloadButton: entry ? (entry.enclosure && entry.enclosure.status === Enclosure.Downloading) : false
    property bool showDeleteDownloadButton: entry ? (isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded) : false
    property bool showAddToQueueButton: entry ? (!isDownloads && !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded) : false
    property bool showPlayButton: entry ? (!isDownloads && entry.queueStatus && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded) && (AudioManager.entry !== entry || AudioManager.playbackState !== KMediaSession.PlayingState)) : false
    property bool showStreamingPlayButton: entry ? (!isDownloads && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded && entry.enclosure.status !== Enclosure.Downloading && NetworkConnectionManager.streamingAllowed && SettingsManager.prioritizeStreaming) && (AudioManager.entry !== entry || AudioManager.playbackState !== KMediaSession.PlayingState)) : false
    property bool showPauseButton: entry ? (!isDownloads && entry.queueStatus && entry.enclosure && (AudioManager.entry === entry && AudioManager.playbackState === KMediaSession.PlayingState)) : false

    component IconOnlyButton : Controls.ToolButton {
        display: Controls.ToolButton.IconOnly

        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
     }

    highlighted: selected

    Accessible.role: Accessible.Button
    Accessible.name: entry ? entry.title : ""
    Accessible.onPressAction: {
         delegateTapped();
    }

    Keys.onReturnPressed: {
        delegateTapped();
    }

    // We need to update the "selected" status:
    // - if the selected indexes changes
    // - if our delegate moves
    // - if the model moves and the delegate stays in the same place
    function updateIsSelected() {
        selected = listViewObject.selectionModel.rowIntersectsSelection(row);
    }

    onRowChanged: {
        updateIsSelected();
    }

    Component.onCompleted: {
        updateIsSelected();
    }

    function delegateTapped() {
        // only mark pure rss feeds as read + not new;
        // podcasts should only be marked read once they have been listened to, and only
        // marked as non-new once they've been downloaded
        if (!entry.enclosure) {
            entry.read = true;
            entry.new = false;
        }
        if (isQueue || isDownloads) {
            lastEntry = entry.id;
        }

        if (pageStack.depth > (currentPage === "FeedListPage" ? 2 : 1)) {
            pageStack.pop();
        }

        pageStack.push("qrc:/qt/qml/org/kde/kasts/qml/EntryPage.qml", {
            entry: entry,
        });
    }

    TapHandler {
        id: shiftHandler

        acceptedModifiers: Qt.ShiftModifier

        onTapped: (eventPoint) => {
            const modelIndex = listItem.listViewObject.model.index(index, 0);

            // Have to take a detour through c++ since selecting large sets
            // in QML is extremely slow
            listViewObject.selectionModel.select(
                listViewObject.model.createSelection(modelIndex.row, listViewObject.selectionModel.currentIndex.row),
                ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows
            );
        }
    }

    TapHandler {
        id: controlHandler

        acceptedModifiers: Qt.ControlModifier

        onTapped: (eventPoint) => {
            const modelIndex = listItem.listViewObject.model.index(index, 0);

            listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }
    }

    TapHandler {
        id: tapHandler

        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onTapped: (eventPoint, button) => {

            // Keep track of (currently) selected items
            const modelIndex = listItem.listViewObject.model.index(index, 0);

            if (listViewObject.selectionModel.isSelected(modelIndex) && button == Qt.RightButton) {
                listViewObject.contextMenu.popup(null, eventPoint.position.x + 1, eventPoint.position.y + 1);
            } else if (button == Qt.RightButton) {
                // This item is right-clicked, but isn't selected
                listViewObject.selectionForContextMenu = [modelIndex];
                listViewObject.contextMenu.popup(null, eventPoint.position.x + 1, eventPoint.position.y + 1);
            } else if (button == Qt.LeftButton || button == Qt.NoButton) {
                listViewObject.currentIndex = index;
                listViewObject.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                delegateTapped();
            }
        }

        onLongPressed: {
            const modelIndex = listItem.listViewObject.model.index(index, 0);
            listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }
    }

    contentItem: RowLayout {
        Connections {
            target: listViewObject.selectionModel
            function onSelectionChanged() {
                updateIsSelected();
            }
        }

        Connections {
            target: listViewObject.model
            function onLayoutChanged() {
                updateIsSelected();
            }
        }

        Loader {
            property var loaderListView: listViewObject
            property var loaderListItem: listItem
            sourceComponent: dragHandleComponent
            active: isQueue
        }

        Component {
            id: dragHandleComponent
            Kirigami.ListItemDragHandle {
                listItem: loaderListItem
                listView: loaderListView
                onMoveRequested: (oldIndex, newIndex) => {
                    DataManager.moveQueueItem(oldIndex, newIndex);
                    // reset current selection when moving items
                    var modelIndex = listItem.listView.model.index(newIndex, 0);
                    listViewObject.currentIndex = newIndex;
                    listViewObject.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                }
            }
        }

        ImageWithFallback {
            id: img
            imageSource: entry ? entry.cachedImage : "no-image"
            property int size: Kirigami.Units.gridUnit * 3
            Layout.preferredHeight: size
            Layout.preferredWidth: size
            Layout.rightMargin: Kirigami.Units.smallSpacing
            fractionalRadius: 1.0 / 8.0
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            RowLayout{
                Kirigami.Icon {
                    Layout.maximumHeight: playedLabel.implicitHeight
                    Layout.maximumWidth:  playedLabel.implicitHeight
                    source: "checkbox"
                    visible: entry ? entry.read : false
                }
                Controls.Label {
                    id: playedLabel
                    text: entry ? ((entry.enclosure ? i18n("Played") : i18n("Read")) +  "  ·") : ""
                    font: Kirigami.Theme.smallFont
                    visible: entry ? entry.read : false
                    opacity: 0.7
                }
                Controls.Label {
                    text: entry ? (entry.new ? i18n("New") + "  ·" : "") : ""
                    font.capitalization: Font.AllUppercase
                    color: Kirigami.Theme.highlightColor
                    visible: entry ? entry.new : false
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth:  0.8 * supertitle.implicitHeight
                    source: "starred-symbolic"
                    visible: entry ? (entry.favorite) : false
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth:  0.8 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: entry ? (!isQueue && entry.queueStatus) : false
                    opacity: 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: entry ? (((!isQueue && entry.queueStatus) || entry.favorite ? "·  " : "") + entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? "  ·  " + entry.enclosure.formattedSize : "") : "" )) : ""
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                }
            }
            Controls.Label {
                text: entry ? entry.title : ""
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
            }
            Loader {
                sourceComponent: entry ? (entry.enclosure && (entry.enclosure.status === Enclosure.Downloading || (isDownloads && entry.enclosure.status === Enclosure.PartiallyDownloaded)) ? downloadProgress : ( entry.enclosure && entry.enclosure.playPosition > 0 ? playProgress : subtitle)) : undefined
                Layout.fillWidth: true
            }
            Component {
                id: subtitle
                Controls.Label {
                    text: entry ? (entry.enclosure ? entry.enclosure.formattedDuration : "") : ""
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                    visible: !downloadProgress.visible
                }
            }
            Component {
                id: downloadProgress
                RowLayout {
                    Controls.Label {
                        text: entry ? entry.enclosure.formattedDownloadSize : ""
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: 1
                        value: entry ? entry.enclosure.downloadProgress : 0
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: entry ? entry.enclosure.formattedSize : ""
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
            Component {
                id: playProgress
                RowLayout {
                    Controls.Label {
                        text: entry ? entry.enclosure.formattedPlayPosition : ""
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: entry ? entry.enclosure.duration : 1
                        value: entry ? entry.enclosure.playPosition / 1000 : 0
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: entry ? ((SettingsManager.toggleRemainingTime)
                                ? "-" + entry.enclosure.formattedLeftDuration
                                : entry.enclosure.formattedDuration) : ""
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
        }

        IconOnlyButton {
            text: i18n("Remove from Queue")
            icon.name: "list-remove"
            onClicked: {
                entry.queueStatus = false;
            }
            visible: showRemoveFromQueueButton
        }

        IconOnlyButton {
            text: i18n("Download")
            icon.name: "download"
            onClicked: {
                downloadOverlay.entry = entry;
                downloadOverlay.run();
            }
            visible: showDownloadButton
        }

        IconOnlyButton {
            text: i18n("Cancel Download")
            icon.name: "edit-delete-remove"
            onClicked: entry.enclosure.cancelDownload()
            visible: showCancelDownloadButton
        }

        IconOnlyButton {
            text: i18n("Delete Download")
            icon.name: "delete"
            onClicked: entry.enclosure.deleteFile()
            visible: showDeleteDownloadButton
        }

        IconOnlyButton {
            text: i18n("Add to Queue")
            icon.name: "media-playlist-append"
            visible: showAddToQueueButton
            onClicked: entry.queueStatus = true
        }

        IconOnlyButton {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: showPlayButton
            onClicked: {
                AudioManager.entry = entry;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
            icon.name: "media-playback-cloud"
            visible: showStreamingPlayButton
            onClicked: {
                if (!entry.queueStatus) {
                    entry.queueStatus = true;
                }
                AudioManager.entry = entry;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: showPauseButton
            onClicked: AudioManager.pause()
        }
    }
}

