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

import org.kde.kirigami 2.14 as Kirigami

import org.kde.alligator 1.0

Kirigami.ApplicationWindow {
    id: root
    title: "Alligator"

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    property int tabBarHeight: Kirigami.Units.gridUnit * 2
    property int tabBarActive: 0

    Kirigami.PagePool {
        id: mainPagePool
        cachePages: false
    }

    pageStack.initialPage: mainPagePool.loadPage(SettingsManager.lastOpenedPage === "FeedListPage" ? "qrc:/FeedListPage.qml"
                                                 : SettingsManager.lastOpenedPage === "QueuePage" ? "qrc:/QueuePage.qml"
                                                 : SettingsManager.lastOpenedPage === "EpisodeSwipePage" ? "qrc:/EpisodeSwipePage.qml"
                                                 : SettingsManager.lastOpenedPage === "DownloadSwipePage" ? "qrc:/DownloadSwipePage.qml"
                                                 : "qrc:/FeedListPage.qml")

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: false
        // make room at the bottom for miniplayer
        handle.anchors.bottomMargin: ( audio.entry ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 ) + Kirigami.Units.smallSpacing + tabBarActive * tabBarHeight
        handleVisible: !audio.entry || footerLoader.item.contentY == 0
        actions: [
            Kirigami.PagePoolAction {
                text: i18n("Queue")
                iconName: "source-playlist"
                pagePool: mainPagePool
                page: "qrc:/QueuePage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "QueuePage" // for persistency
                    tabBarActive = 0
                }
            },
            Kirigami.PagePoolAction {
                text: i18n("Episodes")
                iconName: "rss"
                pagePool: mainPagePool
                page: "qrc:/EpisodeSwipePage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "EpisodeSwipePage" // for persistency
                    tabBarActive = 1
                }
            },
            Kirigami.PagePoolAction {
                text: i18n("Subscriptions")
                iconName: "document-open-folder"
                pagePool: mainPagePool
                page: "qrc:/FeedListPage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "FeedListPage" // for persistency
                    tabBarActive = 0
                }
            },
            Kirigami.PagePoolAction {
                text: i18n("Downloads")
                iconName: "download"
                pagePool: mainPagePool
                page: "qrc:/DownloadSwipePage.qml"
                onTriggered: {
                    SettingsManager.lastOpenedPage = "DownloadSwipePage" // for persistency
                    tabBarActive = 1
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
        // make room at the bottom for miniplayer
        handle.anchors.bottomMargin: ( audio.entry ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 ) + Kirigami.Units.smallSpacing + tabBarActive * tabBarHeight
        handleVisible: !audio.entry || footerLoader.item.contentY == 0
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
}
