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

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18n("On metered connections")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: allowMeteredFeedUpdates
            checked: SettingsManager.allowMeteredFeedUpdates
            text: i18n("Allow podcast updates")
            onToggled: {
                SettingsManager.allowMeteredFeedUpdates = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredEpisodeDownloads
            checked: SettingsManager.allowMeteredEpisodeDownloads
            text: i18n("Allow episode downloads")
            onToggled: {
                SettingsManager.allowMeteredEpisodeDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredImageDownloads
            checked: SettingsManager.allowMeteredImageDownloads
            text: i18n("Allow image downloads")
            onToggled: {
                SettingsManager.allowMeteredImageDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredStreaming
            checked: SettingsManager.allowMeteredStreaming
            text: i18n("Allow streaming")
            onToggled: {
                SettingsManager.allowMeteredStreaming = checked;
                SettingsManager.save();
            }
        }
    }
}
