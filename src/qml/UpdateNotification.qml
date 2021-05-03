/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.15 as Kirigami
import org.kde.kasts 1.0

/*
 * This visually mimics the Kirigami.InlineMessage due to the
 * BusyIndicator, which is not supported by the InlineMessage.
 * Consider implementing support for the BusyIndicator within
 * the InlineMessage in the future.
 */
Rectangle {
    id: rootComponent

    color: Kirigami.Theme.activeTextColor

    width: (labelWidth.boundingRect.width - labelWidth.boundingRect.x) + 3 * Kirigami.Units.largeSpacing +
           indicator.width
    height: indicator.height

    visible: opacity > 0
    opacity: 0

    radius: Kirigami.Units.smallSpacing / 2

    Rectangle {
        id: bgFillRect

        anchors.fill: parent
        anchors.margins: Kirigami.Units.devicePixelRatio

        color: Kirigami.Theme.backgroundColor

        radius: rootComponent.radius * 0.60
    }

    Rectangle {
        anchors.fill: bgFillRect
        color: rootComponent.color
        opacity: 0.20
        radius: bgFillRect.radius
    }

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        Controls.BusyIndicator {
            id: indicator
            Layout.alignment: Qt.AlignVCenter
        }

        Controls.Label {
            id: feedUpdateCountLabel
            text: i18ncp("number of updated feeds",
                         "Updated %2 of %1 feed",
                         "Updated %2 of %1 feeds",
                              Fetcher.updateTotal, Fetcher.updateProgress)
            color: Kirigami.Theme.textColor

            Layout.fillWidth: true
            //Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
        }
    }

    TextMetrics {
        id: labelWidth

        text: i18ncp("number of updated feeds",
                     "Updated %2 of %1 feed",
                     "Updated %2 of %1 feeds",
                     999, 999)
    }

    Timer {
        id: hideTimer

        interval: 2000
        repeat: false

        onTriggered:
        {
            rootComponent.opacity = 0
        }
    }

    Behavior on opacity {
        NumberAnimation {
            easing.type: Easing.InOutQuad
            duration: Kirigami.Units.longDuration
        }
    }

    Connections {
        target: Fetcher
        function onUpdatingChanged() {
            if (Fetcher.updating) {
                hideTimer.stop()
                opacity = 1
            } else {
                hideTimer.start()
            }
        }
    }
}
