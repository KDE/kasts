/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtMultimedia 5.15
import QtGraphicalEffects 1.15
import QtQml.Models 2.15

import org.kde.kirigami 2.19 as Kirigami

import org.kde.kasts 1.0

Rectangle {
    id: headerBar
    implicitHeight: headerRowLayout.implicitHeight
    implicitWidth: headerRowLayout.implicitWidth

    //set background color
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Header
    color: Kirigami.Theme.backgroundColor

    function openEntry() {
        if (AudioManager.entry) {
            pushPage("QueuePage");
            pageStack.push("qrc:/EntryPage.qml", {"entry": AudioManager.entry});
            SettingsManager.lastOpenedPage = "QueuePage";
            SettingsManager.save();
            pageStack.get(0).lastEntry = AudioManager.entry.id;
            var model = pageStack.get(0).queueList.model;
            for (var i = 0; i <  model.rowCount(); i++) {
                var index = model.index(i, 0);
                if (AudioManager.entry == model.data(index, EpisodeModel.EntryRole)) {
                    pageStack.get(0).queueList.currentIndex = i;
                    pageStack.get(0).queueList.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);

                }
            }
        }
    }

    RowLayout {
        id: headerRowLayout
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing
        ImageWithFallback {
            id: mainImage
            imageSource: AudioManager.entry ? AudioManager.entry.cachedImage : "no-image"
            height: controlsLayout.height
            width: height
            absoluteRadius: 5
            Layout.leftMargin: Kirigami.Units.largeSpacing
            MouseArea {
                anchors.fill: mainImage
                onClicked: {
                    headerBar.openEntry();
                }
            }
        }
        ColumnLayout {
            id: controlsLayout
            Layout.fillWidth: true
            Item {
                id: titlesAndButtons
                Layout.fillWidth: true
                height: Math.max(titleLabels.implicitHeight, controlButtons.implicitHeight)
                clip: true
                ColumnLayout {
                    id: titleLabels
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: controlButtons.left
                    Kirigami.Heading {
                        text: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Title")
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        level: 3
                        font.bold: true
                    }
                    Controls.Label {
                        text: AudioManager.entry ? AudioManager.entry.feed.name : i18n("No Track Loaded")
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        opacity: 0.6
                        Layout.bottomMargin: Kirigami.Units.largeSpacing
                    }
                }
                MouseArea {
                    anchors.fill: titleLabels
                    onClicked: {
                        openEntry();
                    }
                }
                RowLayout {
                    id: controlButtons
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: Kirigami.Units.largeSpacing
                    clip: true

                    property int optionalButtonCollapseWidth: Kirigami.Units.gridUnit * 12

                    Controls.ToolButton {
                        id: chapterButton
                        visible: AudioManager.entry && chapterList.count !== 0 && (titlesAndButtons.width > essentialButtons.width + infoButton.implicitWidth + implicitWidth + 2 * infoButton.implicitWidth +  parent.optionalButtonCollapseWidth)
                        text: i18n("Chapters")
                        icon.name: "view-media-playlist"
                        icon.height: essentialButtons.iconSize
                        icon.width: essentialButtons.iconSize
                        Layout.preferredHeight: essentialButtons.buttonSize
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: chapterOverlay.open();
                    }
                    Controls.ToolButton {
                        id: infoButton
                        visible: AudioManager.entry && (titlesAndButtons.width > essentialButtons.width + 2 * implicitWidth + parent.optionalButtonCollapseWidth)
                        icon.name: "help-about-symbolic"
                        icon.height: essentialButtons.iconSize
                        icon.width: essentialButtons.iconSize
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: essentialButtons.buttonSize
                        onClicked: entryDetailsOverlay.open();
                    }
                    Controls.ToolButton {
                        id: sleepButton
                        checkable: true
                        checked: AudioManager.remainingSleepTime > 0
                        visible: titlesAndButtons.width > essentialButtons.width + implicitWidth + parent.optionalButtonCollapseWidth
                        icon.name: "clock"
                        icon.height: essentialButtons.iconSize
                        icon.width: essentialButtons.iconSize
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: essentialButtons.buttonSize
                        onClicked: {
                            toggle(); // only set the on/off state based on sleep timer state
                            sleepTimerDialog.open();
                        }
                    }
                    RowLayout {
                        id: essentialButtons
                        property int iconSize: Kirigami.Units.gridUnit
                        property int buttonSize: playButton.implicitWidth
                        Layout.margins: 0
                        clip: true

                        Controls.ToolButton {
                            contentItem: Controls.Label {
                                text: AudioManager.playbackRate + "x"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: playbackRateDialog.open()
                            Layout.alignment: Qt.AlignHCenter
                            padding: 0
                            implicitWidth: playButton.width * 1.5
                            implicitHeight: playButton.height

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            Controls.ToolTip.text: i18n("Playback Rate: ") + AudioManager.playbackRate + "x"
                        }
                        Controls.ToolButton {
                            icon.name: "media-seek-backward"
                            icon.height: parent.iconSize
                            icon.width: parent.iconSize
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: parent.buttonSize
                            onClicked: AudioManager.skipBackward()
                            enabled: AudioManager.canSkipBackward

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            Controls.ToolTip.text: i18n("Seek Backward")
                        }
                        Controls.ToolButton {
                            id: playButton
                            icon.name: AudioManager.playbackState === Audio.PlayingState ? "media-playback-pause" : "media-playback-start"
                            icon.height: parent.iconSize
                            icon.width: parent.iconSize
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: parent.buttonSize
                            onClicked: AudioManager.playbackState === Audio.PlayingState ? AudioManager.pause() : AudioManager.play()
                            enabled: AudioManager.canPlay

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            Controls.ToolTip.text: AudioManager.playbackState === Audio.PlayingState ? i18n("Pause") : i18n("Play")
                        }
                        Controls.ToolButton {
                            icon.name: "media-seek-forward"
                            icon.height: parent.iconSize
                            icon.width: parent.iconSize
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: parent.buttonSize
                            onClicked: AudioManager.skipForward()
                            enabled: AudioManager.canSkipForward

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            Controls.ToolTip.text: i18n("Seek Forward")
                        }
                        Controls.ToolButton {
                            icon.name: "media-skip-forward"
                            icon.height: parent.iconSize
                            icon.width: parent.iconSize
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: parent.buttonSize
                            onClicked: AudioManager.next()
                            enabled: AudioManager.canGoNext

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            Controls.ToolTip.text: i18n("Skip Forward")
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Controls.Label {
                    text: AudioManager.formattedPosition
                }
                Controls.Slider {
                    id: durationSlider
                    enabled: AudioManager.entry
                    Layout.fillWidth: true
                    padding: 0
                    from: 0
                    to: AudioManager.duration / 1000
                    value: AudioManager.position / 1000
                    onMoved: AudioManager.seek(value * 1000)
                }

                Item {
                    Layout.preferredHeight: endLabel.implicitHeight
                    Layout.preferredWidth: endLabel.implicitWidth
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Controls.Label {
                        id: endLabel
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: (SettingsManager.toggleRemainingTime) ?
                                "-" + AudioManager.formattedLeftDuration
                                : AudioManager.formattedDuration

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
        }
    }

    Kirigami.Dialog {
        id: chapterOverlay
        preferredWidth: Kirigami.Units.gridUnit * 20
        preferredHeight: Kirigami.Units.gridUnit * 16

        showCloseButton: true

        title: i18n("Chapters")

        ListView {
            id: chapterList
            model: ChapterModel {
                enclosureId: AudioManager.entry ? AudioManager.entry.id : ""
                enclosurePath: AudioManager.entry ? AudioManager.entry.enclosure.path : ""
            }
            delegate: ChapterListDelegate {
                id: chapterDelegate
                width: chapterList.width
                entry: AudioManager.entry
                overlay: chapterOverlay
            }
        }
    }

    Kirigami.Dialog {
        id: entryDetailsOverlay
        preferredWidth: Kirigami.Units.gridUnit * 25

        showCloseButton: true

        title: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Title")
        padding: Kirigami.Units.largeSpacing

        Controls.Label {
            id: text
            text: AudioManager.entry ? AudioManager.entry.content : i18n("No Track Loaded")
            verticalAlignment: Text.AlignTop
            baseUrl: AudioManager.entry ? AudioManager.entry.baseUrl : ""
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}
