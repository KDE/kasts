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
import QtQuick.Controls 2.10 as Controls
import QtQuick.Layouts 1.12

import org.kde.kirigami 2.8 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
        title: "Alligator"

        contextualActions: [
                Kirigami.Action {
                    text: i18n("Add feed")
                    onTriggered: {
                        addSheet.open()
                    }
                }
            ]

        Kirigami.OverlaySheet {
               id: addSheet
               contentItem: Kirigami.FormLayout {
                   Controls.TextField {
                       id: urlField
                       Layout.fillWidth: true
                       //placeholderText: "https://example.org/feed.xml"
                       text: "https://planet.kde.org/global/atom.xml/"
                       Kirigami.FormData.label: "Url"
                   }
                   Controls.Button {
                       text: i18n("Add")
                       Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                       enabled: urlField.text
                       onClicked: {
                            feedListModel.addFeed(urlField.text)
                            addSheet.close()
                       }
                   }
               }
        }

        ListView {
            id: feedList
            anchors.fill: parent
            model: FeedListModel {
                id: feedListModel
            }

            header:
                Kirigami.AbstractListItem {
                    Controls.Label {
                        text: i18n("All feeds")
                    }

                    width: parent.width;
                    height: Kirigami.Units.gridUnit * 2
                    onClicked: {
                        feedList.focus = false
                        pageStack.push("qrc:/EntryListPage.qml", {"name": "All feeds", "url": "all"})
                    }
                }

            delegate: Kirigami.AbstractListItem {
                height: Kirigami.Units.gridUnit*2

                Item {
                    height: parent.height

                    Kirigami.Icon {
                        id: icon
                        source: model.feed.image
                        width: height
                        height: parent.height
                    }

                    Controls.Label {
                        text: model.feed.name
                        height: parent.height
                        leftPadding: Kirigami.Units.gridUnit*0.5
                        anchors.left: icon.right
                    }
                    MouseArea {
                        x: 0
                        y: 0
                        width: parent.width
                        height: parent.height
                        onClicked: pageStack.push("qrc:/EntryListPage.qml", {feed: model.feed})
                    }
                }
            }
        }
    }
