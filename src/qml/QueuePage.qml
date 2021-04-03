/*
   SPDX-FileCopyrightText: 2021 (c) Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// Includes relevant modules used by the QML
import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.15
import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: queuepage
    title: i18n("Queue")

    Kirigami.PlaceholderMessage {
        visible: queueList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("Nothing added to the queue yet")
    }

    Component {
        id: delegateComponent
        QueueDelegate { }
    }

    ListView {
        id: queueList
        visible: count !== 0

        model: QueueModel {
            id: queueModel
        }
        delegate: Kirigami.DelegateRecycler {
            width: queueList.width
            sourceComponent: delegateComponent
        }
        anchors.fill: parent

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }
}

