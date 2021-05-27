import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtMultimedia 5.15
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Rectangle {
    id: headerBar
    anchors.fill: parent
    color: Kirigami.Theme.backgroundColor
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Kirigami.Units.largeSpacing
        anchors.verticalCenter: parent.verticalCenter
        ImageWithFallback {
            id: mainImage
            imageSource: AudioManager.entry ? AudioManager.entry.cachedImage : "no-image"
            height: controlsLayout.height
            width: height
            absoluteRadius: 5
            Layout.leftMargin: Kirigami.Units.largeSpacing
        }
        ColumnLayout {
            id: controlsLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            RowLayout {
                Layout.rightMargin: Kirigami.Units.largeSpacing
                ColumnLayout {
                    Kirigami.Heading {
                        text: AudioManager.entry ? AudioManager.entry.title : "No track title"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        level: 2
                        wrapMode: Text.Wrap
                        font.bold: true
                    }
                    Label {
                        text: AudioManager.entry ? AudioManager.entry.feed.name : "No feed"
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        opacity: 0.6
                        Layout.bottomMargin: 10
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                RowLayout {
                    property int iconSize: Kirigami.Units.gridUnit
                    property int buttonSize: playButton.implicitWidth
                    Button {
                        contentItem: Label {
                            text: AudioManager.playbackRate + "x"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if(AudioManager.playbackRate === 2.5)
                                AudioManager.playbackRate = 1
                            else
                                AudioManager.playbackRate = AudioManager.playbackRate + 0.25
                        }
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        padding: 0
                        implicitWidth: playButton.width * 2
                        implicitHeight: playButton.height
                    }
                    Button {
                        icon.name: "media-seek-backward"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.skipBackward()
                        enabled: AudioManager.canSkipBackward
                    }
                    Button {
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
                    Button {
                        icon.name: "media-seek-forward"
                        icon.height: parent.iconSize
                        icon.width: parent.iconSize
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: parent.buttonSize
                        onClicked: AudioManager.skipForward()
                        enabled: AudioManager.canSkipForward
                    }
                    Button {
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
            Slider {
                id: durationSlider
                enabled: AudioManager.entry
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: Kirigami.Units.largeSpacing
                padding: 0
                from: 0
                to: AudioManager.duration
                value: AudioManager.position
                onMoved: AudioManager.seek(value)
            }
        }
    }
}
