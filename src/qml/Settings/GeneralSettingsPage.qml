/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("General Settings")

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
        Controls.CheckBox {
            id: adjustTimeLeft
            checked: SettingsManager.adjustTimeLeft
            text: i18n("Adjust time left based on current playback speed")
            onToggled: SettingsManager.adjustTimeLeft = checked
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
            id: doFullUpdate
            checked: SettingsManager.doFullUpdate
            text: i18n("Update existing episode data on refresh (slower)")
            onToggled: SettingsManager.doFullUpdate = checked
        }
        Controls.CheckBox {
            id: autoQueue
            Kirigami.FormData.label: i18n("New Episodes:")
            checked: SettingsManager.autoQueue
            text: i18n("Automatically Queue")

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
            text: i18n("Automatically Download")

            enabled: autoQueue.checked
            onToggled: SettingsManager.autoDownload = checked
        }

        Controls.ComboBox {
            Kirigami.FormData.label: i18n("Played Episodes:")
            Layout.alignment: Qt.AlignHCenter
            textRole: "text"
            valueRole: "value"
            model: [{"text": i18n("Do Not Delete"), "value": 0},
                    {"text": i18n("Delete Immediately"), "value": 1},
                    {"text": i18n("Delete at Next Startup"), "value": 2}]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoDeleteOnPlayed)
            onActivated: {
                SettingsManager.autoDeleteOnPlayed = currentValue;
            }
        }

        Controls.CheckBox {
            checked: SettingsManager.resetPositionOnPlayed
            text: i18n("Reset Play Position")
            onToggled: SettingsManager.resetPositionOnPlayed = checked
        }

        Controls.RadioButton {
            Kirigami.FormData.label: i18n("When adding new podcasts:")
            checked: SettingsManager.markUnreadOnNewFeed === 0
            text: i18n("Mark all episodes as played")
            onToggled: SettingsManager.markUnreadOnNewFeed = 0
        }

        RowLayout {
            Controls.RadioButton {
                id: markCustomUnreadNumberButton
                checked: SettingsManager.markUnreadOnNewFeed === 1
                text: i18n("Mark most recent episodes as unplayed:")
                onToggled: SettingsManager.markUnreadOnNewFeed = 1
            }

            Controls.SpinBox {
                id: markCustomUnreadNumberSpinBox
                enabled: markCustomUnreadNumberButton.checked
                value: SettingsManager.markUnreadOnNewFeedCustomAmount
                from: 0
                to: 100

                onValueModified: SettingsManager.markUnreadOnNewFeedCustomAmount = value
            }
        }

        Controls.RadioButton {
            checked: SettingsManager.markUnreadOnNewFeed === 2
            text: i18n("Mark all episodes as unplayed")
            onToggled: SettingsManager.markUnreadOnNewFeed = 2
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
            onClicked: settingsErrorOverlay.open()
        }

        ErrorListOverlay {
            id: settingsErrorOverlay
        }
    }
}
