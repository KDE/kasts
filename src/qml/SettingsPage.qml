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

import org.kde.kirigami 2.8 as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Settings")

    property QtObject settings


    Kirigami.FormLayout {
        Controls.TextField {
            id: deleteAfterCount
            text: settings.deleteAfterCount
            Kirigami.FormData.label: i18n("Delete posts after:")
        }
        Controls.ComboBox {
            id: deleteAfterType
            currentIndex: settings.deleteAfterType
            model: [i18n("Posts"), i18n("Days"), i18n("Weeks"), i18n("Months")]
        }
        Controls.Button {
            text: i18n("Save")
            onClicked: {
                settings.deleteAfterCount = deleteAfterCount.text
                settings.deleteAfterType = deleteAfterType.currentIndex
                settings.save()
                pageStack.pop()
            }
        }
    }
}
