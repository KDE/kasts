/**
   SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// Includes relevant modules used by the QML
import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.15
import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: queuepage
    title: i18n("Queue")

    property var lastEntry: ""

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing)  {
            Fetcher.fetchAll()
            refreshing = false
        }
    }

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh All Feeds")
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile || queueList.count === 0
    }

    Kirigami.PlaceholderMessage {
        visible: queueList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("Nothing added to the queue yet")
    }

    Component {
        id: delegateComponent
        GenericEntryDelegate {
            isQueue: true
            listView: queueList
        }
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

