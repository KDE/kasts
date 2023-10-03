/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kmediasession

import org.kde.kasts

FocusScope {
    id: desktopPlayerControls
    implicitHeight: playerControlToolBar.implicitHeight + Kirigami.Units.largeSpacing * 2

    property alias chapterModel: chapterModel
    /*
     * Emmited when User uses the Item as a handle to resize the layout.
     * y: difference to previous position
     * offset: cursor offset (y coordinate relative to this Item, where dragging
     * begun)
     */
    signal handlePositionChanged(int y, int offset)

    Rectangle {
        id: toolbarBackground
        anchors.fill: parent

        opacity: 0.7

        //set background color
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        color: Kirigami.Theme.backgroundColor

        MouseArea {
          anchors.fill: parent
          property int dragStartOffset: 0

          cursorShape: Qt.SizeVerCursor

          onPressed: (mouse) => {
            dragStartOffset = mouse.y
          }

          onPositionChanged: (mouse) => {
            desktopPlayerControls.handlePositionChanged(mouse.y, dragStartOffset)
          }

          drag.axis: Drag.YAxis
          drag.threshold: 1
        }
    }

    RowLayout {
        id: playerControlToolBar
        property int iconSize: Kirigami.Units.gridUnit

        anchors.fill: parent
        anchors.topMargin: Kirigami.Units.largeSpacing
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing
        anchors.leftMargin: Kirigami.Units.smallSpacing

        property int audioSliderNiceMinimumWidth: 300
        property int audioSliderAbsoluteMinimumWidth: 200

        // size of volumeButton serves as size of extra buttons too
        // this is chosen because the volumeButton is always visible
        property bool tooNarrowExtra: playerControlToolBar.width - (audioButtons.width + 4 * volumeButton.width + (chapterAction.visible ? chapterTextMetric.width : 0) + extraButtonsTextMetric.width) < audioSliderNiceMinimumWidth + 40

        property bool tooNarrowChapter: playerControlToolBar.width - (audioButtons.width + 4 * volumeButton.width + (chapterAction.visible ? chapterTextMetric.width : 0)) < audioSliderNiceMinimumWidth +  20

        property bool tooNarrowOverflow: playerControlToolBar.width - (audioButtons.width + 4 * volumeButton.width) < audioSliderNiceMinimumWidth

        property bool tooNarrowAudioLabels: playerControlToolBar.width - (audioButtons.width + 2 * volumeButton.width) < audioSliderAbsoluteMinimumWidth

        clip: true

        Loader {
            Layout.fillHeight: true
            Layout.preferredWidth: height
            active: headerBar.handlePosition === 0
            visible: active
            sourceComponent: imageComponent
        }

        Component {
            id: imageComponent
            ImageWithFallback {
                id: frontImage
                imageSource: headerMetaData.image
                absoluteRadius: Kirigami.Units.smallSpacing
                visible: headerBar.handlePosition === 0
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        headerBar.openFullScreenImage();
                    }
                }
            }
        }

        RowLayout {
            id: audioButtons
            Controls.ToolButton {
                icon.name: "media-seek-backward"
                onClicked: AudioManager.skipBackward()
                enabled: AudioManager.canSkipBackward

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18n("Seek backward")
            }
            Controls.ToolButton {
                id: playButton
                icon.name: AudioManager.playbackState === KMediaSession.PlayingState ? "media-playback-pause" : "media-playback-start"
                onClicked: AudioManager.playbackState === KMediaSession.PlayingState ? AudioManager.pause() : AudioManager.play()
                enabled: AudioManager.canPlay

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: AudioManager.playbackState === KMediaSession.PlayingState ? i18n("Pause") : i18n("Play")
            }
            Controls.ToolButton {
                icon.name: "media-seek-forward"
                onClicked: AudioManager.skipForward()
                enabled: AudioManager.canSkipForward

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18n("Seek forward")
            }
            Controls.ToolButton {
                icon.name: "media-skip-forward"
                onClicked: AudioManager.next()
                enabled: AudioManager.canGoNext

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18n("Skip forward")
            }
            Controls.ToolButton {
                id: playbackRateButton
                contentItem: Controls.Label {
                    text: AudioManager.playbackRate.toFixed(2) + "x"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                checkable: true
                checked: playbackRateMenu.visible
                onClicked: {
                    if (playbackRateMenu.visible) {
                        playbackRateMenu.dismiss();
                    } else {
                        playbackRateMenu.popup(playbackRateButton, 0, playbackRateButton.height);
                    }
                }
                Layout.alignment: Qt.AlignHCenter
                padding: 0
                implicitWidth: playButton.width * 1.5
                implicitHeight: playButton.height

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18n("Playback rate:") + " " + AudioManager.playbackRate.toFixed(2) + "x"
            }
        }

        Controls.Label {
            id: currentPositionLabel
            text: AudioManager.formattedPosition
            visible: !playerControlToolBar.tooNarrowAudioLabels
        }

        Controls.Slider {
            id: durationSlider
            enabled: AudioManager.entry && AudioManager.PlaybackState != AudioManager.StoppedState && AudioManager.canPlay
            Layout.fillWidth: true
            padding: 0
            from: 0
            to: AudioManager.duration / 1000
            value: AudioManager.position / 1000
            onMoved: AudioManager.seek(value * 1000)
            handle.implicitWidth: implicitHeight // workaround to make slider handle position itself exactly at the location of the click
        }

        Item {
            id: durationLabel
            visible: !playerControlToolBar.tooNarrowAudioLabels
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

        RowLayout {
            id: extraButtons
            visible: !playerControlToolBar.tooNarrowOverflow
            Controls.ToolButton {
                id: chapterButton
                action: chapterAction
                display: playerControlToolBar.tooNarrowChapter ? Controls.AbstractButton.IconOnly : Controls.AbstractButton.TextBesideIcon
                visible: chapterAction.visible && !playerControlToolBar.tooNarrowOverflow

                Controls.ToolTip.visible: parent.hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Show chapter list")
            }

            Controls.ToolButton {
                id: infoButton
                action: infoAction
                display: playerControlToolBar.tooNarrowExtra ? Controls.AbstractButton.IconOnly : Controls.AbstractButton.TextBesideIcon
                visible: infoAction.visible && !playerControlToolBar.tooNarrowOverflow

                Controls.ToolTip.visible: parent.hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Show episode info")
            }

            Controls.ToolButton {
                id: sleepButton
                action: sleepAction
                display: playerControlToolBar.tooNarrowExtra ? Controls.AbstractButton.IconOnly : Controls.AbstractButton.TextBesideIcon
                visible:  !playerControlToolBar.tooNarrowOverflow

                Controls.ToolTip.visible: parent.hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Open sleep timer settings")
            }
        }

        RowLayout {
            id: volumeControls
            Controls.ToolButton {
                id: volumeButton
                icon.name: AudioManager.muted ? "player-volume-muted" : "player-volume"
                enabled: AudioManager.PlaybackState != AudioManager.StoppedState && AudioManager.canPlay
                checked: volumePopup.visible

                Controls.ToolTip.visible: parent.hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: i18nc("@action:button", "Open volume settings")

                onClicked: {
                    if (volumePopup.visible) {
                        volumePopup.close();
                    } else {
                        volumePopup.open();
                    }
                }

                Controls.Popup {
                    id: volumePopup
                    x: -padding
                    y: volumeButton.height

                    focus: true
                    padding: Kirigami.Units.smallSpacing
                    contentWidth: volumeButtonVertical.implicitWidth

                    contentItem: ColumnLayout {
                        Controls.Slider {
                            id: volumeSliderVertical
                            height: Kirigami.Units.gridUnit * 7
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredHeight: height
                            Layout.maximumHeight: height
                            Layout.topMargin: Kirigami.Units.smallSpacing
                            orientation: Qt.Vertical
                            padding: 0
                            enabled: !AudioManager.muted && AudioManager.PlaybackState != AudioManager.StoppedState && AudioManager.canPlay
                            from: 0
                            to: 100
                            value: AudioManager.volume
                            onMoved: AudioManager.volume = value

                            onPressedChanged: {
                                tooltip.delay = pressed ? 0 : Kirigami.Units.toolTipDelay
                            }

                            readonly property int wheelEffect: 5

                            Controls.ToolTip {
                                id: tooltip
                                x: volumeSliderVertical.x - volumeSliderVertical.width / 2 - width
                                y: volumeSliderVertical.visualPosition * volumeSliderVertical.height - height / 2
                                visible: volumeSliderVertical.pressed || sliderMouseArea.containsMouse
                                // delay is actually handled in volumeSliderVertical.onPressedChanged, because property bindings aren't immediate
                                delay: volumeSliderVertical.pressed ? 0 : Kirigami.Units.toolTipDelay
                                closePolicy: Controls.Popup.NoAutoClose
                                timeout: -1
                                text: i18nc("Volume as a percentage", "%1%", Math.round(volumeSliderVertical.value))
                            }

                            MouseArea {
                                id: sliderMouseArea
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                hoverEnabled: true
                                onWheel: (wheel) => {
                                    // Can't use Slider's built-in increase() and decrease() functions here
                                    // since they go in increments of 0.1 when the slider's stepSize is not
                                    // defined, which is much too slow. And we don't define a stepSize for
                                    // the slider because if we do, it gets gets tickmarks which look ugly.
                                    if (wheel.angleDelta.y > 0) {
                                        // Increase volume
                                        volumeSliderVertical.value = Math.min(volumeSliderVertical.to,
                                            volumeSliderVertical.value + volumeSliderVertical.wheelEffect);
                                        AudioManager.volume = volumeSliderVertical.value

                                    } else {
                                        // Decrease volume
                                        volumeSliderVertical.value = Math.max(volumeSliderVertical.from,
                                            volumeSliderVertical.value - volumeSliderVertical.wheelEffect);
                                        AudioManager.volume = volumeSliderVertical.value
                                    }
                                }
                            }
                        }

                        Controls.ToolButton {
                            id: volumeButtonVertical
                            enabled: AudioManager.PlaybackState != AudioManager.StoppedState && AudioManager.canPlay
                            icon.name: AudioManager.muted ? "player-volume-muted" : "player-volume"
                            onClicked: AudioManager.muted = !AudioManager.muted

                            Controls.ToolTip.visible: parent.hovered
                            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                            Controls.ToolTip.text: i18nc("@action:button", "Toggle mute")

                        }
                    }
                }
            }
        }

        Controls.ToolButton {
            id: overflowButton
            icon.name: "overflow-menu"
            display: Controls.AbstractButton.IconOnly
            visible: playerControlToolBar.tooNarrowOverflow
            checked: overflowMenu.visible

            Controls.ToolTip.visible: parent.hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            Controls.ToolTip.text: i18nc("@action:button", "Show more")

            onClicked: {
                if (overflowMenu.visible) {
                    overflowMenu.dismiss();
                } else {
                    overflowMenu.popup(0, overflowButton.height)
                }
            }

            Controls.Menu {
                id: overflowMenu
                contentData: extraActions
                onVisibleChanged: {
                    if (visible) {
                        for (var i in contentData) {
                            overflowMenu.contentData[i].visible = overflowMenu.contentData[i].action.visible;
                            overflowMenu.contentData[i].height =
                            (overflowMenu.contentData[i].action.visible) ? overflowMenu.contentData[i].implicitHeight : 0 // workaround for qqc2-breeze-style
                        }
                    }
                }
            }
        }
    }

    // Actions which will be used to create buttons on toolbar or in overflow menu
    Kirigami.Action {
        id: chapterAction
        property bool visible: AudioManager.entry && chapterList.count !== 0
        text: i18nc("@action:button", "Chapters")
        icon.name: "view-media-playlist"
        onTriggered: chapterOverlay.open();
    }

    Kirigami.Action {
        id: infoAction
        property bool visible: AudioManager.entry
        text: i18nc("@action:button", "Show Info")
        icon.name: "documentinfo"
        onTriggered: entryDetailsOverlay.open();
    }

    Kirigami.Action {
        id: sleepAction
        checkable: true
        checked: AudioManager.remainingSleepTime > 0
        property bool visible: true
        text: i18nc("@action:button", "Sleep Timer")
        icon.name: "clock"
        onTriggered: {
            toggle(); // only set the on/off state based on sleep timer state
            sleepTimerDialog.open();
        }
    }

    property var extraActions: [ chapterAction,
                                 infoAction,
                                 sleepAction ]

    TextMetrics {
        id: chapterTextMetric
        text: chapterAction.text
    }

    TextMetrics {
        id: extraButtonsTextMetric
        text: infoAction.text + sleepAction.text
    }

    ChapterModel {
        id: chapterModel
        entry: AudioManager.entry ? AudioManager.entry : null
    }

    Kirigami.Dialog {
        id: chapterOverlay

        showCloseButton: false

        title: i18n("Chapters")

        ListView {
            id: chapterList

            currentIndex: -1

            implicitWidth: Kirigami.Units.gridUnit * 30
            implicitHeight: Kirigami.Units.gridUnit * 25

            model: chapterModel
            delegate: ChapterListDelegate {
                id: chapterDelegate
                width: chapterList.width
                entry: AudioManager.entry ? AudioManager.entry : null
                overlay: chapterOverlay
            }
        }
    }

    Kirigami.Dialog {
        id: entryDetailsOverlay
        preferredWidth: Kirigami.Units.gridUnit * 30

        showCloseButton: false

        title: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Title")
        padding: Kirigami.Units.largeSpacing

        Controls.Label {
            id: text
            text: AudioManager.entry ? AudioManager.entry.adjustedContent(width, font.pixelSize) : i18n("No track loaded")
            verticalAlignment: Text.AlignTop
            baseUrl: AudioManager.entry ? AudioManager.entry.baseUrl : ""
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            onLinkHovered: {
                cursorShape: Qt.PointingHandCursor;
            }
            onLinkActivated: (link) => {
                if (link.split("://")[0] === "timestamp") {
                    if (AudioManager.entry && AudioManager.entry.enclosure) {
                        AudioManager.seek(link.split("://")[1]);
                    }
                } else {
                    Qt.openUrlExternally(link);
                }
            }
        }
    }

    PlaybackRateMenu {
        id: playbackRateMenu
        parentButton: playbackRateButton
    }
}
