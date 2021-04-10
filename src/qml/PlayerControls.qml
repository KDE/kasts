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
        Controls.SwipeView {
            id: swipeView

            currentIndex: 0
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height - media.height - indicator.height
            Layout.margins: 0
            Item {
                property int textMargin: Kirigami.Units.gridUnit // margin above and below the text below the image
                Image {
                    id: coverImage
                    asynchronous: true
                    source: audio.entry.image === "" ? "logo.png" : "file://"+Fetcher.image(audio.entry.image)
                    fillMode: Image.PreserveAspectFit
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Math.max(0, parent.height - (height + imageLabels.height + 2*parent.textMargin))/2
                    height: Math.min( Math.min(parent.height, Kirigami.Units.iconSizes.enormous * 3)-(imageLabels.height+2*parent.textMargin),
                                    Math.min(parent.width, Kirigami.Units.iconSizes.enormous * 3))
                    width: height
                }
                ColumnLayout {
                    id: imageLabels
                    anchors.top: coverImage.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: parent.textMargin
                    Controls.Label {
                        text: audio.entry.title
                        elide: Text.ElideRight
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: parent.width
                    }
                    Controls.Label {
                        text: audio.entry.feed.name
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
                    anchors.leftMargin: 25
                    anchors.rightMargin: 25
                    clip: true
                    contentHeight: text.height
                    Controls.Label {
                        width: parent.width
                        id: text
                        text: audio.entry.content
                        verticalAlignment: Text.AlignTop
                        baseUrl: audio.entry.baseUrl
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                        onLinkActivated: Qt.openUrlExternally(link)
                    }
                }
            }
        }

        Controls.PageIndicator {
            id: indicator

            count:swipeView.count
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

                Controls.Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: audio.duration
                    value: audio.position
                    onMoved: audio.seek(value)
                }
                RowLayout {
                    id: controls
                    //Layout.leftMargin: 0
                    //Layout.rightMargin: 0
                    Layout.fillWidth: true
                    Controls.Label {
                        //anchor.left: parent.left
                        text: (Math.floor(audio.position/3600000) < 10 ? "0" : "") + Math.floor(audio.position/3600000) + ":" + (Math.floor(audio.position/60000) % 60 < 10 ? "0" : "") + Math.floor(audio.position/60000) % 60 + ":" + (Math.floor(audio.position/1000) % 60 < 10 ? "0" : "") + Math.floor(audio.position/1000) % 60
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Item {
                        Layout.preferredHeight: endLabel.implicitHeight + Kirigami.Units.gridUnit
                        Layout.preferredWidth: endLabel.implicitWidth + Kirigami.Units.gridUnit
                        Controls.Label {
                            id: endLabel
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: (SettingsManager.showRemainingTime) ?
                                    ((Math.floor((audio.duration-audio.position)/3600000) < 10 ? "-0" : "-") + Math.floor((audio.duration-audio.position)/3600000) + ":" + (Math.floor((audio.duration-audio.position)/60000) % 60 < 10 ? "0" : "") + Math.floor((audio.duration-audio.position)/60000) % 60 + ":" + (Math.floor((audio.duration-audio.position)/1000) % 60 < 10 ? "0" : "") + Math.floor((audio.duration-audio.position)/1000) % 60)
                                    : ((Math.floor(audio.duration/3600000) < 10 ? "0" : "") + Math.floor(audio.duration/3600000) + ":" + (Math.floor(audio.duration/60000) % 60 < 10 ? "0" : "") + Math.floor(audio.duration/60000) % 60 + ":" + (Math.floor(audio.duration/1000) % 60 < 10 ? "0" : "") + Math.floor(audio.duration/1000) % 60)

                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: SettingsManager.showRemainingTime = !SettingsManager.showRemainingTime
                        }
                    }
                }
                RowLayout {
                    Layout.maximumWidth: Number.POSITIVE_INFINITY //TODO ?
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.gridUnit * 2
                    property int buttonsize: Kirigami.Units.gridUnit * 2

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
                        icon.height: parent.buttonsize
                        icon.width: parent.buttonsize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: audio.seek(audio.position - 10000)
                    }
                    Controls.Button {
                        id: playButton
                        icon.name: audio.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
                        icon.height: parent.buttonsize
                        icon.width: parent.buttonsize
                        flat: true
                        onClicked: audio.playbackState === Audio.PlayingState ? audio.pause() : audio.play()
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Controls.Button {
                        icon.name: "media-seek-forward"
                        icon.height: parent.buttonsize
                        icon.width: parent.buttonsize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: audio.seek(audio.position + 10000)
                    }
                    Controls.Button {
                        icon.name: "media-skip-forward"
                        icon.height: parent.buttonsize
                        icon.width: parent.buttonsize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: console.log("TODO")
                    }
                }
            }
        }
    }

}
