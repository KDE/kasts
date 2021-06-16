/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Kirigami.SwipeListItem {
    id: listItem
    alwaysVisibleActions: true

    property bool isQueue: false
    property bool isDownloads: false
    property var listView: ""

    Accessible.role: Accessible.Button
    Accessible.name: entry.title
    Accessible.onPressAction: {
         listItem.click()
    }

    contentItem: RowLayout {

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
                onMoveRequested: DataManager.moveQueueItem(oldIndex, newIndex)
            }
        }

        ImageWithFallback {
            id: img
            imageSource: entry.cachedImage
            property int size: Kirigami.Units.gridUnit * 3
            Layout.preferredHeight: size
            Layout.preferredWidth: size
            Layout.rightMargin:Kirigami.Units.smallSpacing
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
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : ( entry.enclosure && entry.enclosure.playPosition > 0 ? playProgress : subtitle)
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
                Controls.ProgressBar {
                    from: 0
                    to: 100
                    value: entry.enclosure.downloadProgress
                    visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
                    Layout.fillWidth: true
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
                        text: entry.enclosure.formattedDuration
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
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
        if (isQueue) {
            lastEntry = entry.id;
        }
        pageStack.push("qrc:/EntryPage.qml", {"entry": entry})
    }

    actions: [
        Kirigami.Action {
            text: i18n("Remove from Queue")
            icon.name: "list-remove"
            onTriggered: {
                entry.queueStatus = false;
            }
            visible: !entry.enclosure && entry.queueStatus
        },
        Kirigami.Action {
            text: i18n("Download")
            icon.name: "download"
            onTriggered: {
                entry.queueStatus = true;
                entry.enclosure.download();
            }
            visible: !isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloadable
        },
        Kirigami.Action {
            text: i18n("Cancel Download")
            icon.name: "edit-delete-remove"
            onTriggered: entry.enclosure.cancelDownload()
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
        },
        Kirigami.Action {
            text: i18n("Delete Download")
            icon.name: "delete"
            onTriggered: entry.enclosure.deleteFile()
            visible: isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
        },
        Kirigami.Action {
            text: i18n("Add to Queue")
            icon.name: "media-playlist-append"
            visible: !isDownloads && !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
            onTriggered: entry.queueStatus = true
        },
        Kirigami.Action {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: !isDownloads && entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && (AudioManager.entry !== entry || AudioManager.playbackState !== Audio.PlayingState)
            onTriggered: {
                AudioManager.entry = entry
                AudioManager.play()
            }
        },
        Kirigami.Action {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: !isDownloads && entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && AudioManager.entry === entry && AudioManager.playbackState === Audio.PlayingState
            onTriggered: AudioManager.pause()
        }
    ]
}

