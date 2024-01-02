/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 * SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kmediasession

import org.kde.kasts

import ".."

Item {
    property int miniplayerheight: Kirigami.Units.gridUnit * 3
    property int progressbarheight: Kirigami.Units.gridUnit / 6
    property int buttonsize: Kirigami.Units.gridUnit * 1.5
    height: miniplayerheight + progressbarheight

    visible: AudioManager.entry

    // progress bar for limited width (phones)
    Rectangle {
        id: miniprogressbar
        z: 1
        anchors.top: parent.top
        anchors.left: parent.left
        height: parent.progressbarheight
        color: Kirigami.Theme.highlightColor
        width: parent.width * AudioManager.position / AudioManager.duration
        visible: true
    }

    ChapterModel {
        id: chapterModel
        entry: AudioManager.entry ?? undefined
    }

    RowLayout {
        id: footerrowlayout
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true

            // press feedback
            color: (trackClick.pressed || trackClick.containsMouse) ? Qt.rgba(0, 0, 0, 0.05) : "transparent"

            RowLayout {
                anchors.fill: parent

                ImageWithFallback {
                    imageSource: AudioManager.entry ? ((chapterModel.currentChapter && chapterModel.currentChapter !== undefined) ? chapterModel.currentChapter.cachedImage : AudioManager.entry.cachedImage) : "no-image"
                    Layout.fillHeight: true
                    Layout.preferredWidth: height
                }

                // track information
                ColumnLayout {
                    Layout.maximumHeight: parent.height
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.smallSpacing
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        id: mainLabel
                        text: AudioManager.entry.title
                        wrapMode: Text.Wrap
                        Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1
                        font.weight: Font.Medium
                    }

                    Controls.Label {
                        id: feedLabel
                        text: AudioManager.entry.feed.name
                        wrapMode: Text.Wrap
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        opacity: 0.6
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1
                    }
                }
            }
            MouseArea {
                id: trackClick
                anchors.fill: parent
                hoverEnabled: true
                onClicked: toOpen.restart()
            }
        }
        Controls.Button {
            id: playButton
            icon.name: AudioManager.playbackState === KMediaSession.PlayingState ? "media-playback-pause" : "media-playback-start"
            icon.height: parent.parent.buttonsize
            icon.width: parent.parent.buttonsize
            flat: true
            Layout.preferredHeight: parent.parent.miniplayerheight - Kirigami.Units.smallSpacing * 2
            Layout.preferredWidth: height
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            onClicked: AudioManager.playbackState === KMediaSession.PlayingState ? AudioManager.pause() : AudioManager.play()
            Layout.alignment: Qt.AlignVCenter
        }
    }
}

