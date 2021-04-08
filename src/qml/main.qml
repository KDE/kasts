/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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

Kirigami.ApplicationWindow {
    id: root

    title: "Alligator"

    pageStack.initialPage: feedList

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true
        actions: [
            Kirigami.Action {
                text: i18n("Queue")
                iconName: "source-playlist"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(queuelist)
                }
            },
            Kirigami.Action {
                text: i18n("Subscriptions")
                iconName: "rss"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(feedList)
                }
            },
            Kirigami.Action {
                text: i18n("Settings")
                iconName: "settings-configure"
                onTriggered: pageStack.layers.push("qrc:/SettingsPage.qml")
                enabled: pageStack.layers.currentItem.title !== i18n("Settings")
            },
            Kirigami.Action {
                text: i18n("About")
                iconName: "help-about-symbolic"
                onTriggered: pageStack.layers.push(aboutPage)
                enabled: pageStack.layers.currentItem.title !== i18n("About")
            }
        ]
    }

    Component {
        id: aboutPage
        Kirigami.AboutPage {
            aboutData: _aboutData
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    FeedListPage  {
        id: feedList
    }

    QueuePage {
        id: queuelist
    }

    Audio {
        id: audio

        property var entry
        property bool playerOpen: false

        source: "file://" + entry.enclosure.path
        //source: entry.enclosure.url
        onError: console.debug(errorString)
        //source: "gst-pipeline: playbin uri=file://" + entry.enclosure.path + " audio_sink=\"scaletempo ! audioconvert ! audioresample ! autoaudiosink\" video_sink=\"fakevideosink\""
    }

    /*
    Loader {
        id: footerLoader

        property var minimizedSize: Kirigami.Units.gridUnit * 3.0

        anchors.fill: parent
        active: (audio.entry == undefined) ? false : true
        visible: active
        z: (!item || item.contentY == 0) ? -1 : 999
        sourceComponent: FooterBar {
            contentHeight: root.height * 2
            focus: true
        }

    }
    */

    footer: MinimizedPlayerControls { }
    /*Item {
        visible: (audio.entry !== undefined)
        height: footerLoader.minimizedSize
    }
    */

    /*Kirigami.OverlaySheet {
        id: playeroverlay
        sheetOpen: False
        PlayerControls {
            height: root.height*5.0/6.0
            width: root.width*5.0/6.0;
        }
    }*/
}
