/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.SwipeListItem {

    contentItem: Kirigami.BasicListItem {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        text: model.entry.title
        subtitle: model.entry.updated.toLocaleString(Qt.locale(), Locale.ShortFormat) + (model.entry.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + model.entry.authors[0].name)
        reserveSpaceForIcon: false
        bold: !model.entry.read

        onClicked: {
            model.entry.read = true
            pageStack.push("qrc:/EntryPage.qml", {"entry": model.entry})
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Open in Browser")
            icon.name: "globe"
            onTriggered: Qt.openUrlExternally(model.entry.link)
        }
    ]
}
