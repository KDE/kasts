/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import QtMultimedia 5.15

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Item {
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    width: parent.width
    //height: (audio.entry == undefined || audio.playerOpen) ? 0 : Kirigami.Units.gridUnit * 3.5 + (Kirigami.Units.gridUnit / 6)
    height: Kirigami.Units.gridUnit * 3.5 + (Kirigami.Units.gridUnit / 6)
    //margins.bottom: miniprogressbar.height
    visible: (audio.entry !== undefined) && !audio.playerOpen

    // Set background
    Rectangle {
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }

    // progress bar for limited width (phones)
    Rectangle {
        id: miniprogressbar
        z: 1
        anchors.top: parent.top
        anchors.left: parent.left
        height: Kirigami.Units.gridUnit / 6
        color: Kirigami.Theme.highlightColor
        width: parent.width * audio.position / audio.duration
        visible: true
    }

    RowLayout {
        id: footerrowlayout
        anchors.topMargin: miniprogressbar.height
        anchors.fill: parent
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Image {
                    asynchronous: true
                    source: audio.entry.image === "" ? "logo.png" : "file://"+Fetcher.image(audio.entry.image)
                    fillMode: Image.PreserveAspectFit
                    Layout.fillHeight: true
                    Layout.maximumWidth: height
                }

                // track information
                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.smallSpacing
                    Controls.Label {
                        id: mainLabel
                        text: audio.entry.title
                        wrapMode: Text.Wrap
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        font.weight: Font.Bold
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1
                    }

                    Controls.Label {
                        id: feedLabel
                        text: audio.entry.feed.name
                        wrapMode: Text.Wrap
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1
                    }
                }
            }
            MouseArea {
                id: trackClick
                anchors.fill: parent
                hoverEnabled: true
                onClicked: pageStack.layers.push("qrc:/PlayerControls.qml")
            }
        }
        Controls.Button {
            id: playButton
            icon.name: audio.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
            icon.height: Kirigami.Units.gridUnit * 2.5
            icon.width: Kirigami.Units.gridUnit * 2.5
            flat: true
            Layout.fillHeight: true
            Layout.maximumHeight: Kirigami.Units.gridUnit *3.5
            Layout.maximumWidth: height
            onClicked: audio.playbackState === Audio.PlayingState ? audio.pause() : audio.play()
            Layout.alignment: Qt.AlignVCenter
        }
    }
}


