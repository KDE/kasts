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
    position: ToolBar.Footer

    property alias toolbarHeight: navBar.implicitHeight

    actions: [
        Kirigami.Action {
            icon.name: "view-media-playlist"
            text: i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")
            checked: "QueuePage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("QueuePage");
            }
        },
        Kirigami.Action {
            icon.name: "bookmarks"
            text: i18nc("@title of page with list of podcast subscriptions", "Subscriptions")
            checked: "FeedListPage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("FeedListPage");
            }
        },
        Kirigami.Action {
            icon.name: "rss"
            text: i18nc("@title of page with list of podcast episodes", "Episodes")
            checked: "EpisodeListPage" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("EpisodeListPage")
            }
        },
        Kirigami.Action {
            icon.name: "settings-configure"
            text: i18nc("@title of dialog with app settings", "Settings")
            checked: "SettingsView" === kastsMainWindow.currentPage
            onTriggered: {
                pushPage("SettingsView");
            }
        }
    ]
}
