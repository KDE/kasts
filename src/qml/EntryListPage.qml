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

import org.kde.kirigami 2.8 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page

    property var data

    property var all: page.data.url === "all"

    contextualActions: [
        Kirigami.Action {
            text: i18n("Details")
            visible: !all
            onTriggered: ;//pageStack.push("qrc:/qml/FeedDetailsPage.qml", {"modelData": atomModel})
        }
    ]

    Component.onCompleted: {
        entryListModel.fetch();
    }

    ListView {
        model: EntryListModel {
            id: entryListModel
            feed: page.data.url
        }

        header: RowLayout {
            width: parent.width
            height: page.height * 0.2
            visible: !all
            Image {
                source: page.data.image
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 0
                sourceSize.height: parent.height
            }
            Kirigami.Heading {
                text: page.data.name
            }
        }

        delegate: Kirigami.SwipeListItem {
            Controls.Label {
                width: parent.width
                text: model.title + " - " + model.updated.toLocaleString(Qt.locale(), Locale.ShortFormat)
                textFormat: Text.RichText
                color: model.read ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
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
                pageStack.push("qrc:/EntryPage.qml", {"data": model})
            }
        }
    }
}
