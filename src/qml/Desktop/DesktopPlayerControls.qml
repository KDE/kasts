/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kmediasession
import org.kde.kasts

import ".."

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

            onPressed: mouse => {
                dragStartOffset = mouse.y;
            }

            onPositionChanged: mouse => {
                desktopPlayerControls.handlePositionChanged(mouse.y, dragStartOffset);
            }

            drag.axis: Drag.YAxis
            drag.threshold: 1
        }
    }

    RowLayout {
        id: playerControlToolBar

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

        property bool tooNarrowChapter: playerControlToolBar.width - (audioButtons.width + 4 * volumeButton.width + (chapterAction.visible ? chapterTextMetric.width : 0)) < audioSliderNiceMinimumWidth + 20

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
                    cursorShape: AudioManager.entryuid > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: AudioManager.entryuid > 0
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
                Controls.ToolTip.text: KI18n.i18n("Seek backward")
            }
            Controls.ToolButton {
                id: playButton
                icon.name: AudioManager.playbackState === KMediaSession.PlayingState ? "media-playback-pause" : "media-playback-start"
                onClicked: AudioManager.playbackState === KMediaSession.PlayingState ? AudioManager.pause() : AudioManager.play()
                enabled: AudioManager.canPlay

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: AudioManager.playbackState === KMediaSession.PlayingState ? KI18n.i18n("Pause") : KI18n.i18n("Play")
            }
            Controls.ToolButton {
                icon.name: "media-seek-forward"
                onClicked: AudioManager.skipForward()
                enabled: AudioManager.canSkipForward

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18n("Seek forward")
            }
            Controls.ToolButton {
                icon.name: "media-skip-forward"
                onClicked: AudioManager.next()
                enabled: AudioManager.canGoNext

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18n("Skip forward")
            }
            Controls.ToolButton {
                id: playbackRateButton
                text: AudioManager.playbackRate.toFixed(2) + "x"
                checkable: true
                checked: playbackRateMenu.visible
                onClicked: {
                    if (playbackRateMenu.visible) {
                        playbackRateMenu.dismiss();
                    } else {
                        playbackRateMenu.popup(playbackRateButton, 0, playbackRateButton.height);
                    }
                }
                padding: 0
                implicitWidth: playButton.width * 1.5
                implicitHeight: playButton.height

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18n("Playback rate:") + " " + AudioManager.playbackRate.toFixed(2) + "x"
            }
        }

        Controls.Label {
            id: currentPositionLabel
            text: AudioManager.formattedPosition
            visible: !playerControlToolBar.tooNarrowAudioLabels
            Layout.alignment: Qt.AlignVCenter
        }

        ChapterSlider {
            id: durationSlider
            model: chapterModel
            enabled: AudioManager.canPlay
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            id: durationLabel
            visible: !playerControlToolBar.tooNarrowAudioLabels
            Layout.preferredHeight: endLabel.implicitHeight
            Layout.preferredWidth: endLabel.implicitWidth
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.alignment: Qt.AlignVCenter

            Controls.Label {
                id: endLabel
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: SettingsManager.toggleRemainingTime ? "-" + AudioManager.formattedLeftDuration : AudioManager.formattedDuration
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

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18nc("@action:button", "Show chapter list")
            }

            Controls.ToolButton {
                id: infoButton
                action: infoAction
                display: playerControlToolBar.tooNarrowExtra ? Controls.AbstractButton.IconOnly : Controls.AbstractButton.TextBesideIcon
                visible: infoAction.visible && !playerControlToolBar.tooNarrowOverflow

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18nc("@action:button", "Show episode info")
            }

            Controls.ToolButton {
                id: sleepButton
                action: sleepAction
                display: playerControlToolBar.tooNarrowExtra ? Controls.AbstractButton.IconOnly : Controls.AbstractButton.TextBesideIcon
                visible: !playerControlToolBar.tooNarrowOverflow

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18nc("@action:button", "Open sleep timer settings")
            }
        }

        RowLayout {
            id: volumeControls
            Controls.ToolButton {
                id: volumeButton
                icon.name: AudioManager.muted ? "audio-volume-muted" : (volumeSlider.value > 66 ? "audio-volume-high" : (volumeSlider.value > 33 ? "audio-volume-medium" : "audio-volume-low"))
                enabled: AudioManager.canPlay
                checked: volumePopup.visible

                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                Controls.ToolTip.text: KI18n.i18nc("@action:button", "Open volume settings")

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
                        VolumeSlider {
                            id: volumeSlider
                        }

                        Controls.ToolButton {
                            id: volumeButtonVertical
                            enabled: AudioManager.canPlay
                            icon.name: AudioManager.muted ? "audio-volume-muted" : (volumeSlider.value > 66 ? "audio-volume-high" : (volumeSlider.value > 33 ? "audio-volume-medium" : "audio-volume-low"))
                            onClicked: AudioManager.muted = !AudioManager.muted

                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                            Controls.ToolTip.text: KI18n.i18nc("@action:button", "Toggle mute")
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

            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            Controls.ToolTip.text: KI18n.i18nc("@action:button", "Show more")

            onClicked: {
                if (overflowMenu.visible) {
                    overflowMenu.dismiss();
                } else {
                    overflowMenu.popup(0, overflowButton.height);
                }
            }

            Controls.Menu {
                id: overflowMenu
                contentData: desktopPlayerControls.extraActions
                onVisibleChanged: {
                    if (visible) {
                        for (var i in contentData) {
                            overflowMenu.contentData[i].visible = overflowMenu.contentData[i].action.visible;
                            overflowMenu.contentData[i].height = (overflowMenu.contentData[i].action.visible) ? overflowMenu.contentData[i].implicitHeight : 0; // workaround for qqc2-breeze-style
                        }
                    }
                }
            }
        }
    }

    // Actions which will be used to create buttons on toolbar or in overflow menu
    Kirigami.Action {
        id: chapterAction
        property bool visible: AudioManager.entryuid > 0 && chapterList.count !== 0
        text: KI18n.i18nc("@action:button", "Chapters")
        icon.name: "view-media-playlist"
        onTriggered: chapterOverlay.open()
    }

    Kirigami.Action {
        id: infoAction
        property bool visible: AudioManager.entryuid > 0
        text: KI18n.i18nc("@action:button", "Show Info")
        icon.name: "documentinfo"
        onTriggered: entryDetailsOverlay.open()
    }

    Kirigami.Action {
        id: sleepAction
        checkable: true
        checked: AudioManager.remainingSleepTime > 0
        property bool visible: true
        text: KI18n.i18nc("@action:button", "Sleep Timer")
        icon.name: "clock"
        onTriggered: {
            toggle(); // only set the on/off state based on sleep timer state
            (Qt.createComponent("org.kde.kasts", "SleepTimerDialog").createObject(Controls.Overlay.overlay) as SleepTimerDialog).open();
        }
    }

    property var extraActions: [chapterAction, infoAction, sleepAction]

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
        entryuid: AudioManager.entryuid
        duration: AudioManager.duration
    }

    Kirigami.Dialog {
        id: chapterOverlay
        preferredWidth: Kirigami.Units.gridUnit * 30
        preferredHeight: Kirigami.Units.gridUnit * 25

        showCloseButton: false

        title: KI18n.i18n("Chapters")

        ListView {
            id: chapterList

            currentIndex: -1

            model: chapterModel
            delegate: ChapterListDelegate {
                overlay: chapterOverlay
            }
        }
    }

    Kirigami.Dialog {
        id: entryDetailsOverlay
        preferredWidth: Kirigami.Units.gridUnit * 30

        showCloseButton: false

        title: (AudioManager.entryuid > 0 && AudioManager.entry) ? AudioManager.entry.title : KI18n.i18n("No Episode Title")
        padding: Kirigami.Units.largeSpacing

        Controls.Label {
            id: text
            text: (AudioManager.entryuid > 0 && AudioManager.entry) ? AudioManager.entry.adjustedContent(width, font.pixelSize) : KI18n.i18n("No episode loaded")
            verticalAlignment: Text.AlignTop
            baseUrl: (AudioManager.entryuid > 0 && AudioManager.entry) ? AudioManager.entry.baseUrl : ""
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            onLinkHovered: {
                cursorShape: Qt.PointingHandCursor;
            }
            onLinkActivated: link => {
                if (link.split("://")[0] === "timestamp") {
                    if (AudioManager.entryuid > 0 && AudioManager.entry && AudioManager.entry.enclosure) {
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
    }
}
