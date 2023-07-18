/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import QtMultimedia 5.15
import Qt.labs.platform 1.1

import org.kde.kirigami 2.15 as Kirigami

import org.kde.kmediasession 1.0

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Example KMediaSession Player")

    KMediaSession {
        id: audio
        playerName: "Kasts"
        desktopEntryName: "org.kde.kasts"

        onRaiseWindowRequested: {
            root.visible = true;
            root.show();
            root.raise();
            root.requestActivate();
        }
        onQuitRequested: {
            root.close();
        }
        onNextRequested: {
            console.log("next");
        }
        onPreviousRequested: {
            console.log("previous");
        }
    }

    ColumnLayout {
        RowLayout {
            Controls.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: ListModel {
                    id: backendModel
                }
                Component.onCompleted: {
                    // have to use Number because QML doesn't know about enum names
                    for (var index in audio.availableBackends) {
                        backendModel.append({"text": audio.backendName(audio.availableBackends[index]),
                                             "value": Number(audio.availableBackends[index])});

                        if (Number(audio.availableBackends[index]) === audio.currentBackend) {
                            currentIndex = index;
                        }
                    }
                }

                onActivated: {
                    audio.currentBackend = currentValue;
                }
            }
        }
        RowLayout {
            Controls.Button {
                icon.name: "document-open-folder"
                text: i18n("Select File…")
                onClicked: filePathDialog.open()
            }

            FileDialog {
                id: filePathDialog
                title: i18n("Select Media File")
                currentFile: audio.source
                onAccepted: {
                    audio.source = filePathDialog.file;
                }
            }
            Controls.Label {
                text: i18n("OR")
            }
            Controls.TextField {
                id: urlField
                Layout.fillWidth: true
                text: audio.source
            }
            Controls.Button {
                text: i18n("Open")
                onClicked: {
                    audio.source = urlField.text;
                }
            }
        }
        RowLayout {
            Controls.Button {
                text: i18n("Play")
                enabled: audio.canPlay
                icon.name: "media-playback-start"
                onClicked: {
                    audio.play()
                }
            }
            Controls.Button {
                text: i18n("Pause")
                enabled: audio.canPause
                icon.name: "media-playback-pause"
                onClicked: {
                    audio.pause()
                }
            }
            Controls.Button {
                text: i18n("Stop")
                icon.name: "media-playback-stop"
                onClicked: {
                    audio.stop()
                }
            }
        }
        RowLayout {
            Controls.Label {
                text: audio.position / 1000
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
            Controls.Slider {
                Layout.fillWidth: true
                Layout.margins: 0
                padding: 0
                from: 0
                to: audio.duration
                value: audio.position
                onMoved: audio.position = value;
                enabled: audio.playbackState !== KMediaSession.StoppedState
            }
            Controls.Label {
                text: audio.duration / 1000
                Layout.preferredWidth: 50
            }
        }
        RowLayout {
            Controls.CheckBox {
                text: i18n("Mute")
                enabled: audio.playbackState !== KMediaSession.StoppedState
                checked: audio.muted
                onToggled: audio.muted = checked
            }
            Controls.Slider {
                Layout.fillWidth: true
                Layout.margins: 0
                padding: 0
                enabled: audio.playbackState !== KMediaSession.StoppedState
                from: 0
                to: 100
                value: audio.volume
                onMoved: audio.volume = value;
            }
            Controls.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{text: "0.5x", value: 0.5},
                        {text: "1x", value: 1.0},
                        {text: "1.5x", value: 1.5}]
                Component.onCompleted: currentIndex = indexOfValue(audio.playbackRate)
                onActivated: audio.playbackRate = currentValue
            }
        }
        ColumnLayout {
            Controls.Label {
                text: i18n("Title: %1", audio.metaData.title)
            }
            Controls.Label {
                text: i18n("Artist: %1", audio.metaData.artist)
            }
            Controls.Label {
                text: i18n("Album: %1", audio.metaData.album)
            }
            Image {
                fillMode: Image.PreserveAspectFit
                Layout.preferredHeight: 200
                Layout.preferredWidth: 200
                source: audio.metaData.artworkUrl
            }
        }
        ColumnLayout {
            Controls.Label {
                text: i18n("Media status: %1", audio.mediaStatus)
            }
            Controls.Label {
                text: i18n("Playback status: %1", audio.playbackState)
            }
            Controls.Label {
                text: i18n("Error: %1", audio.error)
            }
        }
    }
}

