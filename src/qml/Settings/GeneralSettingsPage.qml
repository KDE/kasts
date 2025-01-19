/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kmediasession

import org.kde.kasts

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related to playback", "Playback settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            id: selectAudioBackend
            text: i18nc("@label:listbox Label for setting to select audio playback backend", "Select audio backend")

            textRole: "text"
            valueRole: "value"

            model: ListModel {
                id: backendModel
            }

            Component.onCompleted: {
                // have to use Number because QML doesn't know about enum names
                for (var index in AudioManager.availableBackends) {
                    backendModel.append({"text": AudioManager.backendName(AudioManager.availableBackends[index]),
                                         "value": Number(AudioManager.availableBackends[index])});

                    if (Number(AudioManager.availableBackends[index]) === AudioManager.currentBackend) {
                        currentIndex = index;
                    }
                }
            }

            onActivated: {
                AudioManager.currentBackend = currentValue;
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormCheckDelegate {
            id: showTimeLeft
            checked: SettingsManager.toggleRemainingTime
            text: i18nc("@option:check Label for setting whether the total track time is shown or a countdown of the remaining play time", "Show time left instead of total track time")
            onToggled: {
                SettingsManager.toggleRemainingTime = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: adjustTimeLeft
            checked: SettingsManager.adjustTimeLeft
            enabled: SettingsManager.toggleRemainingTime
            text: i18nc("@option:check", "Adjust time left based on current playback speed")
            onToggled: {
                SettingsManager.adjustTimeLeft = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: prioritizeStreaming
            checked: SettingsManager.prioritizeStreaming
            text: i18nc("@option:check", "Prioritize streaming over downloading")
            onToggled: {
                SettingsManager.prioritizeStreaming = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormDelegateSeparator {
            below: prioritizeStreaming
            above: skipForwardStep
        }
        FormCard.FormTextDelegate {
            id: skipForwardStep
            text: i18nc("@label:spinbox", "Skip forward interval (in seconds)")
            trailing: Controls.SpinBox {
                Layout.rightMargin: Kirigami.Units.gridUnit
                value: SettingsManager.skipForward
                from: 1
                to: 300
                onValueModified: {
                    SettingsManager.skipForward = value;
                    SettingsManager.save();
                }
            }
        }
        FormCard.FormTextDelegate {
            id: skipBackwardInterval
            text: i18nc("@label:spinbox", "Skip backward interval (in seconds)")
            trailing: Controls.SpinBox {
                Layout.rightMargin: Kirigami.Units.gridUnit
                value: SettingsManager.skipBackward
                from: 1
                to: 300
                onValueModified: {
                    SettingsManager.skipBackward = value;
                    SettingsManager.save();
                }
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related podcast updates", "Podcast update settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            id: autoFeedUpdateInterval
            text: i18nc("@label:listbox", "Automatically fetch podcast feeds")
            textRole: "text"
            valueRole: "value"
            model: [{"text": i18nc("@item:inlistbox automatic podcast update interval", "Never"), "value": 0},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 1), "value": 1},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 2), "value": 2},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 4), "value": 4},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 8), "value": 8},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every hour", "Every %1 hours", 12), "value": 12},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every day", "Every %1 days", 1), "value": 24},
                    {"text": i18ncp("@item:inlistbox automatic podcast update interval", "Every day", "Every %1 days", 3), "value": 72}]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoFeedUpdateInterval)
            onActivated: {
                SettingsManager.autoFeedUpdateInterval = currentValue;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: refreshOnStartup
            checked: SettingsManager.refreshOnStartup
            text: i18nc("@option:check", "Fetch podcast updates on startup")
            onToggled: {
                SettingsManager.refreshOnStartup = checked;
                SettingsManager.save();
            }
        }


        FormCard.FormDelegateSeparator { above: refreshOnStartup; below: doFullUpdate }

        FormCard.FormCheckDelegate {
            id: doFullUpdate
            checked: SettingsManager.doFullUpdate
            text: i18nc("@option:check", "Update existing episode data on refresh (slower)")
            onToggled: {
                SettingsManager.doFullUpdate = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: autoQueue
            checked: SettingsManager.autoQueue
            text: i18nc("@option:check", "Automatically queue new episodes")

            onToggled: {
                SettingsManager.autoQueue = checked;
                if (!checked) {
                    autoDownload.checked = false;
                    SettingsManager.autoDownload = false;
                }
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: autoDownload
            checked: SettingsManager.autoDownload
            text: i18nc("@option:check", "Automatically download new episodes")

            enabled: autoQueue.checked
            onToggled: {
                SettingsManager.autoDownload = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related to the queue", "Queue settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: continuePlayingNextEntry
            checked: SettingsManager.continuePlayingNextEntry
            text: i18nc("@option:check", "Continue playing next episode after current one finishes")
            onToggled: {
                SettingsManager.continuePlayingNextEntry = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator { above: continuePlayingNextEntry; below: resetPositionOnPlayed }

        FormCard.FormCheckDelegate {
            id: resetPositionOnPlayed
            checked: SettingsManager.resetPositionOnPlayed
            text: i18nc("@option:check", "Reset play position after an episode is played")
            onToggled: {
                SettingsManager.resetPositionOnPlayed = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator { above: resetPositionOnPlayed; below: episodeBehavior }


        FormCard.FormComboBoxDelegate {
            id: episodeBehavior
            text: i18nc("@label:listbox", "Played episode behavior")
            textRole: "text"
            valueRole: "value"
            model: [{"text": i18nc("@item:inlistbox What to do with played episodes", "Do not delete"), "value": 0},
                    {"text": i18nc("@item:inlistbox What to do with played episodes", "Delete immediately"), "value": 1},
                    {"text": i18nc("@item:inlistbox What to do with played episodes", "Delete at next startup"), "value": 2}]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoDeleteOnPlayed)
            onActivated: {
                SettingsManager.autoDeleteOnPlayed = currentValue;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator { above: episodeBehavior; below: markAsPlayedGracePeriod }

        FormCard.FormTextDelegate {
            id: markAsPlayedGracePeriod
            text: i18nc("@label:spinbox", "Mark episodes as played when the given time is remaining (in seconds)")
            textItem.wrapMode: Text.Wrap
            trailing: Controls.SpinBox {
                Layout.rightMargin: Kirigami.Units.gridUnit
                value: SettingsManager.markAsPlayedBeforeEnd
                from: 0
                to: 300
                onValueModified: {
                    SettingsManager.markAsPlayedBeforeEnd = value;
                    SettingsManager.save();
                }
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings triggered by adding new podcasts", "When adding new podcasts")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormRadioDelegate {
            checked: SettingsManager.markUnreadOnNewFeed === 0
            text: i18nc("@option:radio", "Mark all episodes as played")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 0;
                SettingsManager.save();
            }
        }


        FormCard.FormRadioDelegate {
            id: markCustomUnreadNumberButton
            checked: SettingsManager.markUnreadOnNewFeed === 1
            text: i18nc("@option:radio", "Mark most recent episodes as unplayed")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 1;
                SettingsManager.save();
            }

            trailing: Controls.SpinBox {
                Layout.rightMargin: Kirigami.Units.gridUnit
                id: markCustomUnreadNumberSpinBox
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
            text: i18nc("@option:radio", "Mark all episodes as unplayed")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 2;
                SettingsManager.save();
            }
        }
    }
}
