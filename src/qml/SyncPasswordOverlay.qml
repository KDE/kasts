/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.Dialog {
    id: syncPasswordOverlay
    padding: Kirigami.Units.largeSpacing
    maximumWidth: Kirigami.Units.gridUnit * 25
    parent: applicationWindow().overlay

    showCloseButton: true
    standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel

    title: i18n("Sync Password Required")

    onAccepted: {
        Sync.password = passwordField2.text;
        syncPasswordOverlay.close();
    }
    onRejected: syncPasswordOverlay.close()

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        RowLayout {
            width: parent.width
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                source: Sync.provider === Sync.GPodderNextcloud ? "kaccounts-nextcloud" : "gpodder"
            }
            TextEdit {
                id: passwordField
                Layout.fillWidth: true
                readOnly: true
                wrapMode: Text.WordWrap
                text: Sync.provider === Sync.GPodderNextcloud ? i18n("The password for user \"%1\" on Nextcloud server \"%2\" could not be retrieved.", SettingsManager.syncUsername, SettingsManager.syncHostname) : i18n("The password for user \"%1\" on \"gpodder.net\" could not be retrieved.", SettingsManager.syncUsername)
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
                Keys.onReturnPressed: syncPasswordOverlay.accepted()
                focus: syncPasswordOverlay.visible
                echoMode: TextInput.Password
                text: Sync.password
            }
        }
    }
}
