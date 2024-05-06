/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as AddonDelegates
import org.kde.kirigami.delegates as Delegates

import org.kde.kasts

Kirigami.SearchField {
    id: globalSearchField

    autoAccept: false

    property string searchFilter: ""

    function openEntry(entry) {
        pushPage("EpisodeListPage");
        pageStack.push("qrc:/qt/qml/org/kde/kasts/qml/EntryPage.qml", {"entry": entry});

        // Find the index of the entry on the EpisodeListPage and scroll to it
        var episodeModel = pageStack.get(0).episodeList.model
        for (var i = 0; i <  episodeModel.rowCount(); i++) {
            var index = episodeModel.index(i, 0);
            if (entry.id == episodeModel.data(index, AbstractEpisodeModel.IdRole)) {
                pageStack.get(0).episodeList.currentIndex = i;
                pageStack.get(0).episodeList.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                pageStack.get(0).episodeList.positionViewAtIndex(i, ListView.Center);
            }
        }
    }

    Component.onCompleted: {
        // rightActions are defined from right-to-left
        // if we want to insert the settings action as the rightmost, then it
        // must be defined as first action, which means that we need to save the
        // default clear action and push that as a second action
        var origAction = searchDialog.searchFieldRightActions[0];
        searchDialog.searchFieldRightActions[0] = searchSettingsButton;
        searchDialog.searchFieldRightActions.push(origAction);
    }

    TapHandler {
        onTapped: {
            searchDialog.open();
            searchDialog.forceActiveFocus();
        }
        acceptedButtons: Qt.RightButton | Qt.LeftButton
    }
    Keys.onPressed: (event) => {
        if (event.key !== Qt.Key_Tab || event.key !== Qt.Key_Backtab) {
            searchDialog.open();
            searchDialog.text = text;
            searchDialog.forceActiveFocus();
        }
    }
    Keys.priority: Keys.AfterItem

    Kirigami.SearchDialog {
        id: searchDialog

        parent: Controls.Overlay.overlay

        EpisodeProxyModel {
            id: proxyModel
            searchFilter: globalSearchField.searchFilter
        }

        onAccepted: {
            globalSearchField.searchFilter = searchDialog.text;
        }

        onTextChanged: {
            if (searchDialog.text === "") {
                globalSearchField.searchFilter = "";
            }
        }

        model: globalSearchField.searchFilter === "" ? null : proxyModel

        delegate: AddonDelegates.RoundedItemDelegate {
            id: albumDelegate

            required property int index
            required property var entry

            contentItem: Delegates.IconTitleSubtitle {
                icon.source: albumDelegate.entry.cachedImage
                title: albumDelegate.entry.title
                subtitle: albumDelegate.entry.feed.name
            }
            onClicked: {
                globalSearchField.openEntry(albumDelegate.entry);
                searchDialog.close();
            }
        }

        emptyText: i18nc("@info Placeholder text in search box", "No search results")

        Kirigami.Action {
            id: searchSettingsButton
            visible: searchDialog.visible
            icon.name: "settings-configure"
            text: i18nc("@action:intoolbar", "Advanced Search Options")

            onTriggered: {
                if (searchSettingsMenu.visible) {
                    searchSettingsMenu.dismiss();
                } else {
                    searchSettingsMenu.popup(searchSettingsButton);
                }
            }
        }

        ListModel {
            id: searchSettingsModel

            function reload() {
                clear();
                var searchList = [AbstractEpisodeProxyModel.TitleFlag,
                                AbstractEpisodeProxyModel.ContentFlag,
                                AbstractEpisodeProxyModel.FeedNameFlag]
                for (var i in searchList) {
                    searchSettingsModel.append({"name": proxyModel.getSearchFlagName(searchList[i]),
                                                "searchFlag": searchList[i],
                                                "checked": proxyModel.searchFlags & searchList[i]});
                }
            }

            Component.onCompleted: {
                reload();
            }
        }

        Controls.Menu {
            id: searchSettingsMenu

            title: i18nc("@title:menu", "Search Preferences")

            Controls.Label {
                padding: Kirigami.Units.smallSpacing
                text: i18nc("@title:group Group of fields in which can be searched", "Search in:")
            }

            Repeater {
                model: searchSettingsModel

                Controls.MenuItem {
                    text: model.name
                    checkable: true
                    checked: model.checked
                    onTriggered: {
                        if (checked) {
                            proxyModel.searchFlags = proxyModel.searchFlags | model.searchFlag;
                        } else {
                            proxyModel.searchFlags = proxyModel.searchFlags & ~model.searchFlag;
                        }
                    }
                }
            }

            onOpened: {
                searchSettingsModel.reload();
            }
        }
    }
}
