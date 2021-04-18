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

Kirigami.Page {
    id: page

    title: i18n("Episode List")
    padding: 0

    header: Controls.TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex

        Controls.TabButton {
            text: "New Episodes"
        }
        Controls.TabButton {
            text: "All Episodes"
        }
    }

    Controls.SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        EpisodeListPage {
            title: i18n("New Episodes")
            episodeType: EpisodeModel.New
        }

        EpisodeListPage {
            title: i18n("All Episodes")
            episodeType: EpisodeModel.All
        }
    }

    /*actions.main: Kirigami.Action {
        text: i18n("Refresh all feeds")
        iconName: "view-refresh"
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile
    }*/
}
