/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Settings")

    property QtObject settings


    Kirigami.FormLayout {

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Article List")
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Delete after:")

            Controls.SpinBox {
                id: deleteAfterCount
                value: settings.deleteAfterCount
                enabled: deleteAfterType.currentIndex !== 0
            }

            Controls.ComboBox {
                id: deleteAfterType
                currentIndex: settings.deleteAfterType
                model: [i18n("Never"), i18n("Articles"), i18n("Days"), i18n("Weeks"), i18n("Months")]
            }
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Article")
        }

        Controls.SpinBox {
            id: articleFontSizeSpinBox

            enabled: !useSystemFontCheckBox.checked
            value: settings.articleFontSize
            Kirigami.FormData.label: i18n("Font size:")
            from: 6
            to: 20
        }

        Controls.CheckBox {
            id: useSystemFontCheckBox
            checked: settings.articleFontUseSystem
            text: i18n("Use system default")
        }

        Controls.Button {
            text: i18n("Save")
            onClicked: {
                settings.deleteAfterCount = deleteAfterCount.value
                settings.deleteAfterType = deleteAfterType.currentIndex
                settings.articleFontSize = articleFontSizeSpinBox.value
                settings.articleFontUseSystem = useSystemFontCheckBox.checked
                settings.save()
                pageStack.pop()
            }
        }
    }
}
