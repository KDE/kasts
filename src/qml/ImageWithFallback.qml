/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Effects
import QtQuick.Window

import org.kde.kirigami as Kirigami

import org.kde.kasts

Item {
    id: root
    property string imageSource: "no-image"
    property real imageOpacity: 1
    property int absoluteRadius: 0
    property real fractionalRadius: 0.0
    property string imageTitle: ""
    property bool isLoading: false
    property int imageFillMode: Image.PreserveAspectCrop
    property bool imageResize: true

    Loader {
        id: imageLoader
        anchors.fill: parent
        visible: GraphicsInfo.api === GraphicsInfo.Software
        sourceComponent: (root.imageSource === "no-image" || root.imageSource === "") ? fallbackImg : (root.imageSource === "fetching" ? loaderSymbol : realImg)
    }

    MultiEffect {
        anchors.fill: parent
        source: imageLoader.item as Item
        opacity: root.imageOpacity
        maskEnabled: true
        maskThresholdMin: 0.5
        maskSpreadAtMin: 0.5
        maskSource: ShaderEffectSource {
            width: imageLoader.width
            height: imageLoader.height
            sourceItem: Rectangle {
                anchors.centerIn: parent
                width: Math.min(imageLoader.width, imageLoader.height)
                height: width
                radius: (root.absoluteRadius > 0) ? root.absoluteRadius : ((root.fractionalRadius > 0) ? Math.min(width, height) * root.fractionalRadius : 0)
            }
        }
    }

    Component {
        id: realImg
        Image {
            anchors.fill: parent
            source: root.imageSource
            fillMode: root.imageFillMode
            asynchronous: true
            mipmap: true
            sourceSize.width: 1024
            sourceSize.height: 1024
        }
    }

    Component {
        id: fallbackImg
        Item {
            anchors.fill: parent
            // Add white background color in order to use coloroverlay later on
            Rectangle {
                anchors.fill: parent
                color: "white"
            }
            Kirigami.Icon {
                anchors.fill: parent
                source: "rss"
                isMask: true
                color: "black"
            }
        }
    }

    Component {
        id: imageText
        Item {
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
                text: root.imageTitle
                level: 3
                font.bold: true
                color: "white"
                wrapMode: Text.Wrap
                elide: Text.ElideRight
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.imageTitle !== "" && (SettingsManager.alwaysShowFeedTitles ? true : (root.imageSource === "no-image" || root.imageSource === "fetching"))
        sourceComponent: imageText
    }

    Component {
        id: loaderSymbol
        Item {
            anchors.fill: parent
            Rectangle {
                color: "white"
                opacity: 0.5
                anchors.fill: parent
            }
            Controls.BusyIndicator {
                anchors.centerIn: parent
                width: parent.width / 2
                height: parent.height / 2
            }
        }
    }

    Loader {
        active: root.isLoading
        sourceComponent: loaderSymbol
        anchors.fill: parent
    }
}
