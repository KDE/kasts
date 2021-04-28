/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Controls.ItemDelegate {
    id: feedDelegate

    required property int cardSize
    required property int cardMargin

    property int borderWidth: Kirigami.Units.devicePixelRatio

    implicitWidth: cardSize + 2 * cardMargin
    implicitHeight: cardSize + 2 * cardMargin

    background: Kirigami.ShadowedRectangle {
        anchors.fill: parent
        anchors.margins: cardMargin
        anchors.leftMargin: cardMargin
        color: Kirigami.Theme.backgroundColor

        radius: Kirigami.Units.smallSpacing

        shadow.size: Kirigami.Units.largeSpacing
        shadow.color: Qt.rgba(0.0, 0.0, 0.0, 0.15)
        shadow.yOffset: borderWidth * 2

        border.width: borderWidth
        border.color: Qt.tint(Kirigami.Theme.textColor,
                            Qt.rgba(color.r, color.g, color.b, 0.6))
    }

    contentItem: Item {
        anchors.fill: parent
        anchors.margins: cardMargin + borderWidth
        anchors.leftMargin: cardMargin + borderWidth
        implicitWidth:  cardSize - 2 * borderWidth
        implicitHeight: cardSize  - 2 * borderWidth

        Loader {
            id: img
            anchors.fill: parent
            sourceComponent: (feed.image === "") ? fallbackImg : realImg
        }

        Component {
            id: realImg
            Image {
                //id: img
                visible: feed.image !== ""
                asynchronous: true
                source: feed.image === "" ? "logo.png" : "file://" + Fetcher.image(feed.image)
                fillMode: Image.PreserveAspectFit
                sourceSize.width: cardSize - 2 * borderWidth
                sourceSize.height: cardSize - 2 * borderWidth
            }
        }

        Component {
            id: fallbackImg
            Item {
                anchors.fill: img
                Kirigami.Icon {
                    visible: (feed.image === "")
                    anchors.fill: parent
                    source: "rss"
                }

                Rectangle {
                    anchors.fill: header
                    opacity: 0.5
                    color: "black"
                }

                Kirigami.Heading {
                    id: header
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    padding: 10
                    text: feed.name
                    level: 2
                    color: "white"
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                }
            }


        }

        MouseArea {
            anchors.fill: img
            onClicked: {
                lastFeed = feed.url
                pageStack.push("qrc:/EntryListPage.qml", {"feed": feed})
            }
        }

        Rectangle {
            id: countRectangle
            visible: feed.unreadEntryCount > 0
            anchors.top: img.top
            anchors.right: img.right
            width: actionsButton.width
            height: actionsButton.height
            color: Kirigami.Theme.highlightColor
            opacity: 0.8
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth
        }

        Controls.Label {
            id: countLabel
            visible: feed.unreadEntryCount > 0
            anchors.centerIn: countRectangle
            anchors.margins: Kirigami.Units.smallSpacing
            text: feed.unreadEntryCount
            font.bold: true
            color: Kirigami.Theme.highlightedTextColor
        }

        Rectangle {
            id: actionsRectangle
            visible: false  //TODO: temporary hack
            anchors.fill: actionsButton
            color: "black"
            opacity: 0.5
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth
        }

        Controls.Button {
            id: actionsButton
            visible: false  //TODO: temporary hack
            anchors.right: img.right
            anchors.bottom: img.bottom
            anchors.margins: 0
            padding: 0
            flat: true
            icon.name: "overflow-menu"
            icon.color: "white"
        }

        Kirigami.ActionToolBar {
            anchors.right: img.right
            anchors.left: img.left
            anchors.bottom: img.bottom
            anchors.margins: 0
            padding: 0
            actions: [
                Kirigami.Action {
                    icon.name: "delete"
                    displayHint: Kirigami.Action.DisplayHint.AlwaysHide
                    onTriggered: {
                        if(pageStack.depth > 1 && feed.url === lastFeed)
                            while(pageStack.depth > 1)
                                pageStack.pop()
                        feedsModel.removeFeed(index)
                    }
                }
            ]
        }

        // Rounded edges
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Item {
                width: img.width
                height: img.height
                Rectangle {
                    anchors.centerIn: parent
                    width: img.adapt ? img.width : Math.min(img.width, img.height)
                    height: img.adapt ? img.height : width
                    radius: Kirigami.Units.smallSpacing - borderWidth
                }
            }
        }
    }

    /*actions: [
        Kirigami.Action {
            icon.name: "delete"
            onTriggered: {
                if(pageStack.depth > 1 && feed.url === lastFeed)
                    while(pageStack.depth > 1)
                        pageStack.pop()
                feedsModel.removeFeed(index)
            }
        }

    ]*/

}
