/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import QtMultimedia 5.15

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ApplicationWindow {
    id: root
    title: "Alligator"

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    Kirigami.PagePool {
        id: mainPagePool
        cachePages: true
    }

    pageStack.initialPage: mainPagePool.loadPage(SettingsManager.lastOpenedPage === "FeedListPage" ? "qrc:/FeedListPage.qml"
                                                 : SettingsManager.lastOpenedPage === "QueuePage" ? "qrc:/QueuePage.qml"
                                                 : "qrc:/FeedListPage.qml")

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: false
        actions: [
            Kirigami.PagePoolAction {
                text: i18n("Queue")
                iconName: "source-playlist"
                pagePool: mainPagePool
                page: "qrc:/QueuePage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "QueuePage" // for persistency
                }
            },
            Kirigami.PagePoolAction {
                text: i18n("Subscriptions")
                iconName: "rss"
                pagePool: mainPagePool
                page: "qrc:/FeedListPage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "FeedListPage" // for persistency
                }
            },
            Kirigami.PagePoolAction {
                text: i18n("Settings")
                iconName: "settings-configure"
                pagePool: mainPagePool
                page: "qrc:/SettingsPage.qml"
                useLayers: true
            },
            Kirigami.PagePoolAction {
                text: i18n("About")
                iconName: "help-about-symbolic"
                pagePool: mainPagePool
                page: "qrc:/AboutPage.qml"
                useLayers: true
                //enabled: pageStack.layers.currentItem.title !== i18n("About")
            }
        ]
    }

    Component {
        id: aboutPage

        Kirigami.AboutPage {
            title: i18n("About")
            aboutData: _aboutData
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    AudioManager {
        id: audio
    }

    Mpris2 {
        id: mpris2Interface

        playerName: 'alligator'
        audioPlayer: audio

        onRaisePlayer:
        {
            // TODO: implement
        }
    }

    // create space at the bottom to show miniplayer without it hiding stuff
    // underneath
    pageStack.anchors.bottomMargin: (audio.entry) ? miniplayerSize : 0
    Loader {
        id: footerLoader

        anchors.fill: parent
        active: (audio.entry) ? true : false
        visible: active
        z: (!item || item.contentY == 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: root.height * 2
            focus: true
        }

    }

    // Doesn't look like this is needed at all:
    // capture mouse events behind flickable when it is open
    /*MouseArea {
        visible: footerLoader.item.contentY != 0 // only capture when the mobile footer panel is open
        anchors.fill: footerLoader
        preventStealing: true
        onClicked: mouse.accepted = true
    }*/
}
