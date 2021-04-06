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
        Kirigami.Icon {
            source: entry.image === "" ? "rss" : Fetcher.image(entry.image)
            property int size: Kirigami.Units.iconSizes.medium
            Layout.minimumHeight: size
            Layout.minimumWidth: size
        }
        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Controls.Label {
                text: entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: entry.read ? Font.Normal : Font.Bold
                opacity: 1
            }
            Loader {
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : subtitle
                Layout.fillWidth: true
                Component {
                    id: subtitle
                    Controls.Label {
                        text: entry.updated.toLocaleString(Qt.locale(), Locale.ShortFormat) + (entry.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + entry.authors[0].name)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: entry.read ? 0.7 : 0.9
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

