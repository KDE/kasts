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

    //pageStack.initialPage: SettingsManager.lastOpenedPage === "FeedListPage" ? feedList
    //                        : SettingsManager.lastOpenedPage === "QueuePage" ? queueList
    //                        : feedList
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

    /*FeedListPage  {
        id: feedList
    }

    QueuePage {
        id: queueList
    }*/

    AudioManager {
        id: audio
        playerOpen: false
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

    footer: Loader {
        active: (audio.entry != undefined) && !audio.playerOpen
        visible: (audio.entry != undefined) && !audio.playerOpen
        sourceComponent: MinimizedPlayerControls { }
    }

    /*
    Loader {
        id: footerLoader

        property var minimizedSize: Kirigami.Units.gridUnit * 3.0

        anchors.fill: parent
        active: (audio.entry == undefined) ? false : true
        visible: active
        z: (!item || item.contentY == 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: root.height * 2
            focus: true
        }

    }

    Item {
        visible: (audio.entry !== undefined)
        height: footerLoader.minimizedSize
    }
    */

    /*Kirigami.OverlaySheet {
        id: playeroverlay
        sheetOpen: False
        PlayerControls {
            height: root.height*5.0/6.0
            width: root.width*5.0/6.0;
        }
    }*/
}
