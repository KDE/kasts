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
    id: root

    property alias toolbarHeight: root.implicitHeight
    property bool transparentBackground: false

    shadow: false

    actions: [
        Kirigami.Action {
            iconName: "view-media-playlist"
            text: i18n("Queue")
            checked: "QueuePage" === SettingsManager.lastOpenedPage
            onTriggered: {
                pushPage("QueuePage");
                SettingsManager.lastOpenedPage = "QueuePage"; // for persistency
                SettingsManager.save();
            }
        },
        Kirigami.Action {
            iconName: "bookmarks"
            text: i18n("Subscriptions")
            checked: "FeedListPage" === SettingsManager.lastOpenedPage
            onTriggered: {
                pushPage("FeedListPage");
                SettingsManager.lastOpenedPage = "FeedListPage"; // for persistency
                SettingsManager.save();
            }
        },
        Kirigami.Action {
            iconName: "rss"
            text: i18n("Episodes")
            checked: "EpisodeListPage" === SettingsManager.lastOpenedPage
            onTriggered: {
                pushPage("EpisodeListPage")
                SettingsManager.lastOpenedPage = "EpisodeListPage" // for persistency
                SettingsManager.save();
            }
        },
        Kirigami.Action {
            iconName: "settings-configure"
            text: i18n("Settings")
            checked: "SettingsPage" === SettingsManager.lastOpenedPage
            onTriggered: {
                applicationWindow().pageStack.clear()
                applicationWindow().pageStack.push("qrc:/SettingsPage.qml", {}, {
                    title: i18n("Settings")
                })
            }
        }
    ]
}
