/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.ScrollablePage {
    id: queuepage
    title: i18nc("@title:column Page showing the list queued items", "Queue")

    property var lastEntry: ""
    property string pageName: "queuepage"
    property alias queueList: queueList

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing)  {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    readonly property list<Kirigami.Action> pageActions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: i18nc("@action:intoolbar", "Refresh All Podcasts")
            onTriggered: refreshing = true
        }
    ]

    Component.onCompleted: {
        for (var i in queueList.defaultActionList) {
            pageActions.push(queueList.defaultActionList[i]);
        }
    }

    actions: pageActions

    GenericEntryListView {
        id: queueList
        reuseItems: true
        isQueue: true
        anchors.fill: parent

        Kirigami.PlaceholderMessage {
            visible: queueList.count === 0

            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent

            text: i18nc("@info", "Queue is empty")
        }

        header: ColumnLayout {
            anchors.right: parent.right
            anchors.left: parent.left
            Controls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: i18ncp("@info:progress", "1 Episode", "%1 Episodes", queueModel.rowCount()) + "  ·  " + i18nc("@info:progress", "Time Left") + ": " + queueModel.formattedTimeLeft
            }
            Kirigami.Separator {
                Layout.fillWidth: true
            }
        }

        model: QueueModel {
            id: queueModel
        }

        delegate: Item {
            width: queueList.width
            height: entryDelegate.height
            GenericEntryDelegate {
                id: entryDelegate
                isQueue: true
                listView: queueList
            }
        }

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }
}
