/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Item {
    id: root
    required property string image
    required property string title

    property string subtitle: ""
    property var headerHeight: Kirigami.Units.gridUnit * 5

    property bool clickable: false
    signal clicked()

    implicitHeight: headerHeight
    implicitWidth: parent.width

    ImageWithFallback {
        id: backgroundImage
        anchors.fill: parent
        imageSource: image
        imageResize: false // no "stuttering" on resizing the window

        layer.enabled: true
        layer.effect: HueSaturation {
            cached: true

            lightness: -0.3
            saturation: 0.9

            layer.enabled: true
            layer.effect: FastBlur {
                cached: true
                radius: 64
                transparentBorder: false
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (root.clickable) {
                root.clicked();
            }
        }
    }

    RowLayout {
        property int size: root.headerHeight -  2 * margin
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
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: parent.margin/2
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
            }
        }
    }
}


