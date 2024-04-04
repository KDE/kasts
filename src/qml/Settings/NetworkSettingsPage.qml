/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import org.kde.kasts
import org.kde.kasts.settings

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18nc("@title Form header for settings related to network connections", "Network")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: doNetworkChecks
            checked: SettingsManager.checkNetworkStatus
            text: i18nc("@option:check", "Enable network connection checks")
            onToggled: {
                SettingsManager.checkNetworkStatus = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18nc("@title Form header for settings related to metered connections", "On metered connections")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: allowMeteredFeedUpdates
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredFeedUpdates
            text: i18nc("@option:check", "Allow podcast updates")
            onToggled: {
                SettingsManager.allowMeteredFeedUpdates = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredEpisodeDownloads
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredEpisodeDownloads
            text: i18nc("@option:check", "Allow episode downloads")
            onToggled: {
                SettingsManager.allowMeteredEpisodeDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredImageDownloads
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredImageDownloads
            text: i18nc("@option:check", "Allow image downloads")
            onToggled: {
                SettingsManager.allowMeteredImageDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredStreaming
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredStreaming
            text: i18nc("@option:check", "Allow streaming")
            onToggled: {
                SettingsManager.allowMeteredStreaming = checked;
                SettingsManager.save();
            }
        }
    }
}
