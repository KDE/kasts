/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
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

    Component {
        id: slider
        Controls.Slider {
            enabled: AudioManager.entry
            padding: 0
            from: 0
            to: AudioManager.duration / 1000
            value: AudioManager.position / 1000
            onMoved: AudioManager.seek(value * 1000)
        }
    }

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
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        spacing: 0

        Controls.Button {
            id: swipeUpButton
            property int swipeUpButtonSize: Kirigami.Units.iconSizes.smallMedium
            icon.name: "arrow-down"
            icon.height: swipeUpButtonSize
            icon.width: swipeUpButtonSize
            flat: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 0
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            onClicked: toClose.restart()
        }

        Controls.SwipeView {
            id: swipeView

            currentIndex: 0
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height - media.height - swipeUpButton.height
            Layout.margins: 0

            // we are unable to use Controls.Control here to set padding since it seems to eat touch events on the parent flickable
            Item {
                Item {
                    property int textMargin: Kirigami.Units.largeSpacing // margin above the text below the image
                    anchors.fill: parent
                    anchors.leftMargin: Kirigami.Units.largeSpacing * 2
                    anchors.rightMargin: Kirigami.Units.largeSpacing * 2

                    Item {
                        id: coverImage
                        anchors {
                            top: parent.top
                            bottom: root.isWidescreen ? parent.bottom : undefined
                            left: parent.left
                            right: root.isWidescreen ? undefined : parent.right
                            margins: 0
                            topMargin: root.isWidescreen ? 0 : (parent.height - Math.min(height, width) - imageLabels.implicitHeight - 2 * parent.textMargin) / 2
                        }
                        height: Math.min(parent.height - (root.isWidescreen ? 0 : imageLabels.implicitHeight + 2 * parent.textMargin), parent.width)
                        width: root.isWidescreen ? Math.min(parent.height, parent.width / 2) : Math.min(parent.width, height)

                        ImageWithFallback {
                            imageSource: AudioManager.entry ? ((chapterModel.currentChapter && chapterModel.currentChapter !== undefined) ? chapterModel.currentChapter.cachedImage : AudioManager.entry.cachedImage) : "no-image"
                            imageFillMode: Image.PreserveAspectCrop
                            anchors.centerIn: parent
                            anchors.margins: 0
                            width: Math.min(parent.width, parent.height)
                            height: Math.min(parent.height, parent.width)
                            fractionalRadius: 1 / 20
                        }
                    }

                    Item {
                        anchors {
                            top: root.isWidescreen ? parent.top : coverImage.bottom
                            bottom: parent.bottom
                            left: root.isWidescreen ? coverImage.right : parent.left
                            right: parent.right
                            leftMargin: root.isWidescreen ? parent.textMargin : 0
                            topMargin: root.isWidescreen ? 0 : parent.textMargin
                            bottomMargin: 0
                        }

                        ColumnLayout {
                            id: imageLabels
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: 0
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
                            id: chapterModel
                            entry: AudioManager.entry ? AudioManager.entry : undefined
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

        Item {
            id: media

            implicitHeight: mediaControls.height
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2
            Layout.bottomMargin: 0
            Layout.topMargin: 0
            Layout.fillWidth: true

            ColumnLayout {
                id: mediaControls

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 0
                spacing: 0

                RowLayout {
                    id: contextButtons
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    property int iconSize: Math.floor(Kirigami.Units.gridUnit * 1.3)
                    property int buttonSize: bottomRow.buttonSize

                    Controls.ToolButton {
                        visible: AudioManager.entry
                        Layout.maximumHeight: parent.height
                        Layout.preferredHeight: contextButtons.buttonSize
                        Layout.maximumWidth: height
                        Layout.preferredWidth: height
                        icon.name: "viewimage"
                        icon.width: contextButtons.iconSize
                        icon.height: contextButtons.iconSize
                        onClicked: {
                            swipeView.currentIndex = 0;
                        }
                    }
                    Controls.ToolButton {
                        visible: AudioManager.entry
                        Layout.maximumHeight: parent.height
                        Layout.preferredHeight: contextButtons.buttonSize
                        Layout.maximumWidth: height
                        Layout.preferredWidth: height
                        icon.name: "documentinfo"
                        icon.width: contextButtons.iconSize
                        icon.height: contextButtons.iconSize
                        onClicked: {
                            swipeView.currentIndex = 1;
                        }
                    }
                    Controls.ToolButton {
                        visible: AudioManager.entry && chapterList.count !== 0
                        Layout.maximumHeight: parent.height
                        Layout.preferredHeight: contextButtons.buttonSize
                        Layout.maximumWidth: height
                        Layout.preferredWidth: height
                        icon.name: "view-media-playlist"
                        icon.width: contextButtons.iconSize
                        icon.height: contextButtons.iconSize
                        onClicked: {
                            swipeView.currentIndex = 2;
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Controls.ToolButton {
                        checkable: true
                        checked: AudioManager.remainingSleepTime > 0
                        Layout.maximumHeight: parent.height
                        Layout.preferredHeight: contextButtons.buttonSize
                        Layout.maximumWidth: height
                        Layout.preferredWidth: height
                        icon.name: "clock"
                        icon.width: contextButtons.iconSize
                        icon.height: contextButtons.iconSize
                        onClicked: {
                            toggle(); // only set the on/off state based on sleep timer state
                            sleepTimerDialog.open()
                        }
                    }
                }

                Loader {
                    active: !root.isWidescreen
                    visible: !root.isWidescreen
                    sourceComponent: slider
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
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
                    Loader {
                        active: root.isWidescreen
                        sourceComponent: slider
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
                            onClicked: {
                                SettingsManager.toggleRemainingTime = !SettingsManager.toggleRemainingTime;
                                SettingsManager.save();
                            }
                        }
                    }
                }

                RowLayout {
                    id: bottomRow
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.maximumWidth: Number.POSITIVE_INFINITY //TODO ?
                    Layout.fillWidth: true
                    Layout.bottomMargin: 0
                    Layout.topMargin: 0

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
