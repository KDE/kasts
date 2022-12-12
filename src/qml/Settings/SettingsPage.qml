/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.18 as Kirigami

Kirigami.CategorizedSettings {
    actions: [
        Kirigami.SettingAction {
            text: i18n("General")
            icon.name: ":/logo.svg"
            page: "qrc:/GeneralSettingsPage.qml"
        },
        Kirigami.SettingAction {
            text: i18n("Storage")
            icon.name: "drive-harddisk-symbolic"
            page: "qrc:/StorageSettingsPage.qml"
        },
        Kirigami.SettingAction {
            text: i18n("Network")
            icon.name: "network-connect"
            page: "qrc:/NetworkSettingsPage.qml"
        },
        Kirigami.SettingAction {
            text: i18n("Synchronization")
            icon.name: "state-sync"
            page: "qrc:/SynchronizationSettingsPage.qml"
        },
        Kirigami.SettingAction {
            text: i18n("About")
            icon.name: "documentinfo"
            page: "qrc:/AboutPage.qml"
        }
    ]
}
