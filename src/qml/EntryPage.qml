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

Kirigami.ScrollablePage {
    id: page

    property QtObject entry

    title: entry.title

    Controls.Label {
        text: page.entry.content
        baseUrl: page.entry.baseUrl
        textFormat: Text.RichText
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        onLinkActivated: Qt.openUrlExternally(link)
        onWidthChanged: text = entry.adjustedContent(width, font.pixelSize)
    }

    actions.main: Kirigami.Action {
        text: i18n("Open in Browser")
        icon.name: "globe"
        onTriggered: Qt.openUrlExternally(entry.link)
    }
}
