/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.alligator 1.0

Kirigami.Page {
    id: playercontrols

    title: audio.entry.title
    clip: true
    Layout.margins: 0

    Component.onCompleted: audio.playerOpen = true
    Component.onDestruction: audio.playerOpen = false

    ColumnLayout {
        anchors.fill: parent
        Kirigami.Icon {
            source: Fetcher.image(audio.entry.feed.image)
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.preferredWidth: Math.min(parent.width, Kirigami.Units.iconSizes.enormous * 3)
            Layout.preferredHeight: Math.min(parent.height - 2*controls.height, Kirigami.Units.iconSizes.enormous * 3)
        }
        Item {
            id: media

            implicitHeight: mediaControls.height
            Layout.fillWidth: true
            Layout.margins: 0

            ColumnLayout {
                id: mediaControls

                implicitHeight: controls.height

                anchors.left: parent.left
                anchors.right: parent.right

                RowLayout {
                    id: controls
                    Controls.Label {
                        text: (Math.floor(audio.position/3600000) < 10 ? "0" : "") + Math.floor(audio.position/3600000) + ":" + (Math.floor(audio.position/60000) % 60 < 10 ? "0" : "") + Math.floor(audio.position/60000) % 60 + ":" + (Math.floor(audio.position/1000) % 60 < 10 ? "0" : "") + Math.floor(audio.position/1000) % 60
                        padding: Kirigami.Units.gridUnit
                    }
                    Controls.Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: audio.duration
                        value: audio.position
                        onMoved: audio.seek(value)
                    }
                    Controls.Label {
                        text: (Math.floor(audio.duration/3600000) < 10 ? "0" : "") + Math.floor(audio.duration/3600000) + ":" + (Math.floor(audio.duration/60000) % 60 < 10 ? "0" : "") + Math.floor(audio.duration/60000) % 60 + ":" + (Math.floor(audio.duration/1000) % 60 < 10 ? "0" : "") + Math.floor(audio.duration/1000) % 60
                        padding: Kirigami.Units.gridUnit
                    }
                }
                RowLayout {
                    Layout.maximumWidth: Number.POSITIVE_INFINITY //TODO ?
                    Layout.fillWidth: true

                    Controls.Button {
                        text: audio.playbackRate + "x"
                        onClicked: {
                            if(audio.playbackRate === 2.5)
                                audio.playbackRate = 1
                            else
                                audio.playbackRate = audio.playbackRate + 0.25
                        }
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: playButton.width
                        implicitHeight: playButton.height
                    }
                    Controls.Button {
                        icon.name: "media-seek-backward"
                        icon.height: Kirigami.Units.gridUnit * 2
                        icon.width: Kirigami.Units.gridUnit * 2
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: audio.seek(audio.position - 10000)
                    }
                    Controls.Button {
                        id: playButton
                        icon.name: audio.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
                        icon.height: Kirigami.Units.gridUnit * 2
                        icon.width: Kirigami.Units.gridUnit * 2
                        flat: true
                        onClicked: audio.playbackState === Audio.PlayingState ? audio.pause() : audio.play()
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Controls.Button {
                        icon.name: "media-seek-forward"
                        icon.height: Kirigami.Units.gridUnit * 2
                        icon.width: Kirigami.Units.gridUnit * 2
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: audio.seek(audio.position + 10000)
                    }
                    Controls.Button {
                        icon.name: "media-skip-forward"
                        icon.height: Kirigami.Units.gridUnit * 2
                        icon.width: Kirigami.Units.gridUnit * 2
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: console.log("TODO")
                    }
                }
            }
        }
    }

}
