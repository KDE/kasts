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

import org.kde.kirigami 2.15 as Kirigami

import org.kde.alligator 1.0

Item {

    property int cardSize
    property int cardMargin

    Image {
        id: img
        asynchronous: true
        source: feed.image === "" ? "logo.png" : "file://"+Fetcher.image(feed.image)
        fillMode: Image.PreserveAspectFit
        x: cardMargin
        y: cardMargin
        sourceSize.width: cardSize
        sourceSize.height: cardSize
        height: cardSize - cardMargin
        width: cardSize - cardMargin

        MouseArea {
            anchors.fill: parent
            onClicked: {
                lastFeed = feed.url
                pageStack.push("qrc:/EntryListPage.qml", {"feed": feed})
            }
        }
    }

    Rectangle {
        id: rectangle
        visible: feed.unreadEntryCount > 0
        anchors.top: img.top
        anchors.right: img.right
        width: img.width/5
        height: img.height/5
        color: Kirigami.Theme.highlightColor
    }

    Controls.Label {
        id: countLabel
        visible: feed.unreadEntryCount > 0
        anchors.centerIn: rectangle
        anchors.margins: Kirigami.Units.smallSpacing
        text: feed.unreadEntryCount
        font.bold: true
        color: Kirigami.Theme.highlightedTextColor
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
