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

import org.kde.kasts 1.0

Controls.ItemDelegate {
    id: feedDelegate

    required property int cardSize
    required property int cardMargin

    property int borderWidth: Kirigami.Units.devicePixelRatio

    implicitWidth: cardSize + 2 * cardMargin
    implicitHeight: cardSize + 2 * cardMargin

    Accessible.role: Accessible.Button
    Accessible.name: feed.name
    Accessible.onPressAction: {
         feedDelegate.click()
    }

    onClicked: {
        lastFeed = feed.url
        pageStack.push("qrc:/EntryListPage.qml", {"feed": feed})
    }

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

        ImageWithFallback {
            id: img
            anchors.fill: parent
            imageSource: feed.cachedImage
            imageTitle: feed.name
            isLoading: feed.refreshing
        }

        Rectangle {
            id: countRectangle
            visible: feed.unreadEntryCount > 0
            anchors.top: img.top
            anchors.right: img.right
            width: actionsButton.width
            height: actionsButton.height
            color: Kirigami.Theme.highlightColor
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
            anchors.fill: actionsButton
            color: "black"
            opacity: 0.5
            radius: Kirigami.Units.smallSpacing - 2 * borderWidth
        }

        Controls.Button {
            id: actionsButton
            anchors.right: img.right
            anchors.bottom: img.bottom
            anchors.margins: 0
            padding: 0
            flat: true
            icon.name: "overflow-menu"
            icon.color: "white"
            onClicked: actionOverlay.open()
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

    Kirigami.OverlaySheet {
        id: actionOverlay
        parent: applicationWindow().overlay
        showCloseButton: true

        header: Kirigami.Heading {
            text: feed.name
            level: 2
            wrapMode: Text.Wrap
        }

        contentItem: ColumnLayout {
            RowLayout {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 20
                spacing: 0

                Kirigami.BasicListItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                    leftPadding: Kirigami.Units.smallSpacing
                    rightPadding: 0
                    onClicked: {
                        if(feed.url === lastFeed)
                            while(pageStack.depth > 1)
                                pageStack.pop()
                        DataManager.removeFeed(feed)
                        actionOverlay.close();
                    }
                    icon: "delete"
                    text: i18n("Remove Podcast")
                }
            }

        }
    }
}
