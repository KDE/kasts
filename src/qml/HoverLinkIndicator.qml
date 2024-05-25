// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2024 Elias Probst <mail@eliasprobst.eu>
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

QQC2.Control {
    id: root

    property string text

    visible: root.text.length > 0
    Kirigami.Theme.colorSet: Kirigami.Theme.View

    z: 20

    Accessible.ignored: true

    contentItem: QQC2.Label {
        text: root.text
        Accessible.description: i18nc("@info screenreader", "The currently selected link")
    }

    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
    }
}
