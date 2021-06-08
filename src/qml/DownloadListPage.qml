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

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            Fetcher.fetchAll()
            refreshing = false
        }
    }

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh All Podcasts")
        visible: !Kirigami.Settings.isMobile
        onTriggered: Fetcher.fetchAll()
    }

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Downloads")
    }

    Component {
        id: episodeListDelegate
        GenericEntryDelegate {
            listView: episodeList
            isDownloads: true
        }
    }

    ListView {
        id: episodeList
        visible: count !== 0
        model: DownloadModel

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

        delegate: Kirigami.DelegateRecycler {
            width: episodeList.width
            sourceComponent: episodeListDelegate
        }
    }
}
