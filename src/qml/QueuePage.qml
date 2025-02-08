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
    title: i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")

    property string lastEntry: ""
    property string pageName: "queuepage"
    property alias queueList: queueList

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    supportsRefreshing: true
    onRefreshingChanged: {
        if (refreshing) {
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

    header: Controls.Control {
        anchors.right: parent.right
        anchors.left: parent.left

        padding: Kirigami.Units.largeSpacing

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                }
            }
        }

        contentItem: Controls.Label {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18ncp("@info:progress", "1 Episode", "%1 Episodes", queueModel.rowCount()) + "  ·  " + i18nc("@info:progress", "Time Left") + ": " + queueModel.formattedTimeLeft
        }
    }

    GenericEntryListView {
        id: queueList
        reuseItems: true
        isQueue: true

        Kirigami.PlaceholderMessage {
            visible: queueList.count === 0

            width: Kirigami.Units.gridUnit * 20
            anchors.centerIn: parent

            text: i18nc("@info", "Queue is empty")
        }

        model: QueueModel {
            id: queueModel
        }

        delegate: FocusScope {
            id: focusScope
            width: queueList.width
            height: entryDelegate.height

            required property Entry entry
            required property int index

            GenericEntryDelegate {
                id: entryDelegate
                width: parent.width
                isQueue: true
                listView: queueList
                listViewObject: queueList
                focus: parent.visualFocus || parent.activeFocus
                entry: focusScope.entry
                index: focusScope.index
            }
        }

        moveDisplaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
    }

    ConnectionCheckAction {
        id: updateAllFeeds
    }
}
