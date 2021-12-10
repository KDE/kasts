/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.kasts 1.0

Kirigami.OverlaySheet {
    id: syncPasswordOverlay
    parent: applicationWindow().overlay
    showCloseButton: true

    header: Kirigami.Heading {
        text: i18n("Sync Password Required")
        elide: Text.ElideRight
    }

    contentItem: Column {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        spacing: Kirigami.Units.largeSpacing
        RowLayout {
            width: parent.width
            spacing: Kirigami.Units.largeSpacing
            // Disable images until licensing has been sorted out
            // Image {
            //     sourceSize.height: Kirigami.Units.gridUnit * 4
            //     sourceSize.width: Kirigami.Units.gridUnit * 4
            //     fillMode: Image.PreserveAspectFit
            //     source: Sync.provider === Sync.GPodderNextcloud ? "qrc:/nextcloud-icon.svg" : "qrc:/gpoddernet.svg"
            // }
            TextEdit {
                id: passwordField
                Layout.fillWidth: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: Sync.provider === Sync.GPodderNextcloud ?
                        i18n("The password for user \"%1\" on Nextcloud server \"%2\" could not be retrieved.", SettingsManager.syncUsername, SettingsManager.syncHostname) :
                        i18n("The password for user \"%1\" on \"gpodder.net\" could not be retrieved.", SettingsManager.syncUsername)
                color: Kirigami.Theme.textColor
            }
        }
        RowLayout {
            width: parent.width
            Controls.Label {
                text: i18n("Password:")
            }
            Controls.TextField {
                id: passwordField2
                Layout.fillWidth: true
                Keys.onReturnPressed: passwordButtons.accepted();
                focus: syncPasswordOverlay.sheetOpen
                echoMode: TextInput.Password
                text: Sync.password
            }
        }
    }

    footer: Controls.DialogButtonBox {
        id: passwordButtons
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        onAccepted: {
            Sync.password = passwordField2.text;
            syncPasswordOverlay.close();
        }
        onRejected: syncPasswordOverlay.close();
    }
}
