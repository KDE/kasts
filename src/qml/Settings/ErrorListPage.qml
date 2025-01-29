/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import org.kde.kasts

import ".."

FormCard.FormCardPage {
    title: i18nc("@title:menu Category in settings", "Error Log")

    actions: [
        Kirigami.Action {
            icon.name: "edit-clear-all"
            text: i18nc("@action:button", "Clear All")
            onTriggered: ErrorLogModel.clearAll()
            enabled: errorList.count > 0
        }
    ]

    FormCard.FormCard {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing * 4

        FormCard.AbstractFormDelegate {
            background: null
            contentItem: ErrorList {
                id: errorList
            }
        }
    }
}
