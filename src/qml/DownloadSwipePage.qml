/**
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

    title: i18n("Downloads")
    padding: 0

    header: Loader {
        id: headerLoader
        active: !Kirigami.Settings.isMobile
        sourceComponent: tabBarComponent
        property var swipeViewItem: swipeView
    }

    footer: Loader {
        id: footerLoader
        active: Kirigami.Settings.isMobile
        sourceComponent: tabBarComponent
        property var swipeViewItem: swipeView
    }

    Component {
        id: tabBarComponent
        Controls.TabBar {
            id: tabBar
            position: Controls.TabBar.Footer
            currentIndex: swipeViewItem.currentIndex

            Controls.TabButton {
                width: parent.parent.width/parent.count
                height: Kirigami.Units.gridUnit * 2
                text: i18n("Downloaded")
            }
        }
    }

    Controls.SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: Kirigami.Settings.isMobile ? footerLoader.item.currentIndex : headerLoader.item.currentIndex

        EpisodeListPage {
            title: i18n("Downloaded")
            episodeType: EpisodeModel.Downloaded
        }
    }
}
