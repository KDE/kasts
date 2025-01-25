/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2025 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigamiaddons.settings as KirigamiSettings

KirigamiSettings.ConfigurationView {
    modules: [
        KirigamiSettings.ConfigurationModule {
            moduleId: "general"
            text: i18nc("@title:menu Category in settings", "General")
            icon.name: "kasts"
            page: () => Qt.createComponent("org.kde.kasts", "GeneralSettingsPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "appearance"
            text: i18nc("@title:menu Category in settings", "Appearance")
            icon.name: "preferences-desktop-theme-global"
            page: () => Qt.createComponent("org.kde.kasts", "AppearanceSettingsPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "Storage"
            text: i18nc("@title:menu Category in settings", "Storage")
            icon.name: "drive-harddisk"
            page: () => Qt.createComponent("org.kde.kasts", "StorageSettingsPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "Network"
            text: i18nc("@title:menu Category in settings", "Network")
            icon.name: "network-connect"
            page: () => Qt.createComponent("org.kde.kasts", "NetworkSettingsPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "Synchronization"
            text: i18nc("@title:menu Category in settings", "Synchronization")
            icon.name: "state-sync"
            page: () => Qt.createComponent("org.kde.kasts", "SynchronizationSettingsPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "Error Log"
            text: i18nc("@title:menu Category in settings", "Error Log")
            icon.name: "error"
            page: () => Qt.createComponent("org.kde.kasts", "ErrorListPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "aboutKasts"
            text: i18nc("@title:menu Category in settings", "About")
            icon.name: "documentinfo"
            page: () => Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage")
            category: i18nc("@title:group", "About Kasts")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "aboutKDE"
            text: i18nc("@title:menu Category in settings", "About KDE")
            icon.name: "kde-symbolic"
            page: () => Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutKDEPage")
            category: i18nc("@title:group", "About")
        }
     ]
}
