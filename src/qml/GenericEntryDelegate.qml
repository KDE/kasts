/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15
import QtGraphicalEffects 1.15
import QtQml.Models 2.15

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kasts.solidextras 1.0

import org.kde.kasts 1.0

Kirigami.SwipeListItem {
    id: listItem
    alwaysVisibleActions: true

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property bool isQueue: false
    property bool isDownloads: false
    property QtObject listView: undefined
    property bool selected: false
    property int row: model ? model.row : -1

    property bool streamingAllowed: (NetworkStatus.connectivity !== NetworkStatus.No && SettingsManager.prioritizeStreaming && (SettingsManager.allowMeteredStreaming || NetworkStatus.metered !== NetworkStatus.Yes))

    property bool showRemoveFromQueueButton: !entry.enclosure && entry.queueStatus
    property bool showDownloadButton: (!isDownloads || entry.enclosure.status === Enclosure.PartiallyDownloaded) && entry.enclosure && (entry.enclosure.status === Enclosure.Downloadable || entry.enclosure.status === Enclosure.PartiallyDownloaded) && (!streamingAllowed || isDownloads) && !(AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState)
    property bool showCancelDownloadButton: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
    property bool showDeleteDownloadButton: isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
    property bool showAddToQueueButton: !isDownloads && !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
    property bool showPlayButton: !isDownloads && entry.queueStatus && entry.enclosure && (entry.enclosure.status === Enclosure.Downloaded) && (AudioManager.entry !== entry || AudioManager.playbackState !== Audio.PlayingState)
    property bool showStreamingPlayButton: !isDownloads && entry.enclosure && (entry.enclosure.status !== Enclosure.Downloaded && entry.enclosure.status !== Enclosure.Downloading && streamingAllowed) && (AudioManager.entry !== entry || AudioManager.playbackState !== Audio.PlayingState)
    property bool showPauseButton: !isDownloads && entry.queueStatus && entry.enclosure && (AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState)


    highlighted: selected
    activeBackgroundColor: Qt.lighter(Kirigami.Theme.highlightColor, 1.3)

    Accessible.role: Accessible.Button
    Accessible.name: entry.title
    Accessible.onPressAction: {
         listItem.clicked();
    }

    Keys.onReturnPressed: clicked()

    // We need to update the "selected" status:
    // - if the selected indexes changes
    // - if our delegate moves
    // - if the model moves and the delegate stays in the same place
    function updateIsSelected() {
        selected = listView.selectionModel.rowIntersectsSelection(row);
    }

    onRowChanged: {
        updateIsSelected();
    }

    Component.onCompleted: {
        updateIsSelected();
    }

    contentItem: MouseArea {
        id: mouseArea
        implicitHeight: rowLayout.implicitHeight
        implicitWidth: rowLayout.implicitWidth

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            // Keep track of (currently) selected items
            var modelIndex = listItem.listView.model.index(index, 0);

            if (listView.selectionModel.isSelected(modelIndex) && mouse.button == Qt.RightButton) {
                listView.contextMenu.popup(null, mouse.x+1, mouse.y+1);
            } else if (mouse.modifiers & Qt.ShiftModifier) {
                // Have to take a detour through c++ since selecting large sets
                // in QML is extremely slow
                listView.selectionModel.select(listView.model.createSelection(modelIndex.row, listView.selectionModel.currentIndex.row), ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            } else if (mouse.modifiers & Qt.ControlModifier) {
                listView.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
            } else if (mouse.button == Qt.LeftButton) {
                listView.currentIndex = index;
                listView.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                listItem.clicked();
            } else if (mouse.button == Qt.RightButton) {
                // This item is right-clicked, but isn't selected
                listView.selectionForContextMenu = [modelIndex];
                listView.contextMenu.popup(null, mouse.x+1, mouse.y+1);
            }
        }

        onPressAndHold: {
            var modelIndex = listItem.listView.model.index(index, 0);
            listView.selectionModel.select(modelIndex, ItemSelectionModel.Toggle | ItemSelectionModel.Rows);
        }

        Connections {
            target: listView.selectionModel
            function onSelectionChanged() {
                updateIsSelected();
            }
        }

        Connections {
            target: listView.model
            function onLayoutChanged() {
                updateIsSelected();
            }
        }

        RowLayout {
            id: rowLayout
            anchors.fill: parent

            Loader {
                property var loaderListView: listView
                property var loaderListItem: listItem
                sourceComponent: dragHandleComponent
                active: isQueue
            }

            Component {
                id: dragHandleComponent
                Kirigami.ListItemDragHandle {
                    listItem: loaderListItem
                    listView: loaderListView
                    onMoveRequested: {
                        DataManager.moveQueueItem(oldIndex, newIndex);
                        // reset current selection when moving items
                        var modelIndex = listItem.listView.model.index(newIndex, 0);
                        listView.currentIndex = newIndex;
                        listView.selectionModel.setCurrentIndex(modelIndex, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                    }
                }
            }

            ImageWithFallback {
                id: img
                imageSource: entry.cachedImage
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
                        visible: entry.read
                    }
                    Controls.Label {
                        id: playedLabel
                        text: (entry.enclosure ? i18n("Played") : i18n("Read")) +  "  ·"
                        font: Kirigami.Theme.smallFont
                        visible: entry.read
                        opacity: 0.7
                    }
                    Controls.Label {
                        text: entry.new ? i18n("New") + "  ·" : ""
                        font.capitalization: Font.AllUppercase
                        color: Kirigami.Theme.highlightColor
                        visible: entry.new
                        opacity: 0.7
                    }
                    Kirigami.Icon {
                        Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                        Layout.maximumWidth:  0.8 * supertitle.implicitHeight
                        source: "source-playlist"
                        visible: !isQueue && entry.queueStatus
                        opacity: 0.7
                    }
                    Controls.Label {
                        id: supertitle
                        text: (!isQueue && entry.queueStatus ? "·  " : "") + entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? "  ·  " + entry.enclosure.formattedSize : "") : "" )
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
                Controls.Label {
                    text: entry.title
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.weight: Font.Normal
                }
                Loader {
                    sourceComponent: entry.enclosure && (entry.enclosure.status === Enclosure.Downloading || (isDownloads && entry.enclosure.status === Enclosure.PartiallyDownloaded)) ? downloadProgress : ( entry.enclosure && entry.enclosure.playPosition > 0 ? playProgress : subtitle)
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
                            text: (SettingsManager.toggleRemainingTime)
                                    ? "-" + entry.enclosure.formattedLeftDuration
                                    : entry.enclosure.formattedDuration
                            elide: Text.ElideRight
                            font: Kirigami.Theme.smallFont
                            opacity: 0.7
                        }
                    }
                }
            }
        }
    }

    onClicked: {
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
        if (pageStack.depth > (currentPage === "FeedListPage" ? 2 : 1))
            pageStack.pop();
        pageStack.push("qrc:/EntryPage.qml", {"entry": entry});
    }

    actions: [
        Kirigami.Action {
            text: i18n("Remove from Queue")
            icon.name: "list-remove"
            onTriggered: {
                entry.queueStatus = false;
            }
            visible: showRemoveFromQueueButton
        },
        Kirigami.Action {
            text: i18n("Download")
            icon.name: "download"
            onTriggered: {
                downloadOverlay.entry = entry;
                downloadOverlay.run();
            }
            visible: showDownloadButton
        },
        Kirigami.Action {
            text: i18n("Cancel Download")
            icon.name: "edit-delete-remove"
            onTriggered: entry.enclosure.cancelDownload()
            visible: showCancelDownloadButton
        },
        Kirigami.Action {
            text: i18n("Delete Download")
            icon.name: "delete"
            onTriggered: entry.enclosure.deleteFile()
            visible: showDeleteDownloadButton
        },
        Kirigami.Action {
            text: i18n("Add to Queue")
            icon.name: "media-playlist-append"
            visible: showAddToQueueButton
            onTriggered: entry.queueStatus = true
        },
        Kirigami.Action {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: showPlayButton
            onTriggered: {
                AudioManager.entry = entry;
                AudioManager.play();
            }
        },
        Kirigami.Action {
            text: i18nc("Action to start playback by streaming the episode rather than downloading it first", "Stream")
            icon.name: ":/media-playback-start-cloud"
            visible: showStreamingPlayButton
            onTriggered: {
                if (!entry.queueStatus) {
                    entry.queueStatus = true;
                }
                AudioManager.entry = entry;
                AudioManager.play();
            }
        },
        Kirigami.Action {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: showPauseButton
            onTriggered: AudioManager.pause()
        }
    ]
}

