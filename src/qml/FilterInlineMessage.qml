/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.InlineMessage {
    required property var proxyModel

    anchors {
        horizontalCenter: parent.horizontalCenter
        bottom: parent.bottom
        margins: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing + (errorNotification.visible ? errorNotification.height + Kirigami.Units.largeSpacing : 0) + (updateNotification.visible ? updateNotification.height + Kirigami.Units.largeSpacing : 0) + (updateSyncNotification.visible ? updateSyncNotification.height + Kirigami.Units.largeSpacing : 0)
    }
    type: Kirigami.MessageType.Information
    visible: proxyModel.filterType != AbstractEpisodeProxyModel.NoFilter
    text: textMetrics.text
    width: Math.min(textMetrics.width + 2 * Kirigami.Units.largeSpacing + 10 * Kirigami.Units.gridUnit, parent.width - anchors.leftMargin - anchors.rightMargin)

    actions: [
        Kirigami.Action {
            id: resetButton
            icon.name: "edit-delete-remove"
            text: KI18n.i18nc("@action:button Reset filters active on ListView", "Reset")
            onTriggered: {
                proxyModel.filterType = AbstractEpisodeProxyModel.NoFilter;
            }
        }
    ]

    TextMetrics {
        id: textMetrics
        text: KI18n.i18nc("@info:status Name of the filter which is active on the ListView", "Active filter: %1", proxyModel.filterName)
    }
}
