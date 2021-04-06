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
        Kirigami.Icon {
            source: model.entry.image === "" ? "rss" : Fetcher.image(model.entry.image)
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
        }
    ]
}
