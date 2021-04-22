/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtMultimedia 5.15
import org.kde.kirigami 2.15 as Kirigami

import org.kde.alligator 1.0


Kirigami.ScrollablePage {

    Kirigami.PlaceholderMessage {
        visible: errorList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("No errors logged")
    }
    Component {
        id: errorListDelegate
        Controls.Label {
            text: error.string + error.date + error.string
        }
    }

    ListView {
        anchors.fill: parent
        id: errorList
        visible: count !== 0
        model: ErrorLogModel

        delegate: Kirigami.DelegateRecycler {
            width: errorList.width
            sourceComponent: errorListDelegate
        }
    }
    actions.main: Kirigami.Action {
        text: i18n("Clear all errors")
        iconName: "edit-clear-all"
        onTriggered: ErrorLogModel.clearAll()
    }
}
