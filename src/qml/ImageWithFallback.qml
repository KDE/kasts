/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15
import QtQuick.Window 2.2

import org.kde.kirigami 2.15 as Kirigami

import org.kde.kasts 1.0

Item {
    id: root
    property string imageSource: ""
    property real imageOpacity: 1
    property int absoluteRadius: 0
    property real fractionalRadius: 0.0
    property string imageTitle: ""
    property bool isLoading: false
    property int imageFillMode: Image.PreserveAspectCrop
    property bool imageResize: true
    property bool mipmap: false

    Loader {
        id: imageLoader
        anchors.fill: parent
        sourceComponent: imageSource === "no-image" ? fallbackImg : (imageSource === "fetching" ? loaderSymbol : realImg )
        opacity: root.imageOpacity
        layer.enabled: (root.absoluteRadius > 0 || root.fractionalRadius > 0)
        layer.effect: OpacityMask {
            maskSource: Item {
                width: imageLoader.width
                height: imageLoader.height
                Rectangle {
                    anchors.centerIn: parent
                    width: imageLoader.adapt ? imageLoader.width : Math.min(imageLoader.width, imageLoader.height)
                    height: imageLoader.adapt ? imageLoader.height : width
                    radius: (absoluteRadius > 0) ? absoluteRadius : ( (fractionalRadius > 0) ? Math.min(width, height)*fractionalRadius : 0 )
                }
            }
        }
    }

    Component {
        id: realImg
        Image {
            anchors.fill: parent
            source: root.imageSource
            fillMode: root.imageFillMode
            sourceSize.width: root.imageResize ? width * Screen.devicePixelRatio : undefined
            sourceSize.height: root.imageResize ? height * Screen.devicePixelRatio : undefined
            asynchronous: true
            mipmap: root.mipmap
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
        active: root.imageTitle !== "" && (SettingsManager.alwaysShowFeedTitles ? true : (imageSource === "no-image" || imageSource === "fetching"))
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
        active: isLoading
        sourceComponent: loaderSymbol
        anchors.fill: parent
    }
}
