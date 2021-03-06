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
    required property string image
    required property string title
    property string subtitle: ""
    property var headerHeight: Kirigami.Units.gridUnit * 8

    implicitHeight: headerHeight
    implicitWidth: parent.width

    ImageWithFallback {
        id: backgroundImage
        anchors.fill: parent
        imageSource: image
    }
    GaussianBlur {
        id: blur
        anchors.fill: backgroundImage
        source: backgroundImage
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
                visible: subtitle !== ""
                text: subtitle
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


