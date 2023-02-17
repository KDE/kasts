/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Controls.Control {
    id: searchFilterBar

    required property var proxyModel
    required property var parentKey

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

    contentItem: RowLayout {
        Kirigami.SearchField {
            Layout.fillWidth: true
            id: searchField
            placeholderText: i18nc("@label:textbox Placeholder text for episode search field", "Search Episodes")
            focus: true
            autoAccept: false
            onAccepted: {
                proxyModel.searchFilter = searchField.text;
            }

            Kirigami.Action {
                id: searchSettingsButton
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
            Component.onCompleted: {
                // rightActions are defined from right-to-left
                // if we want to insert the settings action as the rightmost, then it
                // must be defined as first action, which means that we need to save the
                // default clear action and push that as a second action
                var origAction = searchField.rightActions[0];
                searchField.rightActions[0] = searchSettingsButton;
                searchField.rightActions.push(origAction);
            }

            Keys.onEscapePressed: {
                proxyModel.filterType = AbstractEpisodeProxyModel.NoFilter;
                proxyModel.searchFilter = "";
                parentKey.checked = false;
                event.accepted = true;
            }
            Keys.onReturnPressed: {
                accepted();
                event.accepted = true;
            }
        }

        Controls.ToolButton {
            id: filterButton
            icon.name: "view-filter"
            text: i18nc("@action:intoolbar Button to open menu to filter episodes based on their status (played, new, etc.)", "Filter")
            display: wideScreen ? Controls.AbstractButton.TextBesideIcon : Controls.AbstractButton.IconOnly

            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            Controls.ToolTip.text: i18nc("@info:tooltip", "Filter Episodes by Status")

            onPressed: {
                if (filterMenu.visible) {
                    filterMenu.dismiss();
                } else {
                    filterMenu.popup(filterButton, filterButton.x, filterButton.y + filterButton.height);
                }
            }
        }
    }

    Component.onCompleted: {
        searchField.forceActiveFocus();
    }

    ListModel {
        id: filterModel

        // have to use script because i18n doesn't work within ListElement
        Component.onCompleted: {
            var filterList = [AbstractEpisodeProxyModel.NoFilter,
                              AbstractEpisodeProxyModel.ReadFilter,
                              AbstractEpisodeProxyModel.NotReadFilter,
                              AbstractEpisodeProxyModel.NewFilter,
                              AbstractEpisodeProxyModel.NotNewFilter]
            for (var i in filterList) {
                filterModel.append({"name": proxyModel.getFilterName(filterList[i]),
                                    "filterType": filterList[i]});
            }
        }
    }

    Controls.Menu {
        id: filterMenu

        title: i18nc("@title:menu", "Select Filter")

        Controls.ButtonGroup { id: filterGroup }

        Repeater {
            model: filterModel

            Controls.RadioButton {
                padding: Kirigami.Units.smallSpacing
                text: model.name
                checked: model.filterType === proxyModel.filterType
                Controls.ButtonGroup.group: filterGroup

                onToggled: {
                    if (checked) {
                        proxyModel.filterType = model.filterType;
                    }
                    filterMenu.dismiss();
                }
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
