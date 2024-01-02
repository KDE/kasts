/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQml.Models

import org.kde.kirigami as Kirigami
import org.kde.kmediasession

import org.kde.kasts

import ".."

FocusScope {
    id: headerBar
    height: headerMetaData.implicitHeight + desktopPlayerControls.implicitHeight

    property int handlePosition: settings.headerSize
    property int maximumHeight: Kirigami.Units.gridUnit * 8
    property int minimumImageSize: Kirigami.Units.gridUnit * 1.5
    property int subtitleCollapseHeight: Kirigami.Units.gridUnit * 2.5
    property int authorCollapseHeight: Kirigami.Units.gridUnit * 4
    property int disappearHeight: Kirigami.Units.gridUnit * 1.0

    function openEntry() {
        if (AudioManager.entry) {
            pushPage("QueuePage");
            pageStack.push("qrc:/qt/qml/org/kde/kasts/qml/EntryPage.qml", {"entry": AudioManager.entry});
            pageStack.get(0).lastEntry = AudioManager.entry.id;
            var model = pageStack.get(0).queueList.model;
            for (var i = 0; i <  model.rowCount(); i++) {
                var index = model.index(i, 0);
                if (AudioManager.entry == model.data(index, AbstractEpisodeModel.EntryRole)) {
                    pageStack.get(0).queueList.currentIndex = i;
                    pageStack.get(0).queueList.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);

                }
            }
        }
    }

    function openFeed() {
        if (AudioManager.entry) {
            pushPage("FeedListPage");
            pageStack.push("qrc:/qt/qml/org/kde/kasts/qml/FeedDetailsPage.qml", {"feed": AudioManager.entry.feed});
        }
    }

    function openFullScreenImage() {
        var options = {
            "image": Qt.binding(function() { return headerMetaData.image }),
            "description": Qt.binding(function() { return headerMetaData.title }),
            "loader": Qt.binding(function() { return fullScreenImageLoader})
        };
        fullScreenImageLoader.setSource("qrc:/qt/qml/org/kde/kasts/qml/FullScreenImage.qml", options);
        fullScreenImageLoader.active = true;
        fullScreenImageLoader.item.open();
    }

    Rectangle {
        //set background color
        anchors.fill: parent
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        color: Kirigami.Theme.backgroundColor
    }

    Loader {
        id: backgroundImageLoader
        active: handlePosition > 0
        anchors.fill: parent
        sourceComponent: backgroundImageComponent
    }

    Component {
        id: backgroundImageComponent
        MultiEffect {
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

                visible: false
                imageSource: headerMetaData.blurredImage
                imageResize: false // no "stuttering" on resizing the window
                anchors.fill: parent
            }
        }
    }

    Item {
        id: headerMetaData
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        opacity: height - headerBar.disappearHeight > Kirigami.Units.largeSpacing ? 1 : (height - headerBar.disappearHeight < 0 ? 0 : (height - headerBar.disappearHeight) / Kirigami.Units.largeSpacing)

        visible: opacity === 0 ? false : true

        property string image: AudioManager.entry ? ((desktopPlayerControls.chapterModel.currentChapter && desktopPlayerControls.chapterModel.currentChapter !== undefined) ? desktopPlayerControls.chapterModel.currentChapter.cachedImage : AudioManager.entry.cachedImage) : "no-image"
        property string blurredImage: AudioManager.entry ? AudioManager.entry.cachedImage : "no-image"
        property string title: AudioManager.entry ? AudioManager.entry.title : i18n("No Track Title")
        property string feed: AudioManager.entry ? AudioManager.entry.feed.name : i18n("No track loaded")
        property string authors: AudioManager.entry ? AudioManager.entry.feed.authors : ""

        implicitHeight: headerBar.handlePosition
        implicitWidth: parent.width

        RowLayout {
            property int margin: Kirigami.Units.gridUnit * 1
            anchors.fill: parent
            anchors.margins: margin
            anchors.topMargin: parent.height > headerBar.minimumImageSize + 2 * margin ? margin : (parent.height - headerBar.minimumImageSize) / 2
            anchors.bottomMargin: parent.height > headerBar.minimumImageSize + 2 * margin ? margin : (parent.height - headerBar.minimumImageSize) / 2

            ImageWithFallback {
                id: frontImage
                imageSource: headerMetaData.image
                Layout.fillHeight: true
                Layout.preferredWidth: height
                Layout.minimumHeight: Kirigami.Units.gridUnit * 1.5
                absoluteRadius: Kirigami.Units.smallSpacing
                imageResize: false
                mipmap: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        openFullScreenImage();
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
                    text: headerMetaData.title
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
                            openEntry();
                        }
                    }
                }

                Controls.Label {
                    visible: labelLayout.height > headerBar.subtitleCollapseHeight
                    Layout.fillWidth: true
                    text: headerMetaData.feed
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
                            openFeed();
                        }
                    }
                }

                Controls.Label {
                    visible: (headerMetaData.authors) && labelLayout.height > headerBar.authorCollapseHeight
                    Layout.fillWidth: true
                    text: headerMetaData.authors
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
                            openFeed();
                        }
                    }
                }
            }
        }
    }

    DesktopPlayerControls {
        id: desktopPlayerControls

        anchors.top: headerMetaData.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        onHandlePositionChanged: (y, offset) => {
            handlePosition = Math.max(0, Math.min(headerBar.maximumHeight, headerBar.height - implicitHeight - offset + y));
            settings.headerSize = handlePosition;
        }
    }

    Kirigami.Separator {
        width: parent.width
        anchors.bottom: parent.bottom
    }
}
