/*
 * Copyright 2021 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Kirigami.NavigationTabBar {
    id: navBar

    property alias toolbarHeight: navBar.implicitHeight
    property bool transparentBackground: false

    shadow: false

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
                applicationWindow().pageStack.clear()
                applicationWindow().pageStack.push("qrc:/SettingsPage.qml", {}, {
                    title: i18n("Settings")
                })
            }
        }
    ]
}
