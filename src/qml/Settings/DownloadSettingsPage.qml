/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.ki18n

import org.kde.kasts

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: KI18n.i18nc("@title Form header for download related settings", "Episode download preferences")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormSwitchDelegate {
            id: prioritizeStreaming
            checked: SettingsManager.prioritizeStreaming
            text: KI18n.i18nc("@option:check", "Prefer streaming over downloading")
            description: KI18n.i18nc("@info Extra information related to checkbox", "Episode lists will show streaming buttons instead of download buttons")
            onToggled: {
                SettingsManager.prioritizeStreaming = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            id: maximumFeedDownloads
            text: KI18n.i18nc("@label:spinbox", "Maximum number of parallel episode downloads")
            textItem.wrapMode: Text.Wrap
            trailing: Controls.SpinBox {
                Layout.rightMargin: Kirigami.Units.gridUnit
                value: SettingsManager.maximumParallelEpisodeDownloads
                from: 1
                to: 16
                onValueModified: {
                    SettingsManager.maximumParallelEpisodeDownloads = value;
                    SettingsManager.save();
                }
            }
        }
    }

    FormCard.FormHeader {
        title: KI18n.i18nc("@title Form header for settings related automated podcast updates", "Automated actions")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormSwitchDelegate {
            id: refreshOnStartup
            checked: SettingsManager.refreshOnStartup
            text: KI18n.i18nc("@option:check", "Fetch podcast updates on startup")
            onToggled: {
                SettingsManager.refreshOnStartup = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormComboBoxDelegate {
            id: autoFeedUpdateInterval
            text: KI18n.i18nc("@label:listbox", "Periodically retrieve podcast feeds")
            textRole: "text"
            valueRole: "value"
            model: [
                {
                    text: KI18n.i18nc("@item:inlistbox automatic podcast update interval", "Never"),
                    value: 0
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 1),
                    value: 1
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 2),
                    value: 2
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 4),
                    value: 4
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 8),
                    value: 8
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 12),
                    value: 12
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every day", "Every %1 days", 1),
                    value: 24
                },
                {
                    text: KI18n.i18ncp("@item:inlistbox automatic podcast update interval", "Every day", "Every %1 days", 3),
                    value: 72
                }
            ]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoFeedUpdateInterval)
            onActivated: {
                SettingsManager.autoFeedUpdateInterval = currentValue;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormSwitchDelegate {
            id: autoQueue
            checked: SettingsManager.autoQueue
            text: KI18n.i18nc("@option:check", "Automatically queue new episodes")

            onToggled: {
                SettingsManager.autoQueue = checked;
                if (!checked) {
                    autoDownload.checked = false;
                    SettingsManager.autoDownload = false;
                }
                SettingsManager.save();
            }
        }

        FormCard.FormSwitchDelegate {
            id: autoDownload
            checked: SettingsManager.autoDownload
            text: KI18n.i18nc("@option:check", "Automatically download new episodes")

            enabled: autoQueue.checked
            onToggled: {
                SettingsManager.autoDownload = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormSwitchDelegate {
            id: doFullUpdate
            checked: SettingsManager.doFullUpdate
            text: KI18n.i18nc("@option:check", "Update existing episode data on refresh (slower)")
            onToggled: {
                SettingsManager.doFullUpdate = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: KI18n.i18nc("@title Form header for settings triggered by adding new podcasts", "When adding new podcasts")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormRadioDelegate {
            checked: SettingsManager.markUnreadOnNewFeed === 0
            text: KI18n.i18nc("@option:radio", "Mark all episodes as played")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 0;
                SettingsManager.save();
            }
        }

        FormCard.FormRadioDelegate {
            id: markCustomUnreadNumberButton
            checked: SettingsManager.markUnreadOnNewFeed === 1
            text: KI18n.i18nc("@option:radio", "Mark most recent episodes as unplayed")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 1;
                SettingsManager.save();
            }

            trailing: Controls.SpinBox {
                id: markCustomUnreadNumberSpinBox
                Layout.rightMargin: Kirigami.Units.gridUnit
                enabled: markCustomUnreadNumberButton.checked
                value: SettingsManager.markUnreadOnNewFeedCustomAmount
                from: 0
                to: 100

                onValueModified: {
                    SettingsManager.markUnreadOnNewFeedCustomAmount = value;
                    SettingsManager.save();
                }
            }
        }

        FormCard.FormRadioDelegate {
            checked: SettingsManager.markUnreadOnNewFeed === 2
            text: KI18n.i18nc("@option:radio", "Mark all episodes as unplayed")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 2;
                SettingsManager.save();
            }
        }
    }
}
