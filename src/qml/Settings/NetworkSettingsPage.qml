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
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Network Settings")

    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    ColumnLayout {
        spacing: 0

        MobileForm.FormCard {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("On metered connections")
                }

                MobileForm.FormCheckDelegate {
                    id: allowMeteredFeedUpdates
                    checked: SettingsManager.allowMeteredFeedUpdates
                    text: i18n("Allow podcast updates")
                    onToggled: SettingsManager.allowMeteredFeedUpdates = checked
                }

                MobileForm.FormCheckDelegate {
                    id: allowMeteredEpisodeDownloads
                    checked: SettingsManager.allowMeteredEpisodeDownloads
                    text: i18n("Allow episode downloads")
                    onToggled: SettingsManager.allowMeteredEpisodeDownloads = checked
                }

                MobileForm.FormCheckDelegate {
                    id: allowMeteredImageDownloads
                    checked: SettingsManager.allowMeteredImageDownloads
                    text: i18n("Allow image downloads")
                    onToggled: SettingsManager.allowMeteredImageDownloads = checked
                }

                MobileForm.FormCheckDelegate {
                    id: allowMeteredStreaming
                    checked: SettingsManager.allowMeteredStreaming
                    text: i18n("Allow streaming")
                    onToggled: SettingsManager.allowMeteredStreaming = checked
                }
            }
        }
    }
}
