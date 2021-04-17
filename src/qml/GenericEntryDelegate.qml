/**
   SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// Includes relevant modules used by the QML
import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.15
import org.kde.alligator 1.0


Kirigami.SwipeListItem {
    id: listItem
    alwaysVisibleActions: true

    property bool isQueue: false
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
        Image {
            asynchronous: true
            source: entry.image === "" ? "logo.png" : "file://"+Fetcher.image(entry.image)
            fillMode: Image.PreserveAspectFit
            property int size: Kirigami.Units.gridUnit * 3
            sourceSize.width: size
            sourceSize.height: size
            Layout.maximumHeight: size
            Layout.maximumWidth: size
            Layout.rightMargin:Kirigami.Units.smallSpacing
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
                    opacity: (entry.read) ? 0.4 : 0.7
                }
                Kirigami.Icon {
                    Layout.maximumHeight: 0.8 * supertitle.implicitHeight
                    Layout.maximumWidth:  0.8 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: !isQueue && entry.queueStatus
                    opacity: (entry.read) ? 0.4 : 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: (!isQueue && entry.queueStatus ? "·  " : "") + entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? " ·  " + Math.floor(entry.enclosure.size/(1024*1024)) + "MB" : "") : "" )
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: (entry.read) ? 0.4 : 0.7
                }
            }
            Controls.Label {
                text: entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
                opacity: (entry.read) ? 0.6 : 1
            }
            Loader {
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : ( entry.enclosure && entry.enclosure.playPosition > 0 ?playProgress : subtitle)
                Layout.fillWidth: true
            }
            Component {
                id: subtitle
                Controls.Label {
                    text: (Math.floor(entry.enclosure.duration/3600) < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/3600) + ":" + (Math.floor(entry.enclosure.duration/60) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/60) % 60 + ":" + (Math.floor(entry.enclosure.duration) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration) % 60
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
                        text: (Math.floor(entry.enclosure.playPosition/3600000) < 10 ? "0" : "") + Math.floor(entry.enclosure.playPosition/3600000) + ":" + (Math.floor(entry.enclosure.playPosition/60000) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.playPosition/60000) % 60 + ":" + (Math.floor(entry.enclosure.playPosition/1000) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.playPosition/1000) % 60
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                    Controls.ProgressBar {
                        from: 0
                        to: entry.enclosure.duration
                        value: entry.enclosure.playPosition/1000
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: (Math.floor(entry.enclosure.duration/3600) < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/3600) + ":" + (Math.floor(entry.enclosure.duration/60) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/60) % 60 + ":" + (Math.floor(entry.enclosure.duration) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration) % 60
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
            text: i18n("Download")
            icon.name: "download"
            onTriggered: {
                entry.queueStatus = true;
                entry.enclosure.download();
            }
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloadable
        },
        Kirigami.Action {
            text: i18n("Cancel download")
            icon.name: "edit-delete-remove"
            onTriggered: entry.enclosure.cancelDownload()
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloading
        },
        Kirigami.Action {
            text: i18n("Add to queue")
            icon.name: "media-playlist-append"
            visible: !entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
            onTriggered: entry.queueStatus = true
        },
        Kirigami.Action {
            text: i18n("Play")
            icon.name: "media-playback-start"
            visible: entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && (audio.entry !== entry || audio.playbackState !== Audio.PlayingState)
            onTriggered: {
                audio.entry = entry
                audio.play()
            }
        },
        Kirigami.Action {
            text: i18n("Pause")
            icon.name: "media-playback-pause"
            visible: entry.queueStatus && entry.enclosure && entry.enclosure.status === Enclosure.Downloaded && audio.entry === entry && audio.playbackState === Audio.PlayingState
            onTriggered: audio.pause()
        }
    ]
}

