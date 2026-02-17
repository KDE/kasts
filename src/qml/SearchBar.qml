/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Controls.Control {
    id: root

    required property var proxyModel
    required property var parentKey
    property bool showSearchFilters: true

    leftPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    background: Rectangle {
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        color: Kirigami.Theme.backgroundColor

        Kirigami.Separator {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }

    contentItem: Kirigami.SearchField {
        id: searchField
        Layout.fillWidth: true
        placeholderText: KI18n.i18nc("@label:textbox Placeholder text for episode search field", "Search episodes…")
        text: root.proxyModel.searchFilter
        focus: true
        autoAccept: false
        onAccepted: {
            root.proxyModel.searchFilter = searchField.text;
        }

        Kirigami.Action {
            id: searchSettingsButton
            visible: root.showSearchFilters
            enabled: visible
            icon.name: "settings-configure"
            text: KI18n.i18nc("@action:intoolbar", "Advanced Search Options")

            onTriggered: {
                if (searchSettingsMenu.visible) {
                    searchSettingsMenu.dismiss();
                } else {
                    searchSettingsMenu.popup(searchSettingsButton);
                }
            }
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

        Keys.onEscapePressed: event => {
            root.proxyModel.searchFilter = "";
            parentKey.checked = false;
            event.accepted = true;
        }
        Keys.onReturnPressed: event => {
            accepted();
            event.accepted = true;
        }
    }

    Component.onCompleted: {
        searchField.forceActiveFocus();
    }

    ListModel {
        id: searchSettingsModel

        function reload(): void {
            clear();
            if (showSearchFilters) {
                var searchList = [AbstractEpisodeProxyModel.TitleFlag, AbstractEpisodeProxyModel.ContentFlag, AbstractEpisodeProxyModel.FeedNameFlag];
                for (var i in searchList) {
                    searchSettingsModel.append({
                        name: proxyModel.getSearchFlagName(searchList[i]),
                        searchFlag: searchList[i],
                        isChecked: proxyModel.searchFlags & searchList[i]
                    });
                }
            }
        }

        Component.onCompleted: {
            reload();
        }
    }

    Controls.Menu {
        id: searchSettingsMenu

        title: KI18n.i18nc("@title:menu", "Search Preferences")

        Controls.Label {
            padding: Kirigami.Units.smallSpacing
            text: KI18n.i18nc("@title:group Group of fields in which can be searched", "Search in:")
        }

        Repeater {
            model: searchSettingsModel

            delegate: Controls.MenuItem {
                required property string name
                required property var searchFlag
                required property bool isChecked

                text: name
                checkable: true
                checked: isChecked
                onTriggered: {
                    if (checked) {
                        root.proxyModel.searchFlags = root.proxyModel.searchFlags | searchFlag;
                    } else {
                        root.proxyModel.searchFlags = root.proxyModel.searchFlags & ~searchFlag;
                    }
                }
            }
        }

        onOpened: {
            searchSettingsModel.reload();
        }
    }
}
