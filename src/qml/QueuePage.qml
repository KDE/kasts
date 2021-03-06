/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.2
import QtQml.Models 2.15

import org.kde.kirigami 2.13 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: queuepage
    title: i18n("Queue")

    property var lastEntry: ""
    property string pageName: "queuepage"
    property alias queueList: queueList

    supportsRefreshing: true
    onRefreshingChanged: {
        if(refreshing)  {
            updateAllFeeds.run();
            refreshing = false;
        }
    }

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh All Podcasts")
        onTriggered: refreshing = true
    }

    contextualActions: queueList.defaultActionList

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

    GenericEntryListView {
        id: queueList
        isQueue: true
        visible: count !== 0
        anchors.fill: parent

        header: ColumnLayout {
            anchors.right: parent.right
            anchors.left: parent.left
            Controls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: i18np("1 Episode", "%1 Episodes", queueModel.rowCount()) + "  ·  " + i18n("Time Left") + ": " + queueModel.formattedTimeLeft
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
