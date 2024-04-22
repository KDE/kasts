/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtCore

import org.kde.kirigami as Kirigami

import org.kde.kasts
import org.kde.kasts.settings

import "Desktop"
import "Mobile"


Kirigami.ApplicationWindow {
    id: kastsMainWindow
    title: i18n("Kasts")

    property bool isMobile: Kirigami.Settings.isMobile
    width: isMobile ? 360 : 800
    height: isMobile ? 660 : 600

    pageStack.clip: true
    pageStack.popHiddenPages: true
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar;
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton;

    // only have a single page visible at any time
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn

    minimumWidth: Kirigami.Units.gridUnit * 17
    minimumHeight: Kirigami.Units.gridUnit * 12

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    property int bottomMessageSpacing: {
        if (isMobile) {
            return Kirigami.Units.largeSpacing + ( AudioManager.entry ? ( footerLoader.item.contentY == 0 ? miniplayerSize : 0 ) : 0 )
        } else {
            return Kirigami.Units.largeSpacing;
        }
    }
    property var lastFeed: ""
    property string currentPage: ""
    property int feedSorting: FeedsProxyModel.UnreadDescending

    property bool isWidescreen: kastsMainWindow.width > kastsMainWindow.height

    function getPage(page) {
        switch (page) {
            case "QueuePage": return "qrc:/qt/qml/org/kde/kasts/qml/QueuePage.qml";
            case "EpisodeListPage": return "qrc:/qt/qml/org/kde/kasts/qml/EpisodeListPage.qml";
            case "DiscoverPage": return "qrc:/qt/qml/org/kde/kasts/qml/DiscoverPage.qml";
            case "FeedListPage": return "qrc:/qt/qml/org/kde/kasts/qml/FeedListPage.qml";
            case "DownloadListPage": return "qrc:/qt/qml/org/kde/kasts/qml/DownloadListPage.qml";
            case "SettingsPage": return "qrc:/qt/qml/org/kde/kasts/qml/Settings/SettingsPage.qml";
            default: {
                currentPage = "FeedListPage";
                return "qrc:/qt/qml/org/kde/kasts/qml/FeedListPage.qml";
            }
        }
    }
    function pushPage(page) {
        if (page === "SettingsPage") {
            pageStack.layers.clear()
            pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/kasts/qml/Settings/SettingsPage.qml", {}, {
                title: i18n("Settings")
            })
        } else {
            pageStack.clear();
            pageStack.layers.clear();
            pageStack.push(getPage(page));
            currentPage = page;
        }
    }

    Settings {
        id: settings

        property alias x: kastsMainWindow.x
        property alias y: kastsMainWindow.y
        property var mobileWidth
        property var mobileHeight
        property var desktopWidth
        property var desktopHeight
        property int headerSize: Kirigami.Units.gridUnit * 5
        property alias lastOpenedPage: kastsMainWindow.currentPage
        property alias feedSorting: kastsMainWindow.feedSorting
        property int episodeListFilterType: AbstractEpisodeProxyModel.NoFilter
    }

    function saveWindowLayout() {
        if (isMobile) {
            settings.mobileWidth = kastsMainWindow.width;
            settings.mobileHeight = kastsMainWindow.height;
        } else {
            settings.desktopWidth = kastsMainWindow.width;
            settings.desktopHeight = kastsMainWindow.height;
        }
    }

    function restoreWindowLayout() {
        if (isMobile) {
            if (settings.mobileWidth) kastsMainWindow.width = settings.mobileWidth;
            if (settings.mobileHeight) kastsMainWindow.height = settings.mobileHeight;
        } else {
            if (settings.desktopWidth) kastsMainWindow.width = settings.desktopWidth;
            if (settings.desktopHeight) kastsMainWindow.height = settings.desktopHeight;
        }
    }

    Component.onDestruction: {
        saveWindowLayout();
    }

    Component.onCompleted: {
        restoreWindowLayout();
        pageStack.initialPage = getPage(currentPage);

        // Delete played enclosures if set in settings
        if (SettingsManager.autoDeleteOnPlayed == 2) {
            DataManager.deletePlayedEnclosures();
        }

        // Refresh feeds on startup if allowed
        // NOTE: refresh+sync on startup is handled in Sync and not here, since it
        // requires credentials to be loaded before starting a refresh+sync
        if (NetworkConnectionManager.feedUpdatesAllowed) {
            if (SettingsManager.refreshOnStartup && !(SettingsManager.syncEnabled && SettingsManager.syncWhenUpdatingFeeds)) {
                Fetcher.fetchAll();
            }
        }
    }

    property bool showGlobalDrawer: !isMobile || kastsMainWindow.isWidescreen

    globalDrawer: showGlobalDrawer ? myGlobalDrawer : null

    property Kirigami.OverlayDrawer myGlobalDrawer: KastsGlobalDrawer {

    }

    // Implement slots for MPRIS2 signals
    Connections {
        target: AudioManager
        function onRaiseWindowRequested() {
            kastsMainWindow.visible = true;
            kastsMainWindow.show();
            kastsMainWindow.raise();
            kastsMainWindow.requestActivate();
        }
    }
    Connections {
        target: AudioManager
        function onQuitRequested() {
            kastsMainWindow.close();
        }
    }

    header: Loader {
        id: headerLoader
        active: !isMobile
        visible: active

        sourceComponent: HeaderBar { focus: true }
    }

    // create space at the bottom to show miniplayer without it hiding stuff
    // underneath
    pageStack.anchors.bottomMargin: (AudioManager.entry && isMobile) ? miniplayerSize + 1 : 0

    Loader {
        id: footerLoader

        anchors.fill: parent
        active: AudioManager.entry && isMobile
        visible: active
        z: (!item || item.contentY === 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: kastsMainWindow.height * 2
            focus: true
            contentToPlayerSpacing: footer.active ? footer.item.height + 1 : 0
        }
    }

    Loader {
        id: footerShadowLoader
        active: footer.active && !footerLoader.active
        anchors.fill: footer

        sourceComponent: MultiEffect {
            source: bottomToolbarLoader
            shadowEnabled: true
            shadowScale: 1.1
            blurMax: 10
            shadowColor: Qt.rgba(0.0, 0.0, 0.0, 0.1)
        }
    }

    footer: Loader {
        id: bottomToolbarLoader
        visible: active
        height: visible ? implicitHeight : 0
        active: isMobile && !kastsMainWindow.isWidescreen
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

        headingText: i18nc("@info:status", "Podcast downloads are currently not allowed on metered connections")
        condition: NetworkConnectionManager.episodeDownloadsAllowed
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
            SettingsManager.save();
            action();
        }
    }

    SleepTimerDialog {
        id: sleepTimerDialog
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

    Loader {
        id: fullScreenImageLoader
        active: false
        visible: active
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

    // Systray implementation
    Connections {
        target: kastsMainWindow

        function onClosing(close) {
            if (SystrayIcon.available && SettingsManager.showTrayIcon && SettingsManager.minimizeToTray) {
                close.accepted = false;
                kastsMainWindow.hide();
            } else {
                close.accepted = true;
                Qt.quit();
            }
        }
    }

    Connections {
        target: SystrayIcon

        function onRaiseWindow() {
            if (kastsMainWindow.visible) {
                kastsMainWindow.visible = false;
                kastsMainWindow.hide();
            } else {
                kastsMainWindow.visible = true;
                kastsMainWindow.show();
                kastsMainWindow.raise();
                kastsMainWindow.requestActivate();
            }
        }
    }
}
