/**
 * SPDX-FileCopyrightText: 2019 Black Hat <bhat@encom.eu.org>
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.19 as Kirigami

Controls.Popup {
    id: root

    required property var image
    required property QtObject loader
    property string description: undefined

    property int imageWidth: -1
    property int imageHeight: -1

    width: parent.width
    height: parent.height

    parent: Controls.Overlay.overlay
    closePolicy: Controls.Popup.CloseOnEscape
    modal: true
    padding: 0
    background: null

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        Controls.Control {
            Layout.fillWidth: true

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.ActionToolBar {
                    Layout.fillWidth: true
                    alignment: Qt.AlignRight
                    actions: [
                        Kirigami.Action {
                            text: i18n("Zoom in")
                            icon.name: "zoom-in"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            onTriggered: {
                                imageItem.scaleFactor = imageItem.scaleFactor + 0.25
                                if (imageItem.scaleFactor > 3) {
                                    imageItem.scaleFactor = 3
                                }
                            }
                        },
                        Kirigami.Action {
                            text: i18n("Zoom out")
                            icon.name: "zoom-out"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            onTriggered: {
                                imageItem.scaleFactor = imageItem.scaleFactor - 0.25
                                if (imageItem.scaleFactor < 0.25) {
                                    imageItem.scaleFactor = 0.25
                                }
                            }
                        },
                        Kirigami.Action {
                            text: i18n("Rotate left")
                            icon.name: "image-rotate-left-symbolic"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            onTriggered: imageItem.rotationAngle = imageItem.rotationAngle - 90

                        },
                        Kirigami.Action {
                            text: i18n("Rotate right")
                            icon.name: "image-rotate-right-symbolic"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            onTriggered: imageItem.rotationAngle = imageItem.rotationAngle + 90

                        },
                        Kirigami.Action {
                            text: i18n("Close")
                            icon.name: "dialog-close"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            onTriggered: root.close()
                        }
                    ]
                }
            }

            background: Rectangle {
                color: Kirigami.Theme.alternateBackgroundColor
            }

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 1
            }
        }

        Item {
            id: imageContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true

            MouseArea {
                anchors.fill: parent
                onClicked: root.close()
            }

            AnimatedImage {
                id: imageItem

                property var scaleFactor: 1
                property int rotationAngle: 0
                property var rotationInsensitiveWidth: Math.min(root.imageWidth > 0 ? root.imageWidth : sourceSize.width, imageContainer.width - Kirigami.Units.largeSpacing * 2)
                property var rotationInsensitiveHeight: Math.min(root.imageHeight > 0 ? root.imageHeight : sourceSize.height, imageContainer.height - Kirigami.Units.largeSpacing * 2)

                anchors.centerIn: parent
                width: rotationAngle % 180 === 0 ? rotationInsensitiveWidth : rotationInsensitiveHeight
                height: rotationAngle % 180 === 0 ? rotationInsensitiveHeight : rotationInsensitiveWidth
                fillMode: Image.PreserveAspectFit
                clip: true
                source: root.image

                MouseArea {
                    anchors.centerIn: parent
                    width: parent.paintedWidth
                    height: parent.paintedHeight
                }

                Behavior on width {
                    NumberAnimation {duration: Kirigami.Units.longDuration; easing.type: Easing.InOutCubic}
                }
                Behavior on height {
                    NumberAnimation {duration: Kirigami.Units.longDuration; easing.type: Easing.InOutCubic}
                }

                transform: [
                    Rotation {
                        origin.x: imageItem.width / 2
                        origin.y: imageItem.height / 2
                        angle: imageItem.rotationAngle

                        Behavior on angle {
                            RotationAnimation {duration: Kirigami.Units.longDuration; easing.type: Easing.InOutCubic}
                        }
                    },
                    Scale {
                        origin.x: imageItem.width / 2
                        origin.y: imageItem.height / 2
                        xScale: imageItem.scaleFactor
                        yScale: imageItem.scaleFactor

                        Behavior on xScale {
                            NumberAnimation {duration: Kirigami.Units.longDuration; easing.type: Easing.InOutCubic}
                        }
                        Behavior on yScale {
                            NumberAnimation {duration: Kirigami.Units.longDuration; easing.type: Easing.InOutCubic}
                        }
                    }
                ]
            }
        }

        Controls.Control {
            Layout.fillWidth: true
            visible: root.description

            contentItem: Controls.Label {
                Layout.leftMargin: Kirigami.Units.largeSpacing
                wrapMode: Text.WordWrap

                text: root.description

                font.weight: Font.Bold
            }

            background: Rectangle {
                color: Kirigami.Theme.alternateBackgroundColor
            }

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.top
                }
                height: 1
            }
        }
    }

    onClosed: {
        imageItem.scaleFactor = 1;
        imageItem.rotationAngle = 0;
        loader.active = false;
    }
}
