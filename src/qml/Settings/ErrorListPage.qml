/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard

import org.kde.kasts 1.0

FormCard.FormCardPage {
    title: i18nc("@title", "Error Log")

    FormCard.FormHeader {
        title: i18nc("@title", "Error Log")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.AbstractFormDelegate {
            background: null
            contentItem: ErrorList {
                id: errorList
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            trailing: Controls.Button {
                icon.name: "edit-clear-all"
                text: i18nc("@action:button", "Clear All Errors")
                onClicked: ErrorLogModel.clearAll()
                enabled: errorList.count > 0
            }
        }
    }
}
