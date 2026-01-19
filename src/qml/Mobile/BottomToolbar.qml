/*
 * Copyright 2021 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.NavigationTabBar {
    id: root
    position: Controls.ToolBar.Footer

    property alias toolbarHeight: root.implicitHeight
    property var mymain: Controls.ApplicationWindow.window as Main

    // Keep track of the settings page being opened on the layer stack for mobile
    readonly property bool settingsOpened: Kirigami.Settings.isMobile && mymain.pageStack.layers.depth >= 2 && mymain.pageStack.layers.currentItem.title === "Settings"

    actions: [
        Kirigami.Action {
            icon.name: "view-media-playlist"
            text: KI18n.i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")
            checked: "QueuePage" === (root.Controls.ApplicationWindow.window as Main).currentPage && !root.settingsOpened
            onTriggered: {
                root.mymain.pushPage("QueuePage");
            }
        },
        Kirigami.Action {
            icon.name: "bookmarks"
            text: KI18n.i18nc("@title of page with list of podcast subscriptions", "Subscriptions")
            checked: "FeedListPage" === (root.Controls.ApplicationWindow.window as Main).currentPage && !root.settingsOpened
            onTriggered: {
                root.mymain.pushPage("FeedListPage");
            }
        },
        Kirigami.Action {
            icon.name: "rss"
            text: KI18n.i18nc("@title of page with list of podcast episodes", "Episodes")
            checked: "EpisodeListPage" === (root.Controls.ApplicationWindow.window as Main).currentPage && !root.settingsOpened
            onTriggered: {
                root.mymain.pushPage("EpisodeListPage");
            }
        },
        Kirigami.Action {
            icon.name: "settings-configure"
            text: KI18n.i18nc("@title of dialog with app settings", "Settings")
            checked: root.settingsOpened
            onTriggered: {
                root.mymain.pushPage("SettingsView");
            }
        }
    ]
}
