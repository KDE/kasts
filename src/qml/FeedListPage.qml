/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import Qt.labs.platform
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.ScrollablePage {
    id: root
    title: KI18n.i18nc("@title of page with list of podcast subscriptions", "Subscriptions")

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    anchors.margins: 0
    padding: 0

    property string lastFeed: ""

    supportsRefreshing: true
    onRefreshingChanged: {
        if (refreshing) {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            visible: Kirigami.Settings.isMobile
            text: KI18n.i18nc("@title of page allowing to search for new podcasts online", "Discover")
            icon.name: "search"
            onTriggered: {
                ((root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack as Kirigami.PageRow).push(Qt.createComponent("org.kde.kasts", "DiscoverPage"));
            }
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:intoolbar", "Refresh All Podcasts")
            icon.name: "view-refresh"
            onTriggered: root.refreshing = true
        },
        Kirigami.Action {
            id: addAction
            text: KI18n.i18nc("@action:intoolbar", "Add Podcast…")
            icon.name: "list-add"
            onTriggered: {
                addSheet.open();
            }
        },
        Kirigami.Action {
            id: sortActionRoot
            icon.name: "view-sort"
            text: KI18n.i18nc("@action:intoolbar Open menu with options to sort subscriptions", "Sort")

            tooltip: KI18n.i18nc("@info:tooltip", "Select how to sort subscriptions")

            property Controls.ActionGroup sortGroup: Controls.ActionGroup {}

            property Instantiator repeater: Instantiator {
                model: ListModel {
                    id: sortModel
                    // have to use script because KI18n.i18n doesn't work within ListElement
                    Component.onCompleted: {
                        if (sortActionRoot.visible) {
                            var sortList = [FeedsProxyModel.UnreadDescending, FeedsProxyModel.UnreadAscending, FeedsProxyModel.NewDescending, FeedsProxyModel.NewAscending, FeedsProxyModel.FavoriteDescending, FeedsProxyModel.FavoriteAscending, FeedsProxyModel.TitleAscending, FeedsProxyModel.TitleDescending];
                            for (var i in sortList) {
                                sortModel.append({
                                    name: gridView.model.getSortName(sortList[i]),
                                    iconName: gridView.model.getSortIconName(sortList[i]),
                                    sortType: sortList[i]
                                });
                            }
                        }
                    }
                }

                Kirigami.Action {
                    required property string iconName
                    required property string name
                    required property int sortType

                    visible: sortActionRoot.visible
                    icon.name: iconName
                    text: name
                    checkable: true
                    checked: root.Controls.ApplicationWindow.window ? (root.Controls.ApplicationWindow.window as Main).feedSorting === sortType : false
                    Controls.ActionGroup.group: sortActionRoot.sortGroup

                    onTriggered: {
                        (root.Controls.ApplicationWindow.window as Main).feedSorting = sortType;
                    }
                }

                onObjectAdded: (index, object) => {
                    sortActionRoot.children.push(object);
                }
            }
        },
        Kirigami.Action {
            id: searchActionButton
            icon.name: "search"
            text: KI18n.i18nc("@action:intoolbar", "Search")
            checkable: true
        },
        Kirigami.Action {
            id: importAction
            text: KI18n.i18nc("@action:intoolbar", "Import Podcasts…")
            icon.name: "document-import"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: importDialog.open()
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action:intoolbar", "Export Podcasts…")
            icon.name: "document-export"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: exportDialog.open()
        }
    ]

    // add the default actions through onCompleted to add them to the ones
    // defined above
    Component.onCompleted: {
        for (var i in gridView.contextualActionList) {
            pageActions.push(gridView.contextualActionList[i]);
        }
    }

    actions: pageActions

    header: Loader {
        anchors.right: parent.right
        anchors.left: parent.left

        active: searchActionButton.checked
        visible: active
        sourceComponent: SearchBar {
            proxyModel: gridView.model
            parentKey: searchActionButton
            showSearchFilters: false
        }
    }

    AddFeedSheet {
        id: addSheet
    }

    FileDialog {
        id: importDialog
        title: KI18n.i18nc("@title:window", "Import Podcasts")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [KI18n.i18nc("@label:listbox File filter option in file dialog", "OPML Files (*.opml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "XML Files (*.xml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "All Files (*)")]
        onAccepted: DataManager.importFeeds(file)
    }

    FileDialog {
        id: exportDialog
        title: KI18n.i18nc("@title:window", "Export Podcasts")
        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [KI18n.i18nc("@label:listbox File filter option in file dialog", "OPML Files (*.opml)"), KI18n.i18nc("@label:listbox File filter option in file dialog", "All Files (*)")]
        onAccepted: DataManager.exportFeeds(file)
        fileMode: FileDialog.SaveFile
    }

    ConnectionCheckAction {
        id: updateAllFeeds
    }

    FeedListGridView {
        id: gridView
        addAction: addAction
        importAction: importAction
    }
}
