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
import org.kde.config as KConfig

import org.kde.kasts

import "Desktop"
import "Mobile"

Kirigami.ApplicationWindow {
    id: kastsMainWindow

    title: i18n("Kasts")

    width: Kirigami.Settings.isMobile ? Kirigami.Units.gridUnit * 20 : Kirigami.Units.gridUnit * 45
    height: Kirigami.Settings.isMobile ? Kirigami.Units.gridUnit * 37 : Kirigami.Units.gridUnit * 34

    pageStack.clip: true
    pageStack.popHiddenPages: true
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton

    // only have a single page visible at any time
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn

    minimumWidth: Kirigami.Units.gridUnit * 17
    minimumHeight: Kirigami.Units.gridUnit * 12

    property var miniplayerSize: Kirigami.Units.gridUnit * 3 + Kirigami.Units.gridUnit / 6
    property int bottomMessageSpacing: {
        if (Kirigami.Settings.isMobile) {
            return Kirigami.Units.largeSpacing + (AudioManager.entry ? ((footerLoader.item as FooterBar).contentY == 0 ? miniplayerSize : 0) : 0);
        } else {
            return Kirigami.Units.largeSpacing;
        }
    }
    property string lastFeed: ""
    property string currentPage: ""
    property int feedSorting: FeedsProxyModel.UnreadDescending

    function pushPage(page: string): void {
        if (page === "SettingsView") {
            settingsView.open();
        } else {
            var pageObject = Qt.createComponent("org.kde.kasts", page);
            if (!pageObject) {
                page = "FeedListPage";
                pageObject = Qt.createComponent("org.kde.kasts", page);
            }
            pageStack.clear();
            pageStack.layers.clear();
            pageStack.push(pageObject);
            currentPage = page;
        }
    }

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    SettingsView {
        id: settingsView
        window: kastsMainWindow
    }

    Settings {
        id: settings

        property alias lastOpenedPage: kastsMainWindow.currentPage
        property alias feedSorting: kastsMainWindow.feedSorting
        property int episodeListFilterType: AbstractEpisodeProxyModel.NoFilter
    }

    Component.onCompleted: {
        pageStack.initialPage = pushPage(currentPage);

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

    property bool showGlobalDrawer: !Kirigami.Settings.isMobile || WindowUtils.isWidescreen

    globalDrawer: globalDrawerLoader.item

    Loader {
        id: globalDrawerLoader
        active: kastsMainWindow.showGlobalDrawer
        sourceComponent: KastsGlobalDrawer {}
    }

    // Implement slots for MPRIS2 signals
    Connections {
        target: AudioManager
        function onRaiseWindowRequested(): void {
            kastsMainWindow.visible = true;
            kastsMainWindow.show();
            kastsMainWindow.raise();
            kastsMainWindow.requestActivate();
        }
    }
    Connections {
        target: AudioManager
        function onQuitRequested(): void {
            kastsMainWindow.close();
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
    pageStack.anchors.bottomMargin: (AudioManager.entry && Kirigami.Settings.isMobile) ? miniplayerSize + 1 : 0

    Loader {
        id: footerLoader

        anchors.fill: parent
        active: AudioManager.entry && Kirigami.Settings.isMobile
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
        active: Kirigami.Settings.isMobile && !WindowUtils.isWidescreen
        sourceComponent: BottomToolbar {
            opacity: (!footerLoader.item || footerLoader.item.contentY === 0) ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.shortDuration
                }
            }
        }
    }

    // Notification that shows the progress of feed updates
    // It mimicks the behaviour of an InlineMessage, because InlineMessage does
    // not allow to add a BusyIndicator
    UpdateNotification {
        id: updateNotification
        text: i18ncp("Number of Updated Podcasts", "Updated %2 of %1 Podcast", "Updated %2 of %1 Podcasts", Fetcher.updateTotal, Fetcher.updateProgress)

        showAbortButton: true

        function abortAction(): void {
            Fetcher.cancelFetching();
        }

        Connections {
            target: Fetcher
            function onUpdatingChanged(): void {
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
        text: i18ncp("Number of Moved Files", "Moved %2 of %1 File", "Moved %2 of %1 Files", StorageManager.storageMoveTotal, StorageManager.storageMoveProgress)
        showAbortButton: true

        function abortAction(): void {
            StorageManager.cancelStorageMove();
        }

        Connections {
            target: StorageManager
            function onStorageMoveStarted(): void {
                moveStorageNotification.open();
            }
            function onStorageMoveFinished(): void {
                moveStorageNotification.close();
            }
        }
    }

    // Notification that shows the progress of feed and episode syncing
    UpdateNotification {
        id: updateSyncNotification
        text: Sync.syncProgressText
        showAbortButton: true

        function abortAction(): void {
            Sync.abortSync();
        }

        Connections {
            target: Sync
            function onSyncProgressChanged(): void {
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

    // Overlay with options what to do when metered downloads are not allowed
    ConnectionCheckAction {
        id: downloadOverlay

        headingText: i18nc("@info:status", "Podcast downloads are currently not allowed on metered connections")
        condition: NetworkConnectionManager.episodeDownloadsAllowed
        property var entry: undefined
        property var selection: undefined

        function action(): void {
            if (selection) {
                DataManager.bulkDownloadEnclosuresByIndex(selection);
            } else if (entry) {
                entry.queueStatus = true;
                entry.enclosure.download();
            }
            selection = undefined;
            entry = undefined;
        }

        function allowOnceAction(): void {
            SettingsManager.allowMeteredEpisodeDownloads = true;
            action();
            SettingsManager.allowMeteredEpisodeDownloads = false;
        }

        function alwaysAllowAction(): void {
            SettingsManager.allowMeteredEpisodeDownloads = true;
            SettingsManager.save();
            action();
        }
    }

    Connections {
        target: Sync
        function onPasswordInputRequired(): void {
            syncPasswordOverlay.open();
        }
    }

    SyncPasswordOverlay {
        id: syncPasswordOverlay
    }

    //Global Shortcuts
    Shortcut {
        sequence: "space"
        enabled: AudioManager.canPlay
        onActivated: AudioManager.playPause()
    }
    Shortcut {
        sequence: "n"
        enabled: AudioManager.canGoNext
        onActivated: AudioManager.next()
    }

    // Systray implementation
    Connections {
        target: kastsMainWindow

        function onClosing(close: CloseEvent): void {
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

        function onRaiseWindow(): void {
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
