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
            required property string content
            required property int enclosureStatus
            required property string link
            required property bool isNew
            required property bool read
            required property bool favorite
            required property bool removed
            required property date updated
            required property string image
            required property string feeduid
            required property string feedImage
            required property string feedName
            required property bool queueStatus
            required property bool hasEnclosure
            required property bool enclosureUrl
            required property int playPosition
            required property int duration
            required property int size
            required property int downloadSize

            property GenericEntryDelegate entryDelegate: _delegate

            GenericEntryDelegate {
                id: _delegate
                width: parent.width
                isQueue: true
                listViewObject: root.queueList
                focus: parent.activeFocus

                // required properties from model need to passed on manually
                entryuid: focusScope.entryuid
                index: focusScope.index
                title: focusScope.title
                content: focusScope.content
                enclosureStatus: focusScope.enclosureStatus
                link: focusScope.link
                isNew: focusScope.isNew
                read: focusScope.read
                favorite: focusScope.favorite
                removed: focusScope.removed
                updated: focusScope.updated
                image: focusScope.image
                feeduid: focusScope.feeduid
                feedImage: focusScope.feedImage
                feedName: focusScope.feedName
                queueStatus: focusScope.queueStatus
                hasEnclosure: focusScope.hasEnclosure
                enclosureUrl: focusScope.enclosureUrl
                playPosition: focusScope.playPosition
                duration: focusScope.duration
                size: focusScope.size
                downloadSize: focusScope.downloadSize
            }

            // This function is needed to close the EntryPage if it is opened over the
            // QueuePage when the episode is removed from the queue (e.g. when the
            // episode finishes).
            ListView.onPooled: {
                const pageStack = (root.Controls.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack;
                if (pageStack.depth > 1) {
                    if (pageStack.get(0).pageName === "queuepage") {
                        if (pageStack.get(0).lastEntry && pageStack.get(0).lastEntry === focusScope.entryuid) {
                            // if this EntryPage was open, then close it
                            pageStack.pop();
                        }
                    }
                }
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
