/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.2

import org.kde.kirigami 2.13 as Kirigami
import org.kde.kcoreaddons 1.0 as KCoreAddons

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
        text: i18n("Refresh All Podcasts")
        onTriggered: refreshing = true
        visible: !Kirigami.Settings.isMobile || queueList.count === 0
    }

    Kirigami.PlaceholderMessage {
        visible: queueList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: i18n("Nothing Added to the Queue Yet")
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
        anchors.fill: parent

        header: ColumnLayout {
            anchors.right: parent.right
            anchors.left: parent.left
            Controls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: i18np("1 Episode", "%1 Episodes", queueModel.rowCount()) + "  ·  " + i18n("Time Left") + ": " + KCoreAddons.Format.formatDuration(queueModel.timeLeft)
            }
            Kirigami.Separator {
                Layout.fillWidth: true
            }
        }

        model: QueueModel {
            id: queueModel
        }

        delegate: Kirigami.DelegateRecycler {
            width: queueList.width
            sourceComponent: delegateComponent
        }

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }
}
