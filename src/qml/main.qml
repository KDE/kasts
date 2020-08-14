/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls

import org.kde.kirigami 2.12 as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: "Alligator"

    pageStack.initialPage: feedList

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true
        actions: [
            Kirigami.Action {
                text: i18n("Settings")
                iconName: "settings-configure"
                onTriggered: pageStack.layers.push("qrc:/SettingsPage.qml", {"settings": _settings})
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
}
