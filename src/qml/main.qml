/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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

    property bool playerOpen: false

    pageStack.initialPage: feedList

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true
        actions: [
            Kirigami.Action {
                text: i18n("Feed List")
                iconName: "rss"
                onTriggered: root.pageStack.clear(), root.pageStack.push(feedList)
                enabled: pageStack.layers.currentItem.title !== i18n("Feed List")
            },
            Kirigami.Action {
                text: i18n("Podcast Queue")
                iconName: "source-playlist"
                onTriggered: root.pageStack.clear(), root.pageStack.push(queuelist)
                enabled: pageStack.layers.currentItem.title !== i18n("Podcast Queue")
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

        source: "gst-pipeline: playbin uri=file://" + entry.enclosure.path + " audio_sink=\"scaletempo ! audioconvert ! audioresample ! autoaudiosink\" video_sink=\"fakevideosink\""
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

    footer: MinimizedPlayerControls {}
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
