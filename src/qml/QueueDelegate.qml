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
            source: Fetcher.image(model.entry.image)
            height: parent.height
            width: height
        }
        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Controls.Label {
                text: model.entry.title
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: model.entry.read ? Font.Normal : Font.Bold
                opacity: 1
            }
            Loader {
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : subtitle
                Layout.fillWidth: true
                Component {
                    id: subtitle
                    Controls.Label {
                        text: model.entry.updated.toLocaleString(Qt.locale(), Locale.ShortFormat) + (model.entry.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + model.entry.authors[0].name)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: model.entry.read ? 0.7 : 0.9
                        visible: !downloadProgress.visible
                    }
                }
                Component {
                    id: downloadProgress
                    Controls.ProgressBar {
                        from: 0
                        to: 100
                        value: model.entry.enclosure.downloadProgress
                        visible: model.entry.enclosure && model.entry.enclosure.status === Enclosure.Downloading
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    onClicked: {
        model.entry.read = true
        pageStack.push("qrc:/EntryPage.qml", {"entry": model.entry})
    }

    actions: [
        Kirigami.Action {
            iconName: "media-playback-start"
            text: "Play"
            onTriggered: {
            audio.entry = model.entry
            audio.play()
            }
        },
        Kirigami.Action {
            text: i18n("Download")
            icon.name: "download"
            onTriggered: model.entry.enclosure.download()
            visible: model.entry.enclosure && model.entry.enclosure.status === Enclosure.Downloadable
        },
        Kirigami.Action {
            text: i18n("Cancel download")
            icon.name: "edit-delete-remove"
            onTriggered: model.entry.enclosure.cancelDownload()
            visible: model.entry.enclosure && model.entry.enclosure.status === Enclosure.Downloading
        },
        Kirigami.Action {
            text: i18n("Delete download")
            icon.name: "delete"
            onTriggered: model.entry.enclosure.deleteFile()
            visible: model.entry.enclosure && model.entry.enclosure.status === Enclosure.Downloaded
        },
        Kirigami.Action {
            text: i18n("Remove from Queue")
            icon.name: "delete-table-row"
            onTriggered: { DataManager.removeQueueItem(model.entry) }
        }
    ]
}

