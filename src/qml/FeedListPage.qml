/**
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    title: "Alligator"

    property var lastFeed: ""

    supportsRefreshing: true
    onRefreshingChanged:
        if(refreshing)  {
            Fetcher.fetchAll()
            refreshing = false
        }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Refresh all feeds")
            iconName: "view-refresh"
            onTriggered: refreshing = true
            visible: !Kirigami.Settings.isMobile
        }
    ]

    actions.main: Kirigami.Action {
                text: i18n("Add feed")
                iconName: "list-add"
                onTriggered: {
                    addSheet.open()
                }
            }

    Kirigami.OverlaySheet {
        id: addSheet
        parent: applicationWindow().overlay
        header: Kirigami.Heading {
            text: i18n("Add new Feed")
        }

        contentItem: ColumnLayout {
            Controls.Label {
                text: i18n("Url:")
            }
            Controls.TextField {
                id: urlField
                Layout.fillWidth: true
                text: "https://planet.kde.org/global/atom.xml/"
            }
        }

        footer: Controls.Button {
            text: i18n("Add Feed")
            enabled: urlField.text
            onClicked: {
                Database.addFeed(urlField.text)
                addSheet.close()
            }
        }
    }

    Kirigami.PlaceholderMessage {
        visible: feedList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Feeds added yet.")
    }

    ListView {
        id: feedList
        visible: count !== 0
        anchors.fill: parent
        model: FeedListModel {
            id: feedListModel
        }

        delegate: Kirigami.SwipeListItem {
            height: Kirigami.Units.gridUnit*2

            Item {
                Item {
                    id: icon
                    width: height
                    height: parent.height
                    Kirigami.Icon {
                        source: Fetcher.image(model.feed.image)
                        width: height
                        height: parent.height
                        visible: !busy.visible
                    }
                    Controls.BusyIndicator {
                        id: busy
                        width: height
                        height: parent.height
                        visible: model.feed.refreshing
                    }
                }
                Controls.Label {
                    text: model.feed.name
                    height: parent.height
                    anchors.left: icon.right
                    leftPadding: 0.5*Kirigami.Units.gridUnit
                }
            }

            actions: [
                Kirigami.Action {
                    icon.name: "delete"
                    onTriggered: {
                        if(pageStack.depth > 1 && model.feed.url === lastFeed)
                            pageStack.pop()
                        feedListModel.removeFeed(index)
                    }
                }

            ]

            onClicked: {
                lastFeed = model.feed.url
                pageStack.push("qrc:/EntryListPage.qml", {"feed": model.feed})
            }
        }
    }
}
