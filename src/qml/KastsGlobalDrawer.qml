// SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.OverlayDrawer {
    id: root
    modal: false
    closePolicy: QQC2.Popup.NoAutoClose
    edge: Qt.application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge

    readonly property real pinnedWidth: Kirigami.Units.gridUnit * 3
    readonly property real widescreenBigWidth: Kirigami.Units.gridUnit * 10
    readonly property int buttonDisplayMode: kastsMainWindow.isWidescreen ? Kirigami.NavigationTabButton.TextBesideIcon : Kirigami.NavigationTabButton.IconOnly

    width: showGlobalDrawer ? (kastsMainWindow.isWidescreen ? widescreenBigWidth : pinnedWidth) : 0

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    contentItem: Loader {
        id: sidebarColumn
        active: showGlobalDrawer

        sourceComponent: ColumnLayout {
            spacing: 0

            QQC2.ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: pageStack.globalToolBar.preferredHeight

                leftPadding: Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.smallSpacing

                contentItem: GlobalSearchField {}
            }

            QQC2.ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true

                QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOff
                QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
                contentWidth: -1 // disable horizontal scroll

                ColumnLayout {
                    id: column
                    width: scrollView.width
                    spacing: 0

                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")
                        icon.name: "source-playlist"
                        checked: currentPage == "QueuePage"
                        onClicked: {
                            pushPage("QueuePage")
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: i18nc("@title of page allowing to search for new podcasts online", "Discover")
                        icon.name: "search"
                        checked: currentPage == "DiscoverPage"
                        onClicked: {
                            pushPage("DiscoverPage")
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: i18nc("@title of page with list of podcast subscriptions", "Subscriptions")
                        icon.name: "bookmarks"
                        checked: currentPage == "FeedListPage"
                        onClicked: {
                            pushPage("FeedListPage")
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: i18nc("@title of page with list of podcast episodes", "Episodes")
                        icon.name: "rss"
                        checked: currentPage == "EpisodeListPage"
                        onClicked: {
                            pushPage("EpisodeListPage")
                        }
                    }
                    Kirigami.NavigationTabButton {
                        Layout.fillWidth: true
                        display: root.buttonDisplayMode
                        text: i18nc("@title of page with list of downloaded episodes", "Downloads")
                        icon.name: "download"
                        checked: currentPage == "DownloadListPage"
                        onClicked: {
                            pushPage("DownloadListPage")
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

                text: i18nc("@title of dialog with app settings", "Settings")
                icon.name: "settings-configure"
                checked: currentPage == "SettingsPage"
                onClicked: {
                    checked = false;
                    pushPage("SettingsPage")
                }
            }
        }
    }
}
