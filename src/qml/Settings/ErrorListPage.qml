/**
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18nc("@title", "Error Log")

    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    ColumnLayout {
        spacing: 0

        MobileForm.FormCard {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18nc("@title", "Error Log")
                }

                MobileForm.AbstractFormDelegate {
                    background: Item {}
                    contentItem: ErrorList {
                        id: errorList
                    }
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
                    trailing: Controls.Button {
                        icon.name: "edit-clear-all"
                        text: i18nc("@action:button", "Clear All Errors")
                        onClicked: ErrorLogModel.clearAll()
                        enabled: errorList.count > 0
                    }
                }
            }
        }
    }
}
