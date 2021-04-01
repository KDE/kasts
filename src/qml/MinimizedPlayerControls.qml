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
    width: parent.width
    height: (audio.entry == undefined || audio.playerOpen) ? 0 : (footerrowlayout.height + 3.0 * miniprogressbar.height)
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
        width: parent.width
        anchors.margins: miniprogressbar.height
        anchors.bottom: parent.bottom
        spacing: Kirigami.Units.smallSpacing

        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width - playButton.width
            Layout.maximumWidth: parent.width - playButton.width

            RowLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true

                Kirigami.Icon {
                    source: Fetcher.image(audio.entry.feed.image)
                    Layout.preferredHeight: parent.height
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: Kirigami.Units.smallSpacing
                }

                // track information
                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
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
                onClicked: pageStack.layers.push("qrc:/PlayerControls.qml") // playeroverlay.open()
                //showPassiveNotification("Ping")
            }
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
    }
}


