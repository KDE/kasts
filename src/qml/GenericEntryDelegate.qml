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

AddonDelegates.RoundedItemDelegate {
    id: root

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    required property Entry entry
    required property int index

    property bool isQueue: false
    property bool isDownloads: false
    property bool showFeedImage: !SettingsManager.showEpisodeImage
    property bool showFeedTitle: SettingsManager.showPodcastTitle
    property QtObject listViewObject: undefined
    property bool selected: false

    property bool showRemoveFromQueueButton: !entry.enclosure && entry.queueStatus
    property bool showDownloadButton: entry.enclosure && (!isDownloads || entry.enclosure.status === Enclosure.PartiallyDownloaded) && (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) && (!NetworkConnectionManager.streamingAllowed || !SettingsManager.prioritizeStreaming || isDownloads) && !(AudioManager.entry === entry && AudioManager.playbackState === KMediaSession.PlayingState)
    property bool showCancelDownloadButton: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
    property bool showDeleteDownloadButton: isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
    property bool showAddToQueueButton: !isDownloads && !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
    property bool showPlayButton: !isDownloads && entry.queueStatus && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded) && (AudioManager.entry !== entry || AudioManager.playbackState !== KMediaSession.PlayingState)
    property bool showStreamingPlayButton: !isDownloads && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded && entry.enclosure.status !== Enclosure.Downloading && NetworkConnectionManager.streamingAllowed && SettingsManager.prioritizeStreaming) && (AudioManager.entry !== entry || AudioManager.playbackState !== KMediaSession.PlayingState)
    property bool showPauseButton: !isDownloads && entry.queueStatus && entry.enclosure && (AudioManager.entry === entry && AudioManager.playbackState === KMediaSession.PlayingState)

    component IconOnlyButton: Controls.ToolButton {
        display: Controls.ToolButton.IconOnly

        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    highlighted: selected

    Accessible.role: Accessible.Button
    Accessible.name: entry.title
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
    function updateIsSelected(): void {
        selected = listViewObject.selectionModel.rowIntersectsSelection(root.index);
    }

    onIndexChanged: {
        updateIsSelected();
    }

    Component.onCompleted: {
        updateIsSelected();
    }

    function delegateTapped(): void {
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

        pageStack.push(Qt.createComponent("org.kde.kasts", "EntryPage"), {
            entry: entry
        });
    }

    TapHandler {
        id: shiftHandler

        acceptedModifiers: Qt.ShiftModifier

        onTapped: eventPoint => {
            const modelIndex = root.listViewObject.model.index(index, 0);

            // Have to take a detour through c++ since selecting large sets
            // in QML is extremely slow
            listViewObject.selectionModel.select(listViewObject.model.createSelection(modelIndex.row, listViewObject.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
    }

    TapHandler {
        id: controlHandler

        acceptedModifiers: Qt.ControlModifier

        onTapped: eventPoint => {
            const modelIndex = root.listViewObject.model.index(index, 0);

            listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }
    }

    TapHandler {
        id: tapHandler

        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        exclusiveSignals: Kirigami.Settings.isMobile ? (TapHandler.SingleTap | TapHandler.DoubleTap) : TapHandler.NotExclusive

        onSingleTapped: (eventPoint, button) => {

            // Keep track of (currently) selected items
            const modelIndex = root.listViewObject.model.index(index, 0);

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
            const modelIndex = root.listViewObject.model.index(index, 0);
            listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }
    }

    contentItem: RowLayout {
        Connections {
            target: root.listViewObject.selectionModel
            function onSelectionChanged(): void {
                root.updateIsSelected();
            }
        }

        Connections {
            target: root.listViewObject.model
            function onLayoutChanged(): void {
                root.updateIsSelected();
            }
        }

        Loader {
            property var loaderListView: root.listViewObject
            property var loaderListItem: root
            sourceComponent: dragHandleComponent
            active: root.isQueue
        }

        Component {
            id: dragHandleComponent
            Kirigami.ListItemDragHandle {
                listItem: loaderListItem
                listView: loaderListView
                onMoveRequested: (oldIndex, newIndex) => {
                    DataManager.moveQueueItem(oldIndex, newIndex);
                    // reset current selection when moving items
                    var modelIndex = root.listView.model.index(newIndex, 0);
                    listViewObject.currentIndex = newIndex;
                    listViewObject.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                }
            }
        }

        ImageWithFallback {
            id: img
            imageSource: root.showFeedImage ? root.entry.feed.cachedImage : root.entry.cachedImage
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
            RowLayout {
                Kirigami.Icon {
                    Layout.maximumHeight: playedLabel.implicitHeight
                    Layout.maximumWidth: playedLabel.implicitHeight
                    source: "checkbox"
                    visible: root.entry.read
                }
                Controls.Label {
                    id: playedLabel
                    text: (root.entry.enclosure ? i18n("Played") : i18n("Read")) + "  ·"
                    font: Kirigami.Theme.smallFont
                    visible: root.entry.read
                    opacity: 0.7
                }
                Controls.Label {
                    text: root.entry.new ? i18n("New") + "  ·" : ""
                    font.capitalization: Font.AllUppercase
                    color: Kirigami.Theme.highlightColor
                    visible: root.entry.new
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth: 0.8 * supertitle.implicitHeight
                    source: "starred-symbolic"
                    visible: root.entry.favorite
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth: 0.8 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: !root.isQueue && root.entry.queueStatus
                    opacity: 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: ((!root.isQueue && root.entry.queueStatus) || root.entry.favorite ? "·  " : "") + root.entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (root.entry.enclosure ? (root.entry.enclosure.size !== 0 ? "  ·  " + root.entry.enclosure.formattedSize : "") : "") + ((root.entry.feed && root.showFeedTitle) ? "  ·  " + root.entry.feed.name : "")
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                }
            }
            Controls.Label {
                text: root.entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
            }
            Loader {
                sourceComponent: root.entry.enclosure && (root.entry.enclosure.status === Enclosure.Downloading || (root.isDownloads && root.entry.enclosure.status === Enclosure.PartiallyDownloaded)) ? downloadProgress : (root.entry.enclosure && root.entry.enclosure.playPosition > 0 ? playProgress : subtitle)
                Layout.fillWidth: true
            }
            Component {
                id: subtitle
                Controls.Label {
                    text: entry.enclosure ? entry.enclosure.formattedDuration : ""
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
                        text: entry.enclosure.formattedDownloadSize
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: 1
                        value: entry.enclosure.downloadProgress
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: entry.enclosure.formattedSize
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
                        text: entry.enclosure.formattedPlayPosition
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: entry.enclosure.duration
                        value: entry.enclosure.playPosition / 1000
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: SettingsManager.toggleRemainingTime ? "-" + entry.enclosure.formattedLeftDuration : entry.enclosure.formattedDuration
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
                root.entry.queueStatus = false;
            }
            visible: root.showRemoveFromQueueButton
        }

        IconOnlyButton {
            text: i18n("Download")
            icon.name: "download"
            onClicked: {
                downloadOverlay.entry = entry;
                downloadOverlay.run();
            }
            visible: root.showDownloadButton
        }

        IconOnlyButton {
            text: i18n("Cancel Download")
            icon.name: "edit-delete-remove"
            onClicked: root.entry.enclosure.cancelDownload()
            visible: root.showCancelDownloadButton
        }

        IconOnlyButton {
            text: i18n("Delete Download")
            icon.name: "delete"
            onClicked: root.entry.enclosure.deleteFile()
            visible: root.showDeleteDownloadButton
        }

        IconOnlyButton {
            text: i18n("Add to Queue")
            icon.name: "media-playlist-append"
            visible: root.showAddToQueueButton
            onClicked: root.entry.queueStatus = true
        }

        IconOnlyButton {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: root.showPlayButton
            onClicked: {
                AudioManager.entry = root.entry;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
            icon.name: "media-playback-cloud"
            visible: root.showStreamingPlayButton
            onClicked: {
                if (!root.entry.queueStatus) {
                    root.entry.queueStatus = true;
                }
                AudioManager.entry = root.entry;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: root.showPauseButton
            onClicked: AudioManager.pause()
        }
    }
}
