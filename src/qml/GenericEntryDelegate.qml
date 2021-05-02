/**
   SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.14 as Kirigami
import QtMultimedia 5.15
import QtGraphicalEffects 1.15
import org.kde.alligator 1.0

Kirigami.SwipeListItem {
    id: listItem
    alwaysVisibleActions: true

    property bool isQueue: false
    property bool isDownloads: false
    property var listView: ""

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
            imageOpacity: (entry.read) ? 0.5 : 1
            fractionalRadius: 1.0 / 8.0
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            RowLayout{
                Controls.Label {
                    text: entry.new ? i18n("new") + "  ·" : ""
                    font.capitalization: Font.AllUppercase
                    color: Kirigami.Theme.highlightColor
                    visible: entry.new
                    opacity: entry.read ? 0.4 : 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth:  0.8 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: !isQueue && entry.queueStatus
                    opacity: entry.read ? 0.4 : 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: (!isQueue && entry.queueStatus ? "·  " : "") + entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? " ·  " + Math.floor(entry.enclosure.size/(1024*1024)) + "MB" : "") : "" )
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: entry.read ? 0.4 : 0.7
                }
            }
            Controls.Label {
                text: entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
                opacity: entry.read ? 0.6 : 1
            }
            Loader {
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : ( entry.enclosure && entry.enclosure.playPosition > 0 ? playProgress : subtitle)
                Layout.fillWidth: true
            }
            Component {
                id: subtitle
                Controls.Label {
                    text: audio.timeString(entry.enclosure.duration * 1000)
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
                        text: audio.timeString(entry.enclosure.playPosition)
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
                        text: audio.timeString(entry.enclosure.duration * 1000)
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
            text: i18n("Cancel download")
            icon.name: "edit-delete-remove"
            onTriggered: entry.enclosure.cancelDownload()
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
        },
        Kirigami.Action {
            text: i18n("Delete download")
            icon.name: "delete"
            onTriggered: entry.enclosure.deleteFile()
            visible: isDownloads && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
        },
        Kirigami.Action {
            text: i18n("Add to queue")
            icon.name: "media-playlist-append"
            visible: !isDownloads && !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
            onTriggered: entry.queueStatus = true
        },
        Kirigami.Action {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: !isDownloads && entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && (audio.entry !== entry || audio.playbackState !== Audio.PlayingState)
            onTriggered: {
                audio.entry = entry
                audio.play()
            }
        },
        Kirigami.Action {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: !isDownloads && entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && audio.entry === entry && audio.playbackState === Audio.PlayingState
            onTriggered: audio.pause()
        }
    ]
}

