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
    title: i18n("Settings")

    Kirigami.FormLayout {

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Appearance")
        }

        Controls.CheckBox {
            id: alwaysShowFeedTitles
            checked: SettingsManager.alwaysShowFeedTitles
            text: i18n("Always show podcast titles in subscription view")
            onToggled: SettingsManager.alwaysShowFeedTitles = checked
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Play Settings")
        }

        Controls.CheckBox {
            id: continuePlayingNextEntry
            checked: SettingsManager.continuePlayingNextEntry
            text: i18n("Continue playing next episode after current one finishes")
            onToggled: SettingsManager.continuePlayingNextEntry = checked
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Queue Settings")
        }

        Controls.CheckBox {
            id: refreshOnStartup
            checked: SettingsManager.refreshOnStartup
            text: i18n("Automatically fetch podcast updates on startup")
            onToggled: SettingsManager.refreshOnStartup = checked
        }

        Controls.CheckBox {
            id: autoQueue
            checked: SettingsManager.autoQueue
            text: i18n("Automatically queue new episodes")

            onToggled: {
                SettingsManager.autoQueue = checked
                if (!checked) {
                    autoDownload.checked = false
                    SettingsManager.autoDownload = false
                }
            }
        }

        Controls.CheckBox {
            id: autoDownload
            checked: SettingsManager.autoDownload
            text: i18n("Automatically download new episodes")

            enabled: autoQueue.checked
            onToggled: SettingsManager.autoDownload = checked
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Network")
        }

        Controls.CheckBox {
            id: allowMeteredFeedUpdates
            checked: SettingsManager.allowMeteredFeedUpdates || !Fetcher.canCheckNetworkStatus()
            Kirigami.FormData.label: i18n("On metered connections:")
            text: i18n("Allow podcast updates")
            onToggled: SettingsManager.allowMeteredFeedUpdates = checked
            enabled: Fetcher.canCheckNetworkStatus()
        }

        Controls.CheckBox {
            id: allowMeteredEpisodeDownloads
            checked: SettingsManager.allowMeteredEpisodeDownloads || !Fetcher.canCheckNetworkStatus()
            text: i18n("Allow episode downloads")
            onToggled: SettingsManager.allowMeteredEpisodeDownloads = checked
            enabled: Fetcher.canCheckNetworkStatus()
        }

        Controls.CheckBox {
            id: allowMeteredImageDownloads
            checked: SettingsManager.allowMeteredImageDownloads || !Fetcher.canCheckNetworkStatus()
            text: i18n("Allow image downloads")
            onToggled: SettingsManager.allowMeteredImageDownloads = checked
            enabled: Fetcher.canCheckNetworkStatus()
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Article")
        }

        Controls.SpinBox {
            id: articleFontSizeSpinBox

            enabled: !useSystemFontCheckBox.checked
            value: SettingsManager.articleFontSize
            Kirigami.FormData.label: i18n("Font size:")
            from: 6
            to: 20

            onValueModified: SettingsManager.articleFontSize = value
        }

        Controls.CheckBox {
            id: useSystemFontCheckBox
            checked: SettingsManager.articleFontUseSystem
            text: i18n("Use system default")

            onToggled: SettingsManager.articleFontUseSystem = checked
        }

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Errors")
        }

        Controls.Button {
            icon.name: "error"
            text: i18n("Show Error Log")
            onClicked: errorOverlay.open()
        }
    }
}
