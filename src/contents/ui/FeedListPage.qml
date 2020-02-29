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

import QtQuick 2.13
import QtQuick.Controls 2.10 as Controls
import QtQuick.Layouts 1.12

import org.kde.kirigami 2.4 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
        title: "Alligator"

        contextualActions: [
                Kirigami.Action {
                    text: "Add feed"
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
                       text: "https://rss.golem.de/rss.php?feed=RSS2.0"
                       Kirigami.FormData.label: "Url"
                   }
                   Controls.Button {
                       text: "Add"
                       Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                       enabled: urlField.text
                       onClicked: {
                            feedListModel.add_feed(urlField.text)
                            addSheet.close()
                       }
                   }
               }
        }

        ListView {
            anchors.fill: parent
            model: FeedListModel {
                id: feedListModel
            }

            delegate: Kirigami.SwipeListItem {
                Controls.Label {
                    text: model.display
                }

                onTextChanged: console.log(model.display)

                width: parent.width
                height: Kirigami.Units.gridUnit * 2
                actions: [
                    Kirigami.Action {
                        icon.name: "list-remove"
                        text: "Remove"
                        onTriggered: feedListModel.remove_feed(index)
                    },
                    Kirigami.Action {
                        icon.name: "document-edit"
                        text: "Edit"
                        onTriggered:; //TODO
                    }
                ]
                onClicked: {
                    pageStack.push("qrc:/EntryListPage.qml", {"name": model.display, "url": model.url})
                }
            }
        }
    }
