/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQml.Models
import QtCore

import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.kasts

FocusScope {
    id: root
    height: headerMetaData.implicitHeight + desktopPlayerControls.implicitHeight

    property int handlePosition: settings.headerSize
    property int maximumHeight: Kirigami.Units.gridUnit * 8
    property int minimumImageSize: Kirigami.Units.gridUnit * 1.5
    property int subtitleCollapseHeight: Kirigami.Units.gridUnit * 2.5
    property int authorCollapseHeight: Kirigami.Units.gridUnit * 4
    property int disappearHeight: Kirigami.Units.gridUnit * 1.0

    property string image: AudioManager.entry ? ((root.desktopPlayerControls.chapterModel.currentChapter && root.desktopPlayerControls.chapterModel.currentChapter !== undefined) ? root.desktopPlayerControls.chapterModel.currentChapter.cachedImage : AudioManager.entry.cachedImage) : "no-image"
    property string blurredImage: AudioManager.entry ? AudioManager.entry.cachedImage : "no-image"
    property string title: AudioManager.entry ? AudioManager.entry.title : KI18n.i18n("No Episode Title")

    property Item headerMetaData: _headerMetaData
    property DesktopPlayerControls desktopPlayerControls: _desktopPlayerControls

    function openEntry(): void {
        if (AudioManager.entryuid > 0) {
            var mainWindow = root.Controls.ApplicationWindow.window as Main;
            mainWindow.pushPage("QueuePage");
            mainWindow.pageStack.get(0).lastEntry = AudioManager.entryuid;
            var model = mainWindow.pageStack.get(0).queueList.model;
            for (var i = 0; i < model.rowCount(); i++) {
                var index = model.index(i, 0);
                if (AudioManager.entryuid == model.data(index, AbstractEpisodeModel.EntryuidRole)) {
                    mainWindow.pageStack.get(0).queueList.currentIndex = i;
                    mainWindow.pageStack.get(0).queueList.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                }
            }
            mainWindow.pageStack.push(Qt.createComponent("org.kde.kasts", "EntryPage", Component.PreferSynchronous, mainWindow.pageStack.get(0)), {
                entryuid: AudioManager.entryuid
            });
        }
    }

    function openFullScreenImage(): void {
        var options = {
            image: Qt.binding(function () {
                return root.image;
            }),
            description: Qt.binding(function () {
                return root.title;
            }),
            loader: Qt.binding(function () {
                return fullScreenImageLoader;
            })
        };
        fullScreenImageLoader.setSource("qrc:/qt/qml/org/kde/kasts/qml/FullScreenImage.qml", options);
        fullScreenImageLoader.active = true;
        (fullScreenImageLoader.item as FullScreenImage).open();
    }

    Rectangle {
        //set background color
        visible: GraphicsInfo.api !== GraphicsInfo.Software
        anchors.fill: parent
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        color: root.handlePosition > 0 ? "#727272" : Kirigami.Theme.backgroundColor // make sure to have a dark enough background in case image is transparent; color is what backgroundImageLoader produces with a white background as input
    }

    Loader {
        id: backgroundImageLoader
        active: root.handlePosition > 0
        anchors.fill: parent
        sourceComponent: MultiEffect {
            source: backgroundImage
            anchors.fill: parent

            brightness: -0.3
            saturation: 0.6
            contrast: -0.5
            blurMax: 64
            blur: 1.0
            blurEnabled: true
            autoPaddingEnabled: false

            ImageWithFallback {
                id: backgroundImage

                visible: GraphicsInfo.api === GraphicsInfo.Software
                imageSource: root.blurredImage
                imageResize: false // no "stuttering" on resizing the window
                anchors.fill: parent
            }

            Rectangle {
                visible: GraphicsInfo.api === GraphicsInfo.Software
                anchors.fill: parent
                color: "black"
                opacity: 0.8
            }
        }
    }

    Item {
        id: _headerMetaData
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        opacity: height - root.disappearHeight > Kirigami.Units.largeSpacing ? 1 : (height - root.disappearHeight < 0 ? 0 : (height - root.disappearHeight) / Kirigami.Units.largeSpacing)

        visible: opacity === 0 ? false : true

        property string image: root.image
        property string blurredImage: root.blurredImage
        property string title: root.title
        property string feed: AudioManager.entry ? AudioManager.entry.feed.name : KI18n.i18n("No episode loaded")
        property string authors: AudioManager.entry ? AudioManager.entry.feed.authors : ""

        implicitHeight: root.handlePosition
        implicitWidth: parent.width

        RowLayout {
            property int margin: Kirigami.Units.gridUnit * 1
            anchors.fill: parent
            anchors.margins: margin
            anchors.topMargin: parent.height > root.minimumImageSize + 2 * margin ? margin : (parent.height - root.minimumImageSize) / 2
            anchors.bottomMargin: parent.height > root.minimumImageSize + 2 * margin ? margin : (parent.height - root.minimumImageSize) / 2

            ImageWithFallback {
                id: frontImage
                imageSource: _headerMetaData.image
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.minimumHeight: Kirigami.Units.gridUnit * 1.5
                absoluteRadius: Kirigami.Units.smallSpacing

                // resizing the image is very stuttering when the header is resized; relying on mipmap to do the smoothing
                imageResize: false

                MouseArea {
                    anchors.fill: parent
                    cursorShape: AudioManager.entryuid > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: AudioManager.entryuid > 0
                    onClicked: {
                        root.openFullScreenImage();
                    }
                }
            }

            ColumnLayout {
                id: labelLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: parent.margin / 2

                Controls.Label {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: _headerMetaData.title
                    fontSizeMode: Text.Fit
                    font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 1.4)
                    minimumPointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 1.1)
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignBottom
                    color: "#eff0f1" // breeze light text color
                    opacity: 1
                    elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.openEntry();
                        }
                    }
                }

                Controls.Label {
                    visible: labelLayout.height > root.subtitleCollapseHeight
                    Layout.fillWidth: true
                    text: _headerMetaData.feed
                    fontSizeMode: Text.Fit
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                    minimumPointSize: Kirigami.Theme.defaultFont.pointSize
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff0f1" // breeze light text color
                    elide: Text.ElideRight
                    opacity: 1
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).openPodcast(AudioManager.entry.feeduid);
                        }
                    }
                }

                Controls.Label {
                    visible: _headerMetaData.authors && labelLayout.height > root.authorCollapseHeight
                    Layout.fillWidth: true
                    text: _headerMetaData.authors
                    fontSizeMode: Text.Fit
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                    minimumPointSize: Kirigami.Theme.defaultFont.pointSize
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff0f1" // breeze light text color
                    elide: Text.ElideRight
                    opacity: 1
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            (root.Controls.ApplicationWindow.window as Main).openPodcast(AudioManager.entry.feeduid);
                        }
                    }
                }
            }
        }
    }

    DesktopPlayerControls {
        id: _desktopPlayerControls
        anchors.top: root.headerMetaData.bottom
        anchors.bottom: root.bottom
        anchors.left: root.left
        anchors.right: root.right

        headerBar: root as HeaderBar

        onHandlePositionChanged: (y, offset) => {
            root.handlePosition = Math.max(0, Math.min(root.maximumHeight, root.height - implicitHeight - offset + y));
            settings.headerSize = root.handlePosition;
        }
    }

    Kirigami.Separator {
        width: parent.width
        anchors.bottom: parent.bottom
    }

    Loader {
        id: fullScreenImageLoader
        active: false
        visible: active
    }

    Settings {
        id: settings

        property int headerSize: Kirigami.Units.gridUnit * 5
    }
}
