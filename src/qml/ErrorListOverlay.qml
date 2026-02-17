/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.Dialog {
    id: errorOverlay
    preferredWidth: Kirigami.Units.gridUnit * 25
    preferredHeight: Kirigami.Units.gridUnit * 16

    showCloseButton: true

    title: KI18n.i18nc("@title", "Error Log")
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: Kirigami.Action {
        text: KI18n.i18nc("@action:button", "Clear All Errors")
        icon.name: "edit-clear-all"
        onTriggered: ErrorLogModel.clearAll()
        enabled: errorList.count > 0
    }

    ErrorList {
        id: errorList
        clip: true
    }
}
