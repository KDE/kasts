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
import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page

    title: i18n("Episode List")

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            Fetcher.fetchAll()
            refreshing = false
        }
    }

    actions.main: Kirigami.Action {
        text: i18n("Refresh all feeds")
        iconName: "view-refresh"
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile
    }

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Entries available")
    }

    Component {
        id: entryListDelegate
        GenericEntryDelegate {
            listView: episodeList
        }
    }

    ListView {
        id: episodeList
        visible: count !== 0
        model: EpisodeModel { type: EpisodeModel.All }

        delegate: Kirigami.DelegateRecycler {
            width: episodeList.width
            sourceComponent: entryListDelegate
        }
    }
}
