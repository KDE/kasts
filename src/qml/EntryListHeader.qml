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

RowLayout {
    width: parent.width
    height: root.height * 0.2
    Item {
        id: icon
        width: height
        height: parent.height
        Kirigami.Icon {
            source: Fetcher.image(page.feed.image)
            width: height
            height: parent.height
            visible: !busy.visible
        }
        Controls.BusyIndicator {
            id: busy
            width: height
            height: parent.height
            visible: page.feed.refreshing
        }
    }
    ColumnLayout {
        Kirigami.Heading {
            text: page.feed.name
        }
        Controls.Label {
            text: page.feed.description
        }
        Controls.Label {
            text: page.feed.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + page.feed.authors[0].name
        }
    }
}