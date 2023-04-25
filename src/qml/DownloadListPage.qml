/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Downloads")

    property var lastEntry: ""
    property string pageName: "downloadpage"

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
        }
    ]

    Component.onCompleted: {
        for (var i in episodeList.defaultActionList) {
            pageActions.push(episodeList.defaultActionList[i]);
        }
    }

    // TODO: KF6 replace contextualActions with actions
    contextualActions: pageActions

    GenericEntryListView {
        id: episodeList
        isDownloads: true
        reuseItems: true

        Kirigami.PlaceholderMessage {
            visible: episodeList.count === 0

            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent

            text: i18n("No Downloads")
        }

        model: DownloadModel

        delegate: Component {
            id: episodeListDelegate
            GenericEntryDelegate {
                listView: episodeList
                isDownloads: true
            }
        }

        section {
            delegate: Kirigami.ListSectionHeader {
                height: implicitHeight // workaround for bug 422289
                label: section == Enclosure.Downloading ? i18n("Downloading") :
                       section == Enclosure.PartiallyDownloaded ? i18n("Incomplete Downloads") :
                       section == Enclosure.Downloaded ? i18n("Downloaded") :
                       ""
            }
            property: "entry.enclosure.status"
        }
    }
}
