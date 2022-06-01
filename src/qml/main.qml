/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.12

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kasts.solidextras 1.0

import org.kde.kasts 1.0

Kirigami.ApplicationWindow {
    id: root
    title: "Kasts"

    minimumWidth: Kirigami.Units.gridUnit * 17
    minimumHeight: Kirigami.Units.gridUnit * 12

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    property int bottomMessageSpacing: {
        if (Kirigami.Settings.isMobile) {
            return Kirigami.Units.largeSpacing + ( AudioManager.entry ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 )
        } else {
            return Kirigami.Units.largeSpacing;
        }
    }
    property int originalWidth: Kirigami.Units.gridUnit * 10
    property var lastFeed: ""
    property string currentPage: ""

    property bool isWidescreen: root.width >= root.height

    function getPage(page) {
        switch (page) {
            case "QueuePage": return "qrc:/QueuePage.qml";
            case "EpisodeListPage": return "qrc:/EpisodeListPage.qml";
            case "DiscoverPage": return "qrc:/DiscoverPage.qml";
            case "FeedListPage": return "qrc:/FeedListPage.qml";
            case "DownloadListPage": return "qrc:/DownloadListPage.qml";
            case "SettingsPage": return "qrc:/Settings/SettingsPage.qml";
            default: return "qrc:/FeedListPage.qml";
        }
    }
    function pushPage(page) {
        pageStack.clear()
        pageStack.layers.clear()
        pageStack.push(getPage(page))
        currentPage = page
    }

    Component.onCompleted: {
        currentPage = SettingsManager.lastOpenedPage
        pageStack.initialPage = getPage(SettingsManager.lastOpenedPage)

        if (Kirigami.Settings.isMobile) {
            pageStack.globalToolBar.style = Kirigami.ApplicationHeaderStyle.ToolBar;
            pageStack.globalToolBar.showNavigationButtons = Kirigami.ApplicationHeaderStyle.ShowBackButton;
        }

        // Delete played enclosures if set in settings
        if (SettingsManager.autoDeleteOnPlayed == 2) {
            DataManager.deletePlayedEnclosures();
        }

        // Refresh feeds on startup if allowed
        // NOTE: refresh+sync on startup is handled in Sync and not here, since it
        // requires credentials to be loaded before starting a refresh+sync
        if (NetworkStatus.connectivity != NetworkStatus.No && (SettingsManager.allowMeteredFeedUpdates || NetworkStatus.metered !== NetworkStatus.Yes)) {
            if (SettingsManager.refreshOnStartup && !(SettingsManager.syncEnabled && SettingsManager.syncWhenUpdatingFeeds)) {
                Fetcher.fetchAll();
            }
        }
    }

    globalDrawer: sidebar.item
    Loader {
        id: sidebar
        active: !Kirigami.Settings.isMobile || root.isWidescreen
        sourceComponent: Kirigami.GlobalDrawer {
            modal: false
            isMenu: false
            collapsible: !Kirigami.Settings.isMobile
            collapsed: !root.isWidescreen
            width: root.isWidescreen ? root.originalWidth : Layout.implicitWidth
            header: Kirigami.AbstractApplicationHeader {}

            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.inherit: false

            actions: [
                Kirigami.Action {
                    text: i18n("Queue")
                    iconName: "source-playlist"
                    checked: currentPage == "QueuePage"
                    onTriggered: {
                        pushPage("QueuePage")
                        SettingsManager.lastOpenedPage = "QueuePage" // for persistency
                    }
                },
                Kirigami.Action {
                    text: i18n("Discover")
                    iconName: "search"
                    checked: currentPage == "DiscoverPage"
                    onTriggered: {
                        pushPage("DiscoverPage")
                        SettingsManager.lastOpenedPage = "DiscoverPage" // for persistency
                    }
                },
                Kirigami.Action {
                    text: i18n("Subscriptions")
                    iconName: "bookmarks"
                    checked: currentPage == "FeedListPage"
                    onTriggered: {
                        pushPage("FeedListPage")
                        SettingsManager.lastOpenedPage = "FeedListPage" // for persistency
                    }
                },
                Kirigami.Action {
                    text: i18n("Episodes")
                    iconName: "rss"
                    checked: currentPage == "EpisodeListPage"
                    onTriggered: {
                        pushPage("EpisodeListPage")
                        SettingsManager.lastOpenedPage = "EpisodeListPage" // for persistency
                    }
                },
                Kirigami.Action {
                    text: i18n("Downloads")
                    iconName: "download"
                    checked: currentPage == "DownloadListPage"
                    onTriggered: {
                        pushPage("DownloadListPage")
                        SettingsManager.lastOpenedPage = "DownloadListPage" // for persistency
                    }
                },
                Kirigami.Action {
                    text: i18n("Settings")
                    iconName: "settings-configure"
                    checked: currentPage == "SettingsPage"
                    onTriggered: {
                        root.pageStack.layers.clear()
                        root.pageStack.pushDialogLayer("qrc:/SettingsPage.qml", {}, {
                            title: i18n("Settings")
                        })
                    }
                }
            ]
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
        // make room at the bottom for miniplayer
        handle.anchors.bottomMargin: ( (AudioManager.entry && Kirigami.Settings.isMobile) ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 ) + Kirigami.Units.smallSpacing
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

        sourceComponent: HeaderBar { focus: true }
    }

    // create space at the bottom to show miniplayer without it hiding stuff
    // underneath
    pageStack.anchors.bottomMargin: (AudioManager.entry && Kirigami.Settings.isMobile) ? miniplayerSize + 1 : 0

    Loader {
        id: footerLoader

        anchors.fill: parent
        active: AudioManager.entry && Kirigami.Settings.isMobile
        visible: active
        z: (!item || item.contentY === 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: root.height * 2
            focus: true
            contentToPlayerSpacing: footer.active ? footer.item.height + 1 : 0
        }
    }

    Loader {
        id: footerShadowLoader
        active: footer.active && !footerLoader.active
        anchors.fill: footer

        sourceComponent: RectangularGlow {
            glowRadius: 5
            spread: 0.3
            color: Qt.rgba(0.0, 0.0, 0.0, 0.1)
        }
    }

    footer: Loader {
        visible: active
        active: Kirigami.Settings.isMobile && !root.isWidescreen
        sourceComponent: BottomToolbar {
            transparentBackground: footerLoader.active
            opacity: (!footerLoader.item || footerLoader.item.contentY === 0) ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.shortDuration }
            }
        }
    }

    // Notification that shows the progress of feed updates
    // It mimicks the behaviour of an InlineMessage, because InlineMessage does
    // not allow to add a BusyIndicator
    UpdateNotification {
        id: updateNotification
        text: i18ncp("Number of Updated Podcasts",
                     "Updated %2 of %1 Podcast",
                     "Updated %2 of %1 Podcasts",
                     Fetcher.updateTotal,
                     Fetcher.updateProgress)

        showAbortButton: true

        function abortAction() {
            Fetcher.cancelFetching();
        }

        Connections {
            target: Fetcher
            function onUpdatingChanged() {
                if (Fetcher.updating) {
                    updateNotification.open();
                } else {
                    updateNotification.close();
                }
            }
        }
    }

    // Notification to show progress of copying enclosure and images to new location
    UpdateNotification {
        id: moveStorageNotification
        text: i18ncp("Number of Moved Files",
                     "Moved %2 of %1 File",
                     "Moved %2 of %1 Files",
                     StorageManager.storageMoveTotal,
                     StorageManager.storageMoveProgress)
        showAbortButton: true

        function abortAction() {
            StorageManager.cancelStorageMove();
        }

        Connections {
            target: StorageManager
            function onStorageMoveStarted() {
                moveStorageNotification.open()
            }
            function onStorageMoveFinished() {
                moveStorageNotification.close()
            }
        }
    }

    // Notification that shows the progress of feed and episode syncing
    UpdateNotification {
        id: updateSyncNotification
        text: Sync.syncProgressText
        showAbortButton: true

        function abortAction() {
            Sync.abortSync();
        }

        Connections {
            target: Sync
            function onSyncProgressChanged() {
                if (Sync.syncStatus != SyncUtils.NoSync && Sync.syncProgress === 0) {
                    updateSyncNotification.open();
                } else if (Sync.syncStatus === SyncUtils.NoSync) {
                    updateSyncNotification.close();
                }
            }
        }
    }


    // This InlineMessage is used for displaying error messages
    ErrorNotification {
        id: errorNotification
    }

    // overlay with log of all errors that have happened
    ErrorListOverlay {
        id: errorOverlay
    }

    // This item can be used to trigger an update of all feeds; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: updateAllFeeds
    }

    // Overlay with options what to do when metered downloads are not allowed
    ConnectionCheckAction {
        id: downloadOverlay

        headingText: i18n("Podcast downloads are currently not allowed on metered connections")
        condition: SettingsManager.allowMeteredEpisodeDownloads
        property var entry: undefined
        property var selection: undefined

        function action() {
            if (selection) {
                DataManager.bulkDownloadEnclosuresByIndex(selection);
            } else if (entry) {
                entry.queueStatus = true;
                entry.enclosure.download();
            }
            selection = undefined;
            entry = undefined;
        }

        function allowOnceAction() {
            SettingsManager.allowMeteredEpisodeDownloads = true;
            action();
            SettingsManager.allowMeteredEpisodeDownloads = false;
        }

        function alwaysAllowAction() {
            SettingsManager.allowMeteredEpisodeDownloads = true;
            action();
        }
    }

    PlaybackRateDialog {
        id: playbackRateDialog
    }

    Connections {
        target: Sync
        function onPasswordInputRequired() {
            syncPasswordOverlay.open();
        }
    }

    SyncPasswordOverlay {
        id: syncPasswordOverlay
    }

    //Global Shortcuts
    Shortcut {
        sequence:  "space"
        enabled: AudioManager.canPlay
        onActivated: AudioManager.playPause()
    }
    Shortcut {
        sequence:  "n"
        enabled: AudioManager.canGoNext
        onActivated: AudioManager.next()
    }
}
