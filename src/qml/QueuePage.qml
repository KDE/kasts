/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.coreaddons

import org.kde.kasts

Kirigami.ScrollablePage {
    id: root
    title: KI18n.i18nc("@title of page showing the list queued items; this is the noun 'the queue', not the verb", "Queue")

    property int lastEntry: 0
    property string pageName: "queuepage"
    property alias queueList: queueList

    LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
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
            text: KI18n.i18nc("@action:intoolbar", "Refresh All Podcasts")
            onTriggered: root.refreshing = true
        }
    ]

    Component.onCompleted: {
        for (let i in queueList.defaultActionList) {
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
            text: KI18n.i18ncp("@info:progress", "1 Episode", "%1 Episodes", QueueModel.rowCount()) + "  ·  " + KI18n.i18nc("@info:progress", "Time Left") + ": " + Format.formatDuration(QueueModel.timeLeft, Format.HideSeconds | Format.InitialDuration)
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

            text: KI18n.i18nc("@info", "Queue is empty")
        }

        model: QueueModel

        delegate: FocusScope {
            id: focusScope
            width: root.queueList.width
            height: entryDelegate.height

            required property int entryuid
            required property int index
            required property string title
            required property int downloaded
            required property bool hasEnclosure
            required property bool isNew
            required property bool read
            required property bool favorite
            required property bool removed
            required property date updated
            required property string image
            required property string feedImage
            required property string feedName
            required property bool queueStatus
            required property int playPosition
            required property int duration
            required property int size

            GenericEntryDelegate {
                id: entryDelegate
                width: parent.width
                isQueue: true
                listViewObject: root.queueList
                focus: parent.activeFocus

                // required properties from model need to passed on manually
                entryuid: focusScope.entryuid
                index: focusScope.index
                title: focusScope.title
                downloaded: focusScope.downloaded
                hasEnclosure: focusScope.hasEnclosure
                isNew: focusScope.isNew
                read: focusScope.read
                favorite: focusScope.favorite
                removed: focusScope.removed
                updated: focusScope.updated
                image: focusScope.image
                feedImage: focusScope.feedImage
                feedName: focusScope.feedName
                queueStatus: focusScope.queueStatus
                playPosition: focusScope.playPosition
                duration: focusScope.duration
                size: focusScope.size
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
