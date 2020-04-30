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
    id: page

    property var name
    property var url
    property var image

    property var all: page.url === "all"

    contextualActions: [
        Kirigami.Action {
            text: i18n("Details")
            visible: !all
            onTriggered: pageStack.push("qrc:/FeedDetailsPage.qml")
        }
    ]

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh Feed")
        onTriggered: entryListModel.fetch()
    }

    Component.onCompleted: {
        entryListModel.fetch();
    }

    Kirigami.PlaceholderMessage {
        visible: entryList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No Entries available.")
    }

    ListView {
        id: entryList
        visible: count !== 0
        model: EntryListModel {
            id: entryListModel
            feed: page.url
        }

        header: RowLayout {
            width: parent.width
            height: root.height * 0.2
            visible: !all
            Kirigami.Icon {
                source: entryListModel.image(page.image)
                width: height
                height: parent.height
                Component.onCompleted: console.log("Height: " + page.height)
            }
            Kirigami.Heading {
                text: page.name
            }
        }

        delegate: Kirigami.SwipeListItem {
            Controls.Label {
                text: model.title + " - " + model.updated.toLocaleString(Qt.locale(), Locale.ShortFormat)
                textFormat: Text.RichText
                color: model.read ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                wrapMode: Text.Wrap
            }

            actions: [
                Kirigami.Action {
                    icon.name: model.bookmark ? "bookmark-remove" : "bookmark-new"
                    text: i18n("Bookmark")
                    onTriggered: {
                        model.bookmark = !model.bookmark
                    }
                }
            ]

            onClicked: {
                model.read = true;
                pageStack.push("qrc:/EntryPage.qml", {"data": model, "baseUrl": entryListModel.baseUrl(model.link)})
            }
        }
    }
}
