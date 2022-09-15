/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Kirigami.Dialog {
    id: errorOverlay
    preferredWidth: Kirigami.Units.gridUnit * 25
    preferredHeight: Kirigami.Units.gridUnit * 16

    showCloseButton: true

    title: i18nc("@title", "Error Log")
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: Kirigami.Action {
        text: i18nc("@action:button", "Clear All Errors")
        icon.name: "edit-clear-all"
        onTriggered: ErrorLogModel.clearAll()
        enabled: errorList.count > 0
    }

    ErrorList {
        id: errorList
    }
}
