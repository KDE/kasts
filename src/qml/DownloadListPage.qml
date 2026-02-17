/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    title: KI18n.i18nc("@title of page with list of downloaded episodes", "Downloads")

    property int lastEntry: 0
    property string pageName: "downloadpage"

    supportsRefreshing: true
    onRefreshingChanged: {
        if (refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: KI18n.i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
        }
    ]

    Component.onCompleted: {
        for (var i in episodeList.defaultActionList) {
            pageActions.push(episodeList.defaultActionList[i]);
        }
    }

    actions: pageActions

    GenericEntryListView {
        id: episodeList
        isDownloads: true
        reuseItems: true

        Kirigami.PlaceholderMessage {
            visible: episodeList.count === 0

            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent

            icon.name: "download"
            text: KI18n.i18nc("@info:placeholder", "No downloads")
        }

        model: DownloadModel

        delegate: GenericEntryDelegate {
            listViewObject: episodeList
            isDownloads: true
        }

        section {
            delegate: Kirigami.ListSectionHeader {
                width: episodeList.width
                required property string section

                // NOTE: the Enclosure.Status enum values get converted to strings
                label: section == "Downloading" ? KI18n.i18n("Downloading") : section == "PartiallyDownloaded" ? KI18n.i18n("Incomplete Downloads") : section == "Downloaded" ? KI18n.i18n("Downloaded") : ""
            }
            property: "entry.enclosure.status"
        }
    }

    ConnectionCheckAction {
        id: updateAllFeeds
    }
}
