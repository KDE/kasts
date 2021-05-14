/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Kirigami.ApplicationWindow {
    id: root
    title: "Kasts"

    minimumWidth: Kirigami.Units.gridUnit * 17
    minimumHeight: Kirigami.Units.gridUnit * 20

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    property int tabBarHeight: Kirigami.Units.gridUnit * 2
    property int bottomMessageSpacing: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing * 9 + ( AudioManager.entry ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 ) + tabBarActive * tabBarHeight : Kirigami.Units.largeSpacing * 2
    property int tabBarActive: 0
    property int originalWidth: Kirigami.Units.gridUnit * 10

    property bool isWidescreen: root.width >= root.height
    onIsWidescreenChanged: {
        if (!Kirigami.Settings.isMobile) {
            changeNavigation(!isWidescreen);
        }
    }

    Kirigami.PagePool {
        id: mainPagePool
        cachePages: false
    }

    Component.onCompleted: {
        tabBarActive = SettingsManager.lastOpenedPage === "FeedListPage" ? 0
                     : SettingsManager.lastOpenedPage === "QueuePage" ? 0
                     : SettingsManager.lastOpenedPage === "EpisodeSwipePage" ? 1
                     : SettingsManager.lastOpenedPage === "DownloadSwipePage" ? 1
                     : 0
        pageStack.initialPage = mainPagePool.loadPage(SettingsManager.lastOpenedPage === "FeedListPage" ? "qrc:/FeedListPage.qml"
                                                    : SettingsManager.lastOpenedPage === "QueuePage" ? "qrc:/QueuePage.qml"
                                                    : SettingsManager.lastOpenedPage === "EpisodeSwipePage" ? "qrc:/EpisodeSwipePage.qml"
                                                    : SettingsManager.lastOpenedPage === "DownloadSwipePage" ? "qrc:/DownloadSwipePage.qml"
                                                    : "qrc:/FeedListPage.qml")
        if (SettingsManager.refreshOnStartup) Fetcher.fetchAll();
    }

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: false
        modal: Kirigami.Settings.isMobile
        collapsible: !Kirigami.Settings.isMobile
        header: Kirigami.AbstractApplicationHeader {
            visible: !Kirigami.Settings.isMobile
        }

        Component.onCompleted: {
            if (!Kirigami.Settings.isMobile) {
                Kirigami.Theme.colorSet = Kirigami.Theme.Window;
                Kirigami.Theme.inherit = false;
            }
        }

        // make room at the bottom for miniplayer
        handle.anchors.bottomMargin: (( AudioManager.entry && Kirigami.Settings.isMobile ) ? (footerLoader.item.contentY == 0 ? miniplayerSize : 0) : 0) + Kirigami.Units.smallSpacing + tabBarActive * tabBarHeight
        handleVisible: Kirigami.Settings.isMobile ? !AudioManager.entry || footerLoader.item.contentY === 0 : false
        showHeaderWhenCollapsed: true
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
            },
            Kirigami.PagePoolAction {
                text: i18n("About")
                iconName: "help-about-symbolic"
                pagePool: mainPagePool
                page: "qrc:/AboutPage.qml"
            }
        ]
    }

    function changeNavigation(isNarrow) {
        if(isNarrow) {
            globalDrawer.collapsed = true
            globalDrawer.width = Layout.implicitWidth
        }
        else {
            globalDrawer.collapsed = false
            globalDrawer.width = originalWidth
        }
    }

    Component {
        id: aboutPage

        Kirigami.AboutPage {
            aboutData: _aboutData
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
        // make room at the bottom for miniplayer
        handle.anchors.bottomMargin: ( (AudioManager.entry && Kirigami.Settings.isMobile) ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 ) + Kirigami.Units.smallSpacing + tabBarActive * tabBarHeight
        handleVisible: Kirigami.Settings.isMobile ? !AudioManager.entry || footerLoader.item.contentY === 0 : false
    }

    Mpris2 {
        id: mpris2Interface

        playerName: 'kasts'
        audioPlayer: AudioManager

        onRaisePlayer:
        {
            root.visible = true
            root.show()
            root.raise()
            root.requestActivate()
        }
    }

    header: Loader {
        id: headerLoader
        active: !Kirigami.Settings.isMobile
        visible: active

        sourceComponent: HeaderBar {
            focus: true
        }
    }

    // create space at the bottom to show miniplayer without it hiding stuff
    // underneath
    pageStack.anchors.bottomMargin: (AudioManager.entry && Kirigami.Settings.isMobile) ? miniplayerSize : 0

    Loader {
        id: footerLoader

        anchors.fill: parent
        active: AudioManager.entry && Kirigami.Settings.isMobile
        visible: active
        z: (!item || item.contentY === 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: root.height * 2
            focus: true
        }

    }

    UpdateNotification {
        z: 2
        id: updateNotification

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: bottomMessageSpacing + ( inlineMessage.visible ? inlineMessage.height + Kirigami.Units.largeSpacing : 0 )
        }
    }

    Kirigami.InlineMessage {
        id: inlineMessage
        anchors {
            bottom: parent.bottom
            right: parent.right
            left: parent.left
            margins: Kirigami.Units.gridUnit
            bottomMargin: bottomMessageSpacing
        }
        type: Kirigami.MessageType.Error
        showCloseButton: true

        Connections {
            target: ErrorLogModel
            function onNewErrorLogged(error) {
                inlineMessage.text = error.description + "\n" + i18n("Check Error Log Tab (under Downloads) for more details");
                inlineMessage.visible = true;
            }
        }
    }
}
