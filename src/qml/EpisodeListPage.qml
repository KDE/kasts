/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtMultimedia 5.15
import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {

    property var episodeType: EpisodeModel.All

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
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile || episodeList.count === 0
    }

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: episodeType === EpisodeModel.All ? i18n("No Episodes Available")
              : episodeType === EpisodeModel.New ? i18n("No New Episodes")
              : episodeType === EpisodeModel.Unread ? i18n("No Unplayed Episodes")
              : episodeType === EpisodeModel.Downloaded ? i18n("No Downloaded Episodes")
              : episodeType === EpisodeModel.Downloading ? i18n("No Downloads in Progress")
              : i18n("No Episodes Available"))
    }

    Component {
        id: episodeListDelegate
        GenericEntryDelegate {
            listView: episodeList
            isDownloads: episodeType == EpisodeModel.Downloaded || episodeType == EpisodeModel.Downloading
        }
    }

    EpisodeModel {
        id: episodeModel
        type: episodeType
    }

    ListView {
        id: episodeList
        anchors.fill: parent
        visible: count !== 0
        model: episodeType === EpisodeModel.Downloading ? DownloadProgressModel
                                                        : episodeModel

        delegate: Kirigami.DelegateRecycler {
            width: episodeList.width
            sourceComponent: episodeListDelegate
        }
    }
}
