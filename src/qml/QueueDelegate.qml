/*
   SPDX-FileCopyrightText: 2021 (c) Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// Includes relevant modules used by the QML
import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.15
import org.kde.alligator 1.0


Kirigami.SwipeListItem {
    id: listItem

    contentItem: RowLayout {
        Kirigami.ListItemDragHandle {
            listItem: listItem
            listView: queueList
            onMoveRequested: DataManager.moveQueueItem(oldIndex, newIndex)
        }
        Image {
            asynchronous: true
            source: entry.image === "" ? "rss" : "file://"+Fetcher.image(entry.image)
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
            Controls.Label {
                text: entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? " · " + Math.floor(entry.enclosure.size/(1024*1024)) + "MB" : "") : "" )
                Layout.fillWidth: true
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                opacity: 0.7
            }
            Controls.Label {
                text: entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Normal
                opacity: 1
            }
            Loader {
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : subtitle
                Layout.fillWidth: true
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
            }
        }
    }

    onClicked: {
        entry.read = true
        pageStack.push("qrc:/EntryPage.qml", {"entry": entry})
    }

    actions: [
        Kirigami.Action {
            iconName: "media-playback-start"
            text: "Play"
            onTriggered: {
                audio.entry = entry
                audio.play()
            }
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
        },
        Kirigami.Action {
            text: i18n("Download")
            icon.name: "download"
            onTriggered: entry.enclosure.download()
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloadable
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
            visible: entry.enclosure && entry.enclosure.status === Enclosure.Downloaded
        },
        Kirigami.Action {
            text: i18n("Remove from Queue")
            icon.name: "delete-table-row"
            onTriggered: { DataManager.removeQueueItem(entry) }
        }
    ]
}

