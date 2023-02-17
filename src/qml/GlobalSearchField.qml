/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14
import QtQml.Models 2.15

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.components 1.0 as Addons

import org.kde.kasts 1.0

Addons.SearchPopupField {
    id: globalSearchField
    spaceAvailableLeft: false
    spaceAvailableRight: true

    autoAccept: false

    popup.width: Math.min(Kirigami.Units.gridUnit * 20, kastsMainWindow.width - Kirigami.Units.gridUnit * 2)

    property string searchFilter: ""

    onAccepted: {
        globalSearchField.searchFilter = globalSearchField.text
    }

    popup.onClosed: {
        globalSearchField.text = ""
    }

    onTextChanged: {
        if (globalSearchField.text === "") {
            globalSearchField.searchFilter = "";
        }
    }

    function openEntry(entry) {
        pushPage("EpisodeListPage");
        pageStack.push("qrc:/EntryPage.qml", {"entry": entry});

        /* let's not select the current item for now, since it can take a long
         * time and will not automatically scroll to that item either
        var episodeModel = pageStack.get(0).episodeList.model
        for (var i = 0; i <  episodeModel.rowCount(); i++) {
            var index = episodeModel.index(i, 0);
            if (entry == episodeModel.data(index, AbstractEpisodeModel.EntryRole)) {
                pageStack.get(0).episodeList.currentIndex = i;
                pageStack.get(0).episodeList.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
            }
        }
        */
    }

    Component.onCompleted: {
        // rightActions are defined from right-to-left
        // if we want to insert the settings action as the rightmost, then it
        // must be defined as first action, which means that we need to save the
        // default clear action and push that as a second action
        var origAction = searchField.rightActions[0];
        searchField.rightActions[0] = searchSettingsButton;
        searchField.rightActions.push(origAction);
    }

    ListView {
        id: searchListView
        reuseItems: true

        EpisodeProxyModel {
            id: proxyModel
            searchFilter: globalSearchField.searchFilter
        }

        model: globalSearchField.searchFilter === "" ? null : proxyModel

        delegate: Component {
            Kirigami.BasicListItem {
                separatorVisible: true
                icon: model.entry.cachedImage
                label: model.entry.title
                subtitle: model.entry.feed.name
                onClicked: {
                    globalSearchField.openEntry(model.entry);
                    globalSearchField.popup.close();
                }
            }
        }
        Kirigami.PlaceholderMessage {
            id: loadingPlaceholder
            anchors.fill: parent
            visible: searchListView.count === 0

            text: i18nc("@info Placeholder text in search box", "No Search Results")
        }

        Kirigami.Action {
            id: searchSettingsButton
            visible: globalSearchField.popup.visible
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

                Controls.CheckBox {
                    padding: Kirigami.Units.smallSpacing
                    text: model.name
                    checked: model.checked
                    onToggled: {
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
