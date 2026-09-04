/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as AddonDelegates
import org.kde.ki18n
import org.kde.coreaddons

import org.kde.kmediasession
import org.kde.kasts

AddonDelegates.RoundedItemDelegate {
    id: root

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    // These are the properties exposed by the model that are used in this delegate
    // NOTE: don't forget to also add new properties to the delegate in QueuePage
    required property int entryuid
    required property int index
    required property string title
    required property int downloaded
    required property bool isNew
    required property bool read
    required property bool favorite
    required property bool removed
    required property date updated
    required property string image
    required property string feedImage
    required property string feedName
    required property bool queueStatus
    required property bool hasEnclosure
    required property bool enclosureUrl
    required property int playPosition
    required property int duration
    required property int size
    required property int downloadSize

    readonly property Main mainWindow: root.Controls.ApplicationWindow.window as Main

    property bool isQueue: false
    property bool downloadFilterActive: ListView.view && ListView.view.model.filterType === AbstractEpisodeProxyModel.DownloadedFilter
    property bool showFeedImage: !SettingsManager.showEpisodeImage
    property bool showFeedTitle: SettingsManager.showPodcastTitle
    property GenericEntryListView listViewObject: undefined
    property bool selected: false

    property bool showRemoveFromQueueButton: !hasEnclosure && queueStatus
    property bool showDownloadButton: hasEnclosure && (!downloadFilterActive || downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded) && (downloaded === DataTypes.EnclosureStatus.Downloadable || downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded) && (!NetworkConnectionManager.streamingAllowed || !SettingsManager.prioritizeStreaming || downloadFilterActive) && !(AudioManager.entryuid === entryuid && AudioManager.playbackState === KMediaSession.PlayingState)
    property bool showCancelDownloadButton: hasEnclosure && (downloaded === DataTypes.EnclosureStatus.Downloading || downloaded == DataTypes.EnclosureStatus.Queued)
    property bool showDeleteDownloadButton: downloadFilterActive && hasEnclosure && (downloaded === DataTypes.EnclosureStatus.Downloaded || downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded)
    property bool showAddToQueueButton: !downloadFilterActive && !queueStatus && hasEnclosure && downloaded === DataTypes.EnclosureStatus.Downloaded
    property bool showPlayButton: !downloadFilterActive && queueStatus && hasEnclosure && (downloaded === DataTypes.EnclosureStatus.Downloaded) && (AudioManager.entryuid !== entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
    property bool showStreamingPlayButton: !downloadFilterActive && hasEnclosure && (downloaded !== DataTypes.EnclosureStatus.Downloaded && downloaded !== DataTypes.EnclosureStatus.Downloading && NetworkConnectionManager.streamingAllowed && SettingsManager.prioritizeStreaming) && (AudioManager.entryuid !== entryuid || AudioManager.playbackState !== KMediaSession.PlayingState)
    property bool showPauseButton: !downloadFilterActive && queueStatus && hasEnclosure && (AudioManager.entryuid === entryuid && AudioManager.playbackState === KMediaSession.PlayingState)

    component IconOnlyButton: Controls.ToolButton {
        display: Controls.ToolButton.IconOnly

        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    function leftDuration(duration: int, playPosition: int): int {
        var rate = 1.0;

        if (SettingsManager.adjustTimeLeft) {
            rate = AudioManager.playbackRate;
            rate = (rate > 0.0) ? rate : 1.0;
        }
        var diff = duration * 1000 - playPosition;
        return (diff / rate);
    }

    highlighted: selected

    Accessible.role: Accessible.Button
    Accessible.name: title
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
        if (!hasEnclosure) {
            DataManager.bulkMarkRead(true, [entryuid]);
            DataManager.bulkMarkNew(false, [entryuid]);
        }
        if (isQueue || downloadFilterActive) {
            const pageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
            if (pageStack.get(0)?.lastEntry > -1) {
                pageStack.get(0).lastEntry = entryuid;
            }
        }

        if (mainWindow.pageStack.depth > (mainWindow.currentPage === "FeedListPage" ? 2 : 1)) {
            mainWindow.pageStack.pop();
        }

        mainWindow.pageStack.push(Qt.createComponent("org.kde.kasts", "EntryPage"), {
            entryuid: entryuid
        });
    }

    TapHandler {
        id: shiftHandler

        acceptedModifiers: Qt.ShiftModifier

        onTapped: eventPoint => {
            const modelIndex = root.listViewObject.model.index(root.index, 0);

            // Have to take a detour through c++ since selecting large sets
            // in QML is extremely slow
            root.listViewObject.selectionModel.select(root.listViewObject.model.createSelection(modelIndex.row, root.listViewObject.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
    }

    TapHandler {
        id: controlHandler

        acceptedModifiers: Qt.ControlModifier

        onTapped: eventPoint => {
            const modelIndex = root.listViewObject.model.index(root.index, 0);

            root.listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }
    }

    TapHandler {
        id: tapHandler

        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        exclusiveSignals: Kirigami.Settings.isMobile ? (TapHandler.SingleTap | TapHandler.DoubleTap) : TapHandler.NotExclusive

        onSingleTapped: (eventPoint, button) => {

            // Keep track of (currently) selected items
            const modelIndex = root.listViewObject.model.index(root.index, 0);

            if (root.listViewObject.selectionModel.isSelected(modelIndex) && button == Qt.RightButton) {
                root.listViewObject.contextMenu.popup(null, eventPoint.position.x + 1, eventPoint.position.y + 1);
            } else if (button == Qt.RightButton) {
                // This item is right-clicked, but isn't selected
                root.listViewObject.selectionForContextMenu = [modelIndex];
                root.listViewObject.contextMenu.popup(null, eventPoint.position.x + 1, eventPoint.position.y + 1);
            } else if (button == Qt.LeftButton || button == Qt.NoButton) {
                root.listViewObject.currentIndex = root.index;
                root.listViewObject.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                root.delegateTapped();
            }
        }

        onLongPressed: {
            const modelIndex = root.listViewObject.model.index(root.index, 0);
            root.listViewObject.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
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
            sourceComponent: dragHandleComponent
            active: root.isQueue
        }

        Component {
            id: dragHandleComponent
            Kirigami.ListItemDragHandle {
                listItem: root
                listView: root.listViewObject
                onMoveRequested: (oldIndex, newIndex) => {
                    QueueModel.moveQueueItem(oldIndex, newIndex);
                    // reset current selection when moving items
                    var modelIndex = root.listView.model.index(newIndex, 0);
                    root.listViewObject.currentIndex = newIndex;
                    root.listViewObject.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                }
            }
        }

        ImageWithFallback {
            id: img
            imageSource: root.showFeedImage ? root.feedImage : root.image
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
                    visible: root.read
                }
                Controls.Label {
                    id: playedLabel
                    text: (root.hasEnclosure ? KI18n.i18n("Played") : KI18n.i18n("Read")) + "  ·"
                    font: Kirigami.Theme.smallFont
                    visible: root.read
                    opacity: 0.7
                }
                Controls.Label {
                    text: root.isNew ? KI18n.i18n("New") + "  ·" : ""
                    font.capitalization: Font.AllUppercase
                    color: Kirigami.Theme.highlightColor
                    visible: root.isNew
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth: 0.8 * supertitle.implicitHeight
                    source: "starred-symbolic"
                    visible: root.favorite
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth: 0.8 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: !root.isQueue && root.queueStatus
                    opacity: 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: (((!root.isQueue && root.queueStatus) || root.favorite) ? "·  " : "") + root.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (root.hasEnclosure ? (root.size !== 0 ? "  ·  " + Format.formatByteSize(root.size) : "") : "") + (root.showFeedTitle ? "  ·  " + root.feedName : "") + (root.removed ? "  ·" : "")
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth: 0.8 * supertitle.implicitHeight
                    source: "emblem-unmounted"
                    visible: root.removed
                    opacity: 0.7

                    HoverHandler {
                        id: removedHover
                    }

                    Controls.ToolTip.text: KI18n.i18nc("@info:tooltip Tooltip for an icon that can be displayed on an episode list item", "This episode has been removed from the podcast RSS feed by the podcast author(s). Downloading and/or streaming this episode might be broken.")
                    Controls.ToolTip.visible: removedHover.hovered
                    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                Controls.Label {
                    text: ""
                    Layout.fillWidth: true
                    font: Kirigami.Theme.smallFont
                }
            }
            Controls.Label {
                text: root.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
            }
            Loader {
                sourceComponent: root.hasEnclosure && (root.downloaded === DataTypes.EnclosureStatus.Downloading || root.downloaded === DataTypes.EnclosureStatus.Queued || (root.downloadFilterActive && root.downloaded === DataTypes.EnclosureStatus.PartiallyDownloaded)) ? downloadProgress : (root.hasEnclosure && root.playPosition > 0 ? playProgress : subtitle)
                Layout.fillWidth: true
            }
            Component {
                id: subtitle
                Controls.Label {
                    text: root.hasEnclosure ? Format.formatDuration(root.duration * 1000) : ""
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.7
                }
            }
            Component {
                id: downloadProgress
                RowLayout {
                    Controls.Label {
                        visible: root.downloaded != DataTypes.EnclosureStatus.Queued
                        text: Format.formatByteSize(root.downloadSize)
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        indeterminate: root.downloaded == DataTypes.EnclosureStatus.Queued
                        from: 0
                        to: 1
                        value: root.size > 0 ? root.downloadSize / root.size : 0.0
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: Format.formatByteSize(root.size)
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
                        text: Format.formatDuration(root.playPosition)
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: root.duration
                        value: root.playPosition / 1000
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: SettingsManager.toggleRemainingTime ? "-" + Format.formatDuration(root.leftDuration(root.duration, root.playPosition)) : Format.formatDuration(root.duration * 1000)
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
        }

        IconOnlyButton {
            text: KI18n.i18n("Remove from Queue")
            icon.name: "list-remove"
            onClicked: {
                DataManager.bulkQueueStatus(false, [root.entryuid]);
            }
            visible: root.showRemoveFromQueueButton
        }

        IconOnlyButton {
            text: KI18n.i18n("Download")
            icon.name: "download"
            onClicked: {
                root.mainWindow.downloadOverlay.entryuid = root.entryuid;
                root.mainWindow.downloadOverlay.run();
            }
            visible: root.showDownloadButton
        }

        IconOnlyButton {
            text: KI18n.i18n("Cancel Download")
            icon.name: "edit-delete-remove"
            onClicked: Fetcher.cancelEnclosureDownload(root.entryuid)
            visible: root.showCancelDownloadButton
        }

        IconOnlyButton {
            text: KI18n.i18n("Delete Download")
            icon.name: "delete"
            onClicked: DataManager.bulkDeleteEnclosures([root.entryuid])
            visible: root.showDeleteDownloadButton
        }

        IconOnlyButton {
            text: KI18n.i18n("Add to Queue")
            icon.name: "media-playlist-append"
            visible: root.showAddToQueueButton
            onClicked: DataManager.bulkQueueStatus(true, [root.entryuid])
        }

        IconOnlyButton {
            text: KI18n.i18n("Play")
            icon.name: "media-playback-start"
            visible: root.showPlayButton
            onClicked: {
                AudioManager.entryuid = root.entryuid;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: KI18n.i18nc("@action:inmenu Action to start playback by streaming the episode rather than downloading it first", "Stream")
            icon.name: "media-playback-cloud"
            visible: root.showStreamingPlayButton
            onClicked: {
                if (!root.queueStatus) {
                    DataManager.bulkQueueStatus(true, [root.entryuid]);
                }
                AudioManager.entryuid = root.entryuid;
                AudioManager.play();
            }
        }

        IconOnlyButton {
            text: KI18n.i18n("Pause")
            icon.name: "media-playback-pause"
            visible: root.showPauseButton
            onClicked: AudioManager.pause()
        }
    }
}
