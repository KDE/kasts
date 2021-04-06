/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.SwipeListItem {

    contentItem: RowLayout {
        Image {
            asynchronous: true
            source: entry.image === "" ? "rss" : "file://"+Fetcher.image(entry.image)
            fillMode: Image.PreserveAspectFit
            property int size: Kirigami.Units.iconSizes.medium
            Layout.maximumHeight: size
            Layout.maximumWidth: size
            sourceSize.width: size
            sourceSize.height: size
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
        }
    ]
}
