// SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings
import org.kde.ki18n

import org.kde.kasts

Kirigami.OverlayDrawer {
    id: root
    modal: false
    closePolicy: Controls.Popup.NoAutoClose
    edge: Application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge

    readonly property real pinnedWidth: Kirigami.Units.gridUnit * 3
    readonly property real widescreenBigWidth: Kirigami.Units.gridUnit * 10
    readonly property int buttonDisplayMode: Utils.isWidescreen ? Kirigami.NavigationTabButton.TextBesideIcon : Kirigami.NavigationTabButton.IconOnly

    // Keep track of the settings page being opened on the layer stack for mobile
    readonly property bool settingsOpened: Kirigami.Settings.isMobile && (Controls.ApplicationWindow.window as Main).pageStack.layers.depth >= 2 && ((Controls.ApplicationWindow.window as Main).pageStack.layers.currentItem as KirigamiSettings.ConfigurationView).title === "Settings"

    width: (Controls.ApplicationWindow.window as Main).showGlobalDrawer ? (Utils.isWidescreen ? widescreenBigWidth : pinnedWidth) : 0

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    readonly property var rootSafeMargins: Controls.ApplicationWindow.window.SafeArea.margins
    leftPadding: edge === Qt.LeftEdge ? rootSafeMargins.left : 0
    rightPadding: edge === Qt.RightEdge ? rootSafeMargins.right : 0
    topPadding: rootSafeMargins.top
    bottomPadding: rootSafeMargins.bottom

    contentItem: Loader {
        id: sidebarColumn
        active: (root.Controls.ApplicationWindow.window as Main).showGlobalDrawer

        sourceComponent: ColumnLayout {
            spacing: 0

            Controls.ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: (root.Controls.ApplicationWindow.window as Main).pageStack.globalToolBar.preferredHeight

                leftPadding: Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.smallSpacing

                contentItem: GlobalSearchField {}
            }

            Controls.ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true

                Controls.ScrollBar.vertical.policy: Controls.ScrollBar.AlwaysOff
                Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
                contentWidth: -1 // disable horizontal scroll

                ColumnLayout {
                    id: column
                    width: scrollView.width
                    spacing: 0

                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: KI18n.i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")
                        icon.name: "source-playlist"
                        checked: (root.Controls.ApplicationWindow.window as Main).currentPage == "QueuePage" && !root.settingsOpened
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).pushPage("QueuePage");
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: KI18n.i18nc("@title of page allowing to search for new podcasts online", "Discover")
                        icon.name: "search"
                        checked: (root.Controls.ApplicationWindow.window as Main).currentPage == "DiscoverPage" && !root.settingsOpened
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).pushPage("DiscoverPage");
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: KI18n.i18nc("@title of page with list of podcast subscriptions", "Subscriptions")
                        icon.name: "bookmarks"
                        checked: (root.Controls.ApplicationWindow.window as Main).currentPage == "FeedListPage" && !root.settingsOpened
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).pushPage("FeedListPage");
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: KI18n.i18nc("@title of page with list of podcast episodes", "Episodes")
                        icon.name: "rss"
                        checked: (root.Controls.ApplicationWindow.window as Main).currentPage == "EpisodeListPage" && !root.settingsOpened
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).pushPage("EpisodeListPage");
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: KI18n.i18nc("@title of page with list of downloaded episodes", "Downloads")
                        icon.name: "download"
                        checked: (root.Controls.ApplicationWindow.window as Main).currentPage == "DownloadListPage" && !root.settingsOpened
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).pushPage("DownloadListPage");
                        }
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
                Layout.rightMargin: Kirigami.Units.smallSpacing
                Layout.leftMargin: Kirigami.Units.smallSpacing
            }

            Kirigami.NavigationTabButton {
                Layout.fillWidth: true
                display: root.buttonDisplayMode
                text: KI18n.i18nc("@title of dialog with app settings", "Settings")
                icon.name: "settings-configure"
                checked: root.settingsOpened
                onClicked: {
                    if (!Kirigami.Settings.isMobile)
                        checked = false;
                    (root.Controls.ApplicationWindow.window as Main).pushPage("SettingsView");
                }
            }
        }
    }
}
