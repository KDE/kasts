/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings

KirigamiSettings.CategorizedSettings {
    actions: [
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "General")
            actionName: "General"
            icon.name: "kasts"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/GeneralSettingsPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Appearance")
            actionName: "Appearance"
            icon.name: "preferences-desktop-theme-global"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/AppearanceSettingsPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Storage")
            actionName: "Storage"
            icon.name: "drive-harddisk"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/StorageSettingsPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Network")
            actionName: "Network"
            icon.name: "network-connect"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/NetworkSettingsPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Synchronization")
            actionName: "Synchronization"
            icon.name: "state-sync"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/SynchronizationSettingsPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Error Log")
            actionName: "Error Log"
            icon.name: "error"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/ErrorListPage.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "About")
            actionName: "About"
            icon.name: "documentinfo"
            page: "qrc:/qt/qml/org/kde/kasts/qml/Settings/AboutPage.qml"
        }
    ]
}
