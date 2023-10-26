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
        title: i18n("Appearance")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            Layout.fillWidth: true
            id: colorTheme
            text: i18n("Color theme")
            textRole: "display"
            valueRole: "display"
            model: ColorSchemer.model
            Component.onCompleted: currentIndex = ColorSchemer.indexForScheme(SettingsManager.colorScheme);
            onCurrentValueChanged: {
                ColorSchemer.apply(currentIndex);
                SettingsManager.colorScheme = ColorSchemer.nameForIndex(currentIndex);
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormCheckDelegate {
            id: alwaysShowFeedTitles
            text: i18n("Always show podcast titles in subscription view")
            checked: SettingsManager.alwaysShowFeedTitles
            onToggled: {
                SettingsManager.alwaysShowFeedTitles = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormCheckDelegate {
            id: showTrayIcon
            visible: SystrayIcon.available
            enabled: SystrayIcon.available
            text: i18n("Show icon in system tray")
            checked: SettingsManager.showTrayIcon
            onToggled: {
                SettingsManager.showTrayIcon = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: minimizeToTray
            visible: SystrayIcon.available
            enabled: SettingsManager.showTrayIcon && SystrayIcon.available
            text: i18n("Minimize to tray instead of closing")
            checked: SettingsManager.minimizeToTray
            onToggled: {
                SettingsManager.minimizeToTray = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormComboBoxDelegate {
            id: trayIconType
            visible: SystrayIcon.available
            enabled: SettingsManager.showTrayIcon && SystrayIcon.available
            text: i18nc("Label for selecting the color of the tray icon", "Tray icon type")

            textRole: "text"
            valueRole: "value"

            model: [{"text": i18nc("Label describing style of tray icon", "Colorful"), "value": 0},
                    {"text": i18nc("Label describing style of tray icon", "Light"), "value": 1},
                    {"text": i18nc("Label describing style of tray icon", "Dark"), "value": 2}]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.trayIconType)
            onActivated: {
                SettingsManager.trayIconType = currentValue;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18n("Playback settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            id: selectAudioBackend
            text: i18nc("Label for setting to select audio playback backend", "Select audio backend")

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
            Kirigami.FormData.label: i18nc("Label for settings related to the play time, e.g. whether the total track time is shown or a countdown of the remaining play time", "Play time:")
            checked: SettingsManager.toggleRemainingTime
            text: i18n("Show time left instead of total track time")
            onToggled: {
                SettingsManager.toggleRemainingTime = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: adjustTimeLeft
            checked: SettingsManager.adjustTimeLeft
            enabled: SettingsManager.toggleRemainingTime
            text: i18n("Adjust time left based on current playback speed")
            onToggled: {
                SettingsManager.adjustTimeLeft = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: prioritizeStreaming
            checked: SettingsManager.prioritizeStreaming
            text: i18n("Prioritize streaming over downloading")
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
        title: i18n("Queue settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: continuePlayingNextEntry
            checked: SettingsManager.continuePlayingNextEntry
            text: i18n("Continue playing next episode after current one finishes")
            onToggled: {
                SettingsManager.continuePlayingNextEntry = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: refreshOnStartup
            Kirigami.FormData.label: i18nc("Label for settings related to podcast updates", "Update Settings:")
            checked: SettingsManager.refreshOnStartup
            text: i18n("Automatically fetch podcast updates on startup")
            onToggled: {
                SettingsManager.refreshOnStartup = checked;
                SettingsManager.save();
            }
        }
        FormCard.FormCheckDelegate {
            id: doFullUpdate
            checked: SettingsManager.doFullUpdate
            text: i18n("Update existing episode data on refresh (slower)")
            onToggled: {
                SettingsManager.doFullUpdate = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: autoQueue
            checked: SettingsManager.autoQueue
            text: i18n("Automatically queue new episodes")

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
            text: i18n("Automatically download new episodes")

            enabled: autoQueue.checked
            onToggled: {
                SettingsManager.autoDownload = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator { above: autoDownload; below: episodeBehavior }

        FormCard.FormComboBoxDelegate {
            id: episodeBehavior
            text: i18n("Played episode behavior")
            textRole: "text"
            valueRole: "value"
            model: [{"text": i18n("Do not delete"), "value": 0},
                    {"text": i18n("Delete immediately"), "value": 1},
                    {"text": i18n("Delete at next startup"), "value": 2}]
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

        FormCard.FormDelegateSeparator { above: markAsPlayedGracePeriod; below: resetPositionOnPlayed }

        FormCard.FormCheckDelegate {
            id: resetPositionOnPlayed
            checked: SettingsManager.resetPositionOnPlayed
            text: i18n("Reset play position after an episode is played")
            onToggled: {
                SettingsManager.resetPositionOnPlayed = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18n("When adding new podcasts")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormRadioDelegate {
            checked: SettingsManager.markUnreadOnNewFeed === 0
            text: i18n("Mark all episodes as played")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 0;
                SettingsManager.save();
            }
        }


        FormCard.FormRadioDelegate {
            id: markCustomUnreadNumberButton
            checked: SettingsManager.markUnreadOnNewFeed === 1
            text: i18n("Mark most recent episodes as unplayed")
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
            text: i18n("Mark all episodes as unplayed")
            onToggled: {
                SettingsManager.markUnreadOnNewFeed = 2;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18n("Article")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormTextDelegate {
            id: fontSize
            text: i18n("Font size")

            trailing: Controls.SpinBox {
                id: articleFontSizeSpinBox

                enabled: !useSystemFontCheckBox.checked
                value: SettingsManager.articleFontSize
                Kirigami.FormData.label: i18n("Font size:")
                from: 6
                to: 20

                onValueModified: {
                    SettingsManager.articleFontSize = value;
                    SettingsManager.save();
                }
            }
        }

        FormCard.FormDelegateSeparator { above: fontSize; below: useSystemFontCheckBox }

        FormCard.FormCheckDelegate {
            id: useSystemFontCheckBox
            checked: SettingsManager.articleFontUseSystem
            text: i18n("Use system default")

            onToggled: {
                SettingsManager.articleFontUseSystem = checked;
                SettingsManager.save();
            }
        }
    }
}
