/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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

    property QtObject feed;
    property var headerHeight: Kirigami.Units.gridUnit * 8

    title: i18nc("<Feed Name> - Details", "%1 - Details", feed.name)

    header: Item {
        id: headerImage
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        height: headerHeight
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

    ColumnLayout {
        Kirigami.Heading {
            text: feed.description;
            level: 3
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18nc("by <author(s)>", "by %1", feed.authors[0].name)
            visible: feed.authors.length !== 0
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: "<a href='%1'>%1</a>".arg(feed.link)
            onLinkActivated: Qt.openUrlExternally(link)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("Subscribed since: %1", feed.subscribed.toLocaleString(Qt.locale(), Locale.ShortFormat))
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("last updated: %1", feed.lastUpdated.toLocaleString(Qt.locale(), Locale.ShortFormat))
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("%1 posts, %2 unread", feed.entryCount, feed.unreadEntryCount)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
