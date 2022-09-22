/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Network Settings")

    Kirigami.FormLayout {
        Controls.CheckBox {
            id: allowMeteredFeedUpdates
            checked: SettingsManager.allowMeteredFeedUpdates
            Kirigami.FormData.label: i18n("On metered connections:")
            text: i18n("Allow podcast updates")
            onToggled: SettingsManager.allowMeteredFeedUpdates = checked
        }

        Controls.CheckBox {
            id: allowMeteredEpisodeDownloads
            checked: SettingsManager.allowMeteredEpisodeDownloads
            text: i18n("Allow episode downloads")
            onToggled: SettingsManager.allowMeteredEpisodeDownloads = checked
        }

        Controls.CheckBox {
            id: allowMeteredImageDownloads
            checked: SettingsManager.allowMeteredImageDownloads
            text: i18n("Allow image downloads")
            onToggled: SettingsManager.allowMeteredImageDownloads = checked
        }

        Controls.CheckBox {
            id: allowMeteredStreaming
            checked: SettingsManager.allowMeteredStreaming
            text: i18n("Allow streaming")
            onToggled: SettingsManager.allowMeteredStreaming = checked
        }
    }
}
