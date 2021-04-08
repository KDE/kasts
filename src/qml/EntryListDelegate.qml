/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.13 as Kirigami

import org.kde.alligator 1.0

Kirigami.SwipeListItem {

    contentItem: RowLayout {
        Image {
            asynchronous: true
            source: entry.image === "" ? "rss" : "file://"+Fetcher.image(entry.image)
            fillMode: Image.PreserveAspectFit
            property int size: Kirigami.Units.gridUnit * 3
            Layout.maximumHeight: size
            Layout.maximumWidth: size
            sourceSize.width: size
            sourceSize.height: size
            Layout.rightMargin: Kirigami.Units.smallSpacing
        }
        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            RowLayout{
                Kirigami.Icon {
                    Layout.maximumHeight: 0.7 * supertitle.implicitHeight
                    Layout.maximumWidth:  0.7 * supertitle.implicitHeight
                    source: "source-playlist"
                    visible: entry.queueStatus
                    opacity: (entry.read) ? 0.4 : 0.7
                }
                Controls.Label {
                    id: supertitle
                    text: (entry.queueStatus ? "·  " : "") + entry.updated.toLocaleDateString(Qt.locale(), Locale.NarrowFormat) + (entry.enclosure ? ( entry.enclosure.size !== 0 ? "  ·  " + Math.floor(entry.enclosure.size/(1024*1024)) + "MB" : "") : "" )
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
                sourceComponent: entry.enclosure && entry.enclosure.status === Enclosure.Downloading ? downloadProgress : subtitle
                Layout.fillWidth: true
                Component {
                    id: subtitle
                    Controls.Label {
                        text: (Math.floor(entry.enclosure.duration/3600) < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/3600) + ":" + (Math.floor(entry.enclosure.duration/60) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration/60) % 60 + ":" + (Math.floor(entry.enclosure.duration) % 60 < 10 ? "0" : "") + Math.floor(entry.enclosure.duration) % 60
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: (entry.read) ? 0.4 : 0.7
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
            text: i18n("Add to queue")
            icon.name: "media-playlist-append"
            visible: entry.enclosure  && !entry.queueStatus
            onTriggered: { DataManager.addtoQueue(entry.feed.url, entry.id) }
        }
    ]
}
