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

    property var feed

    title: feed.title

    //property bool all: feed

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
        onTriggered: Fetcher.fetch(page.feed.url)
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
            feed: page.feed
        }

        header: RowLayout {
            width: parent.width
            height: root.height * 0.2
            visible: !all
            Kirigami.Icon {
                source: Fetcher.image(page.feed.image)
                width: height
                height: parent.height
            }
            Kirigami.Heading {
                text: page.feed.name
            }
        }

        delegate: Kirigami.SwipeListItem {
            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                Controls.Label {
                    text: model.entry.title
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    opacity: 1
                }
                Controls.Label {
                    id: subtitleItem
                    text: model.entry.updated.toLocaleString(Qt.locale(), Locale.ShortFormat) + (model.entry.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + model.entry.authors[0].name)
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    opacity: 0.6
                    visible: text.length > 0
                }
            }

            onClicked: {
                pageStack.push("qrc:/EntryPage.qml", {"entry": model.entry})
            }
        }
    }
}
