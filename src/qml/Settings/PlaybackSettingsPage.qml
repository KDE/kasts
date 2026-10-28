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
import org.kde.ki18n

import org.kde.kasts

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        title: KI18n.i18nc("@title Form header for settings related to playback", "Playback settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            id: selectAudioBackend
            text: KI18n.i18nc("@label:listbox Label for setting to select audio playback backend", "Select audio backend")

            textRole: "text"
            valueRole: "value"

            model: ListModel {
                id: backendModel
            }

            Component.onCompleted: {
                // have to use Number because QML doesn't know about enum names
                for (let index in AudioManager.availableBackends) {
                    backendModel.append({
                        text: AudioManager.backendName(AudioManager.availableBackends[index]),
                        value: Number(AudioManager.availableBackends[index])
                    });

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

        FormCard.FormTextDelegate {
            id: skipForwardStep
            text: KI18n.i18nc("@label:spinbox", "Skip forward interval (in seconds)")
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
            text: KI18n.i18nc("@label:spinbox", "Skip backward interval (in seconds)")
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
        title: KI18n.i18nc("@title Form header for settings related to the queue", "Queue settings")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormSwitchDelegate {
            id: continuePlayingNextEntry
            checked: SettingsManager.continuePlayingNextEntry
            text: KI18n.i18nc("@option:check", "Continue playing next episode after current one finishes")
            onToggled: {
                SettingsManager.continuePlayingNextEntry = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {
            above: continuePlayingNextEntry
            below: resetPositionOnPlayed
        }

        FormCard.FormSwitchDelegate {
            id: resetPositionOnPlayed
            checked: SettingsManager.resetPositionOnPlayed
            text: KI18n.i18nc("@option:check", "Reset play position after an episode is played")
            onToggled: {
                SettingsManager.resetPositionOnPlayed = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {
            above: resetPositionOnPlayed
            below: episodeBehavior
        }

        FormCard.FormComboBoxDelegate {
            id: episodeBehavior
            text: KI18n.i18nc("@label:listbox", "Played episode behavior")
            textRole: "text"
            valueRole: "value"
            model: [
                {
                    text: KI18n.i18nc("@item:inlistbox What to do with played episodes", "Do not delete"),
                    value: 0
                },
                {
                    text: KI18n.i18nc("@item:inlistbox What to do with played episodes", "Delete immediately"),
                    value: 1
                },
                {
                    text: KI18n.i18nc("@item:inlistbox What to do with played episodes", "Delete at next startup"),
                    value: 2
                }
            ]
            Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoDeleteOnPlayed)
            onActivated: {
                SettingsManager.autoDeleteOnPlayed = currentValue;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator {
            above: episodeBehavior
            below: markAsPlayedGracePeriod
        }

        FormCard.FormTextDelegate {
            id: markAsPlayedGracePeriod
            text: KI18n.i18nc("@label:spinbox", "Mark episodes as played when the given time is remaining (in seconds)")
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
}
