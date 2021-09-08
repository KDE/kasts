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
import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {

    title: i18n("Episode List")

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    actions {
        main: Kirigami.Action {
            iconName: "view-refresh"
            text: i18n("Refresh All Podcasts")
            onTriggered: refreshing = true
            visible: !Kirigami.Settings.isMobile || episodeList.count === 0
        }
    }

    Kirigami.PlaceholderMessage {
        visible: episodeList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Episodes Available")
    }

    Component {
        id: episodeListDelegate
        GenericEntryDelegate {
            listView: episodeList
        }
    }

    EpisodeProxyModel {
        id: episodeProxyModel
    }

    ListView {
        id: episodeList
        anchors.fill: parent
        visible: count !== 0
        model: episodeProxyModel

        delegate: Kirigami.DelegateRecycler {
            width: episodeList.width
            sourceComponent: episodeListDelegate
        }
    }
}
