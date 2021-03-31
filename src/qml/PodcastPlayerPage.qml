/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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
    id: podcastPlayerPage
    property QtObject entry

    title: entry.title
    clip: true

    Kirigami.SwipeNavigator {
        anchors.fill: parent
        initialIndex: 1

        Kirigami.Page {
            property var entry: podcastPlayerPage.entry
            Component.onCompleted: audio.entry = entry

            icon.name: "media-playback-start"
            title: "Play"

            ColumnLayout {
                anchors.fill: parent
                Kirigami.Icon {
                    source: Fetcher.image(entry.feed.image)
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.enormous * 3
                    Layout.preferredHeight: Kirigami.Units.iconSizes.enormous * 3
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

        EntryPage {
            entry: podcastPlayerPage.entry
            icon.name: "help-about"
            title: "Info"
        }
    }
    actions.main: Kirigami.Action {
        text: !entry.enclosure ? i18n("Open in Browser") :
            entry.enclosure.status === Enclosure.Downloadable ? i18n("Download") :
            entry.enclosure.status === Enclosure.Downloading ? i18n("Cancel download") :
            i18n("Delete downloaded file")
        icon.name: !entry.enclosure ? "globe" :
            entry.enclosure.status === Enclosure.Downloadable ? "download" :
            entry.enclosure.status === Enclosure.Downloading ? "edit-delete-remove" :
            "delete"
        onTriggered: {
            if(!entry.enclosure) Qt.openUrlExternally(entry.link)
            else if(entry.enclosure.status === Enclosure.Downloadable) entry.enclosure.download()
            else if(entry.enclosure.status === Enclosure.Downloading) entry.enclosure.cancelDownload()
            else entry.enclosure.deleteFile()
        }
    }
}
