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

    Kirigami.FormLayout {

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Article List")
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Delete after:")

            Controls.SpinBox {
                id: deleteAfterCount
                value: _settings.deleteAfterCount
                enabled: deleteAfterType.currentIndex !== 0

                onValueModified: _settings.deleteAfterCount = value
            }

            Controls.ComboBox {
                id: deleteAfterType
                currentIndex: _settings.deleteAfterType
                model: [i18n("Never"), i18n("Articles"), i18n("Days"), i18n("Weeks"), i18n("Months")]

                onActivated: _settings.deleteAfterType = index
            }
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Article")
        }

        Controls.SpinBox {
            id: articleFontSizeSpinBox

            enabled: !useSystemFontCheckBox.checked
            value: _settings.articleFontSize
            Kirigami.FormData.label: i18n("Font size:")
            from: 6
            to: 20

            onValueModified: _settings.articleFontSize = value

        }

        Controls.CheckBox {
            id: useSystemFontCheckBox
            checked: _settings.articleFontUseSystem
            text: i18n("Use system default")

            onToggled: _settings.articleFontUseSystem = checked
        }

    }
}
