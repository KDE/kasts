/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
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

    padding: 0
    bottomPadding: Kirigami.Units.gridUnit

    ColumnLayout {
        id: playerControlsColumn
        anchors.fill: parent
        anchors.topMargin:0
        Controls.Button {
            id: swipeUpButton
            property int swipeUpButtonSize: Kirigami.Units.gridUnit * 2
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
            Layout.preferredHeight: parent.height - media.height - indicator.height - swipeUpButton.height
            Layout.margins: 0
            Item {
                property int textMargin: Kirigami.Units.gridUnit // margin above and below the text below the image
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
            Item {
                Flickable {
                    anchors.fill: parent
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
                Kirigami.PlaceholderMessage {
                    visible: chapterList.count === 0

                    width: parent.width
                    anchors.centerIn: parent

                    text: i18n("No chapter marks found.")
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

        Controls.PageIndicator {
            id: indicator

            count: swipeView.count
            currentIndex: swipeView.currentIndex
            Layout.alignment: Qt.AlignHCenter
        }
        Item {
            id: media

            implicitHeight: mediaControls.height
            Layout.fillWidth: true
            Layout.margins: 0

            ColumnLayout {
                id: mediaControls

                //implicitHeight: controls.height

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 0

                Controls.Slider {
                    enabled: AudioManager.entry
                    Layout.fillWidth: true
                    Layout.margins: 0
                    padding: 0
                    from: 0
                    to: AudioManager.duration
                    value: AudioManager.position
                    onMoved: AudioManager.seek(value)
                }
                RowLayout {
                    id: controls
                    Layout.margins: 0
                    Layout.fillWidth: true
                    Controls.Label {
                        padding: Kirigami.Units.largeSpacing
                        text: AudioManager.formattedPosition
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Item {
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

                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: SettingsManager.toggleRemainingTime = !SettingsManager.toggleRemainingTime
                        }
                    }
                }
                RowLayout {
                    Layout.maximumWidth: Number.POSITIVE_INFINITY //TODO ?
                    Layout.fillWidth: true
                    Layout.margins: 0
                    Layout.topMargin: Kirigami.Units.largeSpacing

                    // Make button width scale properly on narrow windows instead of overflowing
                    property int buttonSize: Math.min(playButton.implicitWidth, ((playerControlsColumn.width - 4 * spacing) / 5 - playButton.leftPadding - playButton.rightPadding))
                    property int iconSize: Kirigami.Units.gridUnit * 2

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
                        Layout.alignment: Qt.AlignHCenter
                        padding: 0
                        implicitWidth: playButton.width
                        implicitHeight: playButton.height
                    }
                    Controls.Button {
                        icon.name: "media-seek-backward"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.skipBackward()
                        enabled: AudioManager.canSkipBackward
                    }
                    Controls.Button {
                        id: playButton
                        icon.name: AudioManager.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.playbackState === Audio.PlayingState ? AudioManager.pause() : AudioManager.play()
                        enabled: AudioManager.canPlay
                    }
                    Controls.Button {
                        icon.name: "media-seek-forward"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.skipForward()
                        enabled: AudioManager.canSkipForward
                    }
                    Controls.Button {
                        icon.name: "media-skip-forward"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.next()
                        enabled: AudioManager.canGoNext
                    }
                }
            }
        }
    }
}
