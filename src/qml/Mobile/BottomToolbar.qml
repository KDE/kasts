/*
 * Copyright 2021 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.NavigationTabBar {
    id: navBar

    property alias toolbarHeight: navBar.implicitHeight
    property bool transparentBackground: false

    actions: [
        Kirigami.Action {
            icon.name: "view-media-playlist"
            text: i18n("Queue")
            checked: "QueuePage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("QueuePage");
            }
        },
        Kirigami.Action {
            icon.name: "bookmarks"
            text: i18n("Subscriptions")
            checked: "FeedListPage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("FeedListPage");
            }
        },
        Kirigami.Action {
            icon.name: "rss"
            text: i18n("Episodes")
            checked: "EpisodeListPage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("EpisodeListPage")
            }
        },
        Kirigami.Action {
            icon.name: "settings-configure"
            text: i18n("Settings")
            checked: "SettingsPage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("SettingsPage")
            }
        }
    ]
}
