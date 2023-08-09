/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard

import org.kde.kasts 1.0

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
