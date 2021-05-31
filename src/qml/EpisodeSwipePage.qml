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

Kirigami.Page {
    id: page

    title: i18n("Episode List")
    padding: 0

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh All Podcasts")
        visible: !Kirigami.Settings.isMobile
        onTriggered: Fetcher.fetchAll()
    }

    header: Loader {
        id: headerLoader
        active: !Kirigami.Settings.isMobile
        sourceComponent: tabBarComponent
    }

    footer: Loader {
        id: footerLoader
        active: Kirigami.Settings.isMobile
        sourceComponent: tabBarComponent
    }

    Component {
        id: tabBarComponent
        Controls.TabBar {
            id: tabBar
            position: Kirigami.Settings.isMobile ? Controls.TabBar.Footer : Controls.TabBar.Header
            currentIndex: swipeView.currentIndex
            contentHeight: tabBarHeight

            Controls.TabButton {
                width: parent.parent.width/parent.count
                height: tabBarHeight
                text: i18n("New Episodes")
            }
            Controls.TabButton {
                width: parent.parent.width/parent.count
                height: tabBarHeight
                text: i18n("All Episodes")
            }
        }
    }

    contentItem: Controls.SwipeView {
        id: swipeView

        anchors {
            top: page.header.bottom
            right: page.right
            left: page.left
            bottom: page.footer.top
        }

        currentIndex: Kirigami.Settings.isMobile ? footerLoader.item.currentIndex : headerLoader.item.currentIndex

        EpisodeListPage {
            title: i18n("New Episodes")
            episodeType: EpisodeModel.New
        }

        EpisodeListPage {
            title: i18n("All Episodes")
            episodeType: EpisodeModel.All
        }
    }
}
