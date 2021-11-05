/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 * SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Kirigami.Page {
    id: playerControls

    title: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Loaded")
    clip: true
    Layout.margins: 0

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    background: Image {
        opacity: 0.2
        source: AudioManager.entry.cachedImage
        asynchronous: true

        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop

        layer.enabled: true
        layer.effect: HueSaturation {
            cached: true

            lightness: 0.7
            saturation: 0.9

            layer.enabled: true
            layer.effect: FastBlur {
                cached: true
                radius: 100
                transparentBorder: false
            }
        }
    }

    ColumnLayout {
        id: playerControlsColumn
        anchors.fill: parent
        anchors.topMargin: Kirigami.Units.largeSpacing * 2
        anchors.bottomMargin: Kirigami.Units.largeSpacing * 2

        Controls.Button {
            id: swipeUpButton
            property int swipeUpButtonSize: Kirigami.Units.iconSizes.smallMedium
            icon.name: "arrow-down"
            icon.height: swipeUpButtonSize
            icon.width: swipeUpButtonSize
            flat: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 0
            onClicked: toClose.restart()
        }

        Controls.SwipeView {
            id: swipeView

            currentIndex: 0
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height - media.height - indicator.height - indicator.Layout.bottomMargin - swipeUpButton.height
            Layout.margins: 0

            // we are unable to use Controls.Control here to set padding since it seems to eat touch events on the parent flickable
            Item {
                Item {
                    property int textMargin: Kirigami.Units.gridUnit // margin above and below the text below the image
                    anchors.fill: parent
                    anchors.leftMargin: Kirigami.Units.largeSpacing * 2
                    anchors.rightMargin: Kirigami.Units.largeSpacing * 2

                    ImageWithFallback {
                        id: coverImage
                        imageSource: AudioManager.entry ? AudioManager.entry.cachedImage : "no-image"
                        imageFillMode: Image.PreserveAspectCrop
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: Math.max(0, parent.height - (height + imageLabels.height + 2*parent.textMargin))/2
                        height: Math.min(Math.min(parent.height, Kirigami.Units.iconSizes.enormous * 3) - (imageLabels.height + 2 * parent.textMargin),
                                        Math.min(parent.width, Kirigami.Units.iconSizes.enormous * 3))
                        width: height
                        fractionalRadius: 1 / 20
                    }
                    ColumnLayout {
                        id: imageLabels
                        anchors.top: coverImage.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: parent.textMargin
                        Controls.Label {
                            text: AudioManager.entry ? AudioManager.entry.title : i18n("No Title")
                            elide: Text.ElideRight
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: parent.width
                            font.weight: Font.Medium
                        }
                        Controls.Label {
                            text: AudioManager.entry ? AudioManager.entry.feed.name : i18n("No Podcast Title")
                            elide: Text.ElideRight
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: parent.width
                            opacity: 0.6
                        }
                    }
                }
            }

            Item {
                Flickable {
                    anchors.fill: parent
                    anchors.leftMargin: Kirigami.Units.largeSpacing * 2 + playerControlsColumn.anchors.margins
                    anchors.rightMargin: Kirigami.Units.largeSpacing * 2
                    clip: true
                    contentHeight: description.height
                    ColumnLayout {
                        id: description
                        width: parent.width
                        Kirigami.Heading {
                            text: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Title")
                            level: 3
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            Layout.bottomMargin: Kirigami.Units.largeSpacing
                        }
                        Controls.Label {
                            id: text
                            text: AudioManager.entry ? AudioManager.entry.content : i18n("No Track Loaded")
                            verticalAlignment: Text.AlignTop
                            baseUrl: AudioManager.entry ? AudioManager.entry.baseUrl : ""
                            textFormat: Text.RichText
                            wrapMode: Text.WordWrap
                            onLinkActivated: Qt.openUrlExternally(link)
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Item {
                Item {
                    anchors.fill: parent
                    anchors.leftMargin: Kirigami.Units.largeSpacing * 2
                    anchors.rightMargin: Kirigami.Units.largeSpacing * 2

                    Kirigami.PlaceholderMessage {
                        visible: chapterList.count === 0

                        width: parent.width
                        anchors.centerIn: parent

                        text: i18n("No chapters found.")
                    }
                    ListView {
                        id: chapterList
                        model: ChapterModel {
                            enclosureId: AudioManager.entry.id
                            enclosurePath: AudioManager.entry.enclosure.path
                        }
                        clip: true
                        visible: chapterList.count !== 0
                        anchors.fill: parent
                        delegate: ChapterListDelegate {
                            entry: AudioManager.entry
                        }
                    }
                }
            }
        }

        Controls.PageIndicator {
            id: indicator

            count: swipeView.count
            currentIndex: swipeView.currentIndex
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Kirigami.Units.gridUnit
        }

        Item {
            id: media

            implicitHeight: mediaControls.height
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2
            Layout.fillWidth: true

            ColumnLayout {
                id: mediaControls

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 0

                Controls.Slider {
                    enabled: AudioManager.entry
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    padding: 0
                    from: 0
                    to: AudioManager.duration
                    value: AudioManager.position
                    onMoved: AudioManager.seek(value)
                }
                RowLayout {
                    id: controls
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true
                    Controls.Label {
                        Layout.alignment: Qt.AlignLeft
                        padding: Kirigami.Units.largeSpacing
                        text: AudioManager.formattedPosition
                        font: Kirigami.Theme.smallFont
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Item {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredHeight: endLabel.implicitHeight
                        Layout.preferredWidth: endLabel.implicitWidth
                        Controls.Label {
                            id: endLabel
                            padding: Kirigami.Units.largeSpacing
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: (SettingsManager.toggleRemainingTime) ?
                                    "-" + AudioManager.formattedLeftDuration
                                    : AudioManager.formattedDuration
                            font: Kirigami.Theme.smallFont

                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: SettingsManager.toggleRemainingTime = !SettingsManager.toggleRemainingTime
                        }
                    }
                }

                RowLayout {
                    id: bottomRow
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.maximumWidth: Number.POSITIVE_INFINITY //TODO ?
                    Layout.fillWidth: true
                    Layout.margins: 0
                    Layout.topMargin: Kirigami.Units.largeSpacing

                    // Make button width scale properly on narrow windows instead of overflowing
                    property int buttonSize: Math.min(playButton.implicitWidth, ((playerControlsColumn.width - 4 * spacing) / 5 - playButton.leftPadding - playButton.rightPadding))
                    property int iconSize: Kirigami.Units.gridUnit * 1.5

                    // left section
                    Controls.Button {
                        // Use contentItem and a Label because using plain "text"
                        // does not rescale automatically if the text changes
                        contentItem: Controls.Label {
                            text: AudioManager.playbackRate + "x"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            playbackRateDialog.open()
                        }
                        flat: true
                        padding: 0
                        implicitWidth: playButton.width
                        implicitHeight: playButton.height
                    }

                    // middle section
                    RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        Layout.alignment: Qt.AlignHCenter
                        Controls.Button {
                            icon.name: "media-seek-backward"
                            icon.height: bottomRow.iconSize
                            icon.width: bottomRow.iconSize
                            flat: true
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: bottomRow.buttonSize
                            onClicked: AudioManager.skipBackward()
                            enabled: AudioManager.canSkipBackward
                        }
                        Controls.Button {
                            id: playButton
                            icon.name: AudioManager.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
                            icon.height: bottomRow.iconSize
                            icon.width: bottomRow.iconSize
                            flat: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: bottomRow.buttonSize
                            onClicked: AudioManager.playbackState === Audio.PlayingState ? AudioManager.pause() : AudioManager.play()
                            enabled: AudioManager.canPlay
                        }
                        Controls.Button {
                            icon.name: "media-seek-forward"
                            icon.height: bottomRow.iconSize
                            icon.width: bottomRow.iconSize
                            flat: true
                            Layout.alignment: Qt.AlignLeft
                            Layout.preferredWidth: bottomRow.buttonSize
                            onClicked: AudioManager.skipForward()
                            enabled: AudioManager.canSkipForward
                        }
                    }

                    // right section
                    Controls.Button {
                        icon.name: "media-skip-forward"
                        icon.height: bottomRow.iconSize
                        icon.width: bottomRow.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: bottomRow.buttonSize
                        onClicked: AudioManager.next()
                        enabled: AudioManager.canGoNext
                    }
                }
            }
        }
    }
}
