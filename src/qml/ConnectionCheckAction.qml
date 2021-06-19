/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.14 as Kirigami

import org.kde.kasts 1.0

Kirigami.OverlaySheet {
    id: overlay
    parent: applicationWindow().overlay

    property string headingText: i18n("Podcast updates are currently not allowed on metered connections")
    property bool condition: SettingsManager.allowMeteredFeedUpdates

    // Function to be overloaded where this is instantiated with another purpose
    // than refreshing all feeds
    function action() {
        Fetcher.fetchAll();
    }

    // This function will be executed when "Don't allow" is chosen; can be overloaded
    function abortAction() { }

    // This function will be executed when the "Allow once" action is chosen; can be overloaded
    function allowOnceAction() {
        SettingsManager.allowMeteredFeedUpdates = true;
        action()
        SettingsManager.allowMeteredFeedUpdates = false;
    }

    // This function will be executed when the "Always allow" action is chosed; can be overloaded
    function alwaysAllowAction() {
        SettingsManager.allowMeteredFeedUpdates = true;
        action()
    }

    // this is the function that should be called if the action should be
    // triggered conditionally (on the basis that the condition is passed)
    function run() {
        if (!Fetcher.isMeteredConnection() || condition) {
            action();
        } else {
            overlay.open();
        }
    }

    header: Kirigami.Heading {
        text: overlay.headingText
        level: 2
        wrapMode: Text.Wrap
        elide: Text.ElideRight
        maximumLineCount: 3
    }

    contentItem: ColumnLayout {

        spacing: 0

        Kirigami.BasicListItem {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2
            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: 0
            text: i18n("Don't Allow")
            onClicked: {
                abortAction();
                close();
            }
        }

        Kirigami.BasicListItem {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2
            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: 0
            text: i18n("Allow Once")
            onClicked: {
                allowOnceAction();
                close();
            }
        }

        Kirigami.BasicListItem {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2
            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: 0
            text: i18n("Always Allow")
            onClicked: {
                alwaysAllowAction();
                close();
            }
        }
    }
}
