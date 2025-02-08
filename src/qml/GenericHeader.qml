/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects

import org.kde.kirigami as Kirigami

import org.kde.kasts

Item {
    id: root
    required property string image
    required property string title

    property string subtitle: ""
    property var headerHeight: Kirigami.Units.gridUnit * 5

    property bool titleClickable: false
    property bool subtitleClickable: false
    signal titleClicked
    signal subtitleClicked

    implicitHeight: headerHeight
    implicitWidth: parent.width

    MultiEffect {
        id: backgroundImage
        source: backgroundImageRaw
        anchors.fill: parent

        brightness: -0.15
        saturation: 0.6
        contrast: -0.4
        blurMax: 64
        blur: 1.0
        blurEnabled: true
        autoPaddingEnabled: false

        ImageWithFallback {
            id: backgroundImageRaw
            anchors.fill: parent
            visible: false
            imageSource: image
            imageResize: false // no "stuttering" on resizing the window
        }
    }

    RowLayout {
        property int size: root.headerHeight - 2 * margin
        property int margin: Kirigami.Units.gridUnit * 1
        height: size
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: margin
        anchors.rightMargin: margin
        anchors.bottomMargin: margin

        ImageWithFallback {
            id: frontImage
            imageSource: image
            Layout.maximumHeight: parent.size
            Layout.maximumWidth: parent.size
            Layout.minimumHeight: parent.size
            Layout.minimumWidth: parent.size
            absoluteRadius: Kirigami.Units.smallSpacing
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    fullScreenImageLoader.setSource("qrc:/qt/qml/org/kde/kasts/qml/FullScreenImage.qml", {
                        image: root.image,
                        description: root.title,
                        loader: fullScreenImageLoader
                    });
                    fullScreenImageLoader.active = true;
                    fullScreenImageLoader.item.open();
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: parent.margin / 2
            Controls.Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: title
                fontSizeMode: Text.Fit
                font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 1.4)
                minimumPointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 1.2)
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignBottom
                color: "#eff0f1" // breeze light text color
                opacity: 1
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                MouseArea {
                    anchors.fill: parent
                    cursorShape: root.titleClickable ? Qt.PointingHandCursor : undefined
                    onClicked: {
                        if (root.titleClickable) {
                            root.titleClicked();
                        }
                    }
                }
            }

            Controls.Label {
                Layout.fillWidth: true
                visible: subtitle !== ""
                text: subtitle
                fontSizeMode: Text.Fit
                font.pointSize: Kirigami.Theme.defaultFont.pointSize
                minimumPointSize: Kirigami.Theme.defaultFont.pointSize
                font.bold: true
                horizontalAlignment: Text.AlignLeft
                color: "#eff0f1" // breeze light text color
                elide: Text.ElideRight
                opacity: 1
                MouseArea {
                    anchors.fill: parent
                    cursorShape: root.subtitleClickable ? Qt.PointingHandCursor : undefined
                    onClicked: {
                        if (root.subtitleClickable) {
                            root.subtitleClicked();
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: fullScreenImageLoader
        active: false
        visible: active
    }
}
