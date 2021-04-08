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

Kirigami.ScrollablePage {
    id: page

    property var feed

    title: feed.name
    supportsRefreshing: true

    onRefreshingChanged:
        if(refreshing) {
            feed.refresh()
        }

    Connections {
        target: feed
        function onRefreshingChanged(refreshing) {
            if(!refreshing)
                page.refreshing = refreshing
        }
    }

    contextualActions: [
        Kirigami.Action {
            iconName: "help-about-symbolic"
            text: i18n("Details")
            onTriggered: {
                while(pageStack.depth > 2)
                    pageStack.pop()
                pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
            }
        }
    ]

    actions.main: Kirigami.Action {
        iconName: "view-refresh"
        text: i18n("Refresh Feed")
        onTriggered: page.refreshing = true
        visible: !Kirigami.Settings.isMobile || entryList.count === 0
    }

    Kirigami.PlaceholderMessage {
        visible: entryList.count === 0

        width: Kirigami.Units.gridUnit * 20
        anchors.centerIn: parent

        text: feed.errorId === 0 ? i18n("No Entries available") : i18n("Error (%1): %2", feed.errorId, feed.errorString)
        icon.name: feed.errorId === 0 ? "" : "data-error"
    }

    Component {
        id: entryListDelegate
        EntryListDelegate { }
    }

    ListView {
        id: entryList
        visible: count !== 0
        model: page.feed.entries

        delegate: Kirigami.DelegateRecycler {
            width: entryList.width
            sourceComponent: entryListDelegate
        }

        //onOriginYChanged: contentY = originY // Why is this needed?

        //headerPositioning: ListView.OverlayHeader  // seems broken
        header: Item {
            //anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            height: Kirigami.Units.gridUnit * 8
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    while(pageStack.depth > 2)
                        pageStack.pop()
                    pageStack.push("qrc:/FeedDetailsPage.qml", {"feed": feed})
                }
            }
            Image {
                id: backgroundimage
                source: page.feed.image === "" ? "logo.png" : "file://"+Fetcher.image(page.feed.image)
                fillMode: Image.PreserveAspectCrop
                anchors.fill: parent
                asynchronous: true
            }
            GaussianBlur {
                id: blur
                anchors.fill: backgroundimage
                source: backgroundimage
                radius: 12
                samples: 16
                deviation: 6
            }
            ColorOverlay {
                anchors.fill: blur
                source: blur
                color:"#87000000"  //RGBA, but first value is actually the alpha channel
            }
            RowLayout {
                property int size: Kirigami.Units.gridUnit * 6
                property int margin: Kirigami.Units.gridUnit * 1
                height: size
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: margin
                anchors.rightMargin: margin
                anchors.bottomMargin: margin

                Image {
                    id: frontimage
                    source: page.feed.image === "" ? "logo.png" : "file://"+Fetcher.image(page.feed.image)
                    Layout.maximumHeight: parent.size
                    Layout.maximumWidth: parent.size
                    asynchronous: true
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: parent.margin/2
                    Controls.Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: page.feed.name
                        fontSizeMode: Text.Fit
                        font.pointSize: 18
                        minimumPointSize: 12
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignBottom
                        color: "white"
                        opacity: 1
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }
                    Controls.Label {
                        Layout.fillWidth: true
                        text: page.feed.authors.length === 0 ? "" : i18nc("by <author(s)>", "by") + " " + page.feed.authors[0].name
                        fontSizeMode: Text.Fit
                        font.pointSize: 12
                        minimumPointSize: 10
                        horizontalAlignment: Text.AlignLeft
                        color: "white"
                        elide: Text.ElideRight
                        opacity: 1
                    }
                }
            }
        }


        /* ColumnLayout {
            width: parent.width
            spacing: 0

            Kirigami.InlineMessage {
                type: Kirigami.MessageType.Error
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: i18n("Error (%1): %2", page.feed.errorId, page.feed.errorString)
                visible: page.feed.errorId !== 0
            }
            RowLayout {
                width: parent.width
                height: root.height * 0.2

                Image {
                    source: page.feed.image === "" ? "rss" : "file://"+Fetcher.image(page.feed.image)
                    property int size: Kirigami.Units.iconSizes.large
                    Layout.minimumWidth: size
                    Layout.minimumHeight: size
                    Layout.maximumWidth: size
                    Layout.maximumHeight: size
                    asynchronous: true
                }

                ColumnLayout {
                    Kirigami.Heading {
                        text: page.feed.name
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: page.feed.description
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: page.feed.authors.length === 0 ? "" : " " + i18nc("by <author(s)>", "by") + " " + page.feed.authors[0].name
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }*/
    }
}
