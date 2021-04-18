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

import org.kde.alligator 1.0


Kirigami.ScrollablePage {
    //anchors.fill: parent

    property var episodeType: EpisodeModel.All

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            Fetcher.fetchAll()
            refreshing = false
        }
    }

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No %1 available", episodeType === EpisodeModel.All ? i18n("episodes")
                                      : episodeType === EpisodeModel.New ? i18n("new episodes")
                                      : episodeType === EpisodeModel.Unread ? i18n("unread episodes")
                                      : i18n("episodes"))
    }
    Component {
        id: episodeListDelegate
        GenericEntryDelegate {
            listView: episodeList
        }
    }
    ListView {
        anchors.fill: parent
        id: episodeList
        visible: count !== 0
        model: EpisodeModel { type: episodeType }

        delegate: Kirigami.DelegateRecycler {
             width: episodeList.width
            sourceComponent: episodeListDelegate
        }
    }
    actions.main: Kirigami.Action {
        text: i18n("Refresh all feeds")
        iconName: "view-refresh"
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile
    }
}
