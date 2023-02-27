/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm
import org.kde.kmediasession 1.0

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("General Settings")

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
                    title: i18n("Appearance")
                }

                MobileForm.FormCheckDelegate {
                    id: alwaysShowFeedTitles
                    text: i18n("Always show podcast titles in subscription view")
                    checked: SettingsManager.alwaysShowFeedTitles
                    onToggled: {
                        SettingsManager.alwaysShowFeedTitles = checked;
                        SettingsManager.save();
                    }
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormCheckDelegate {
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

                MobileForm.FormCheckDelegate {
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

                MobileForm.FormComboBoxDelegate {
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
        }

        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Playback settings")
                }

                MobileForm.FormComboBoxDelegate {
                    id: selectAudioBackend
                    text: i18nc("Label for setting to select audio playback backend", "Select Audio Backend")

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

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormCheckDelegate {
                    id: showTimeLeft
                    Kirigami.FormData.label: i18nc("Label for settings related to the play time, e.g. whether the total track time is shown or a countdown of the remaining play time", "Play Time:")
                    checked: SettingsManager.toggleRemainingTime
                    text: i18n("Show time left instead of total track time")
                    onToggled: {
                        SettingsManager.toggleRemainingTime = checked;
                        SettingsManager.save();
                    }
                }
                MobileForm.FormCheckDelegate {
                    id: adjustTimeLeft
                    checked: SettingsManager.adjustTimeLeft
                    enabled: SettingsManager.toggleRemainingTime
                    text: i18n("Adjust time left based on current playback speed")
                    onToggled: {
                        SettingsManager.adjustTimeLeft = checked;
                        SettingsManager.save();
                    }
                }
                MobileForm.FormCheckDelegate {
                    id: prioritizeStreaming
                    checked: SettingsManager.prioritizeStreaming
                    text: i18n("Prioritize streaming over downloading")
                    onToggled: {
                        SettingsManager.prioritizeStreaming = checked;
                        SettingsManager.save();
                    }
                }
            }
        }

        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Queue settings")
                }

                MobileForm.FormCheckDelegate {
                    id: continuePlayingNextEntry
                    checked: SettingsManager.continuePlayingNextEntry
                    text: i18n("Continue playing next episode after current one finishes")
                    onToggled: {
                        SettingsManager.continuePlayingNextEntry = checked;
                        SettingsManager.save();
                    }
                }
                MobileForm.FormCheckDelegate {
                    id: refreshOnStartup
                    Kirigami.FormData.label: i18nc("Label for settings related to podcast updates", "Update Settings:")
                    checked: SettingsManager.refreshOnStartup
                    text: i18n("Automatically fetch podcast updates on startup")
                    onToggled: {
                        SettingsManager.refreshOnStartup = checked;
                        SettingsManager.save();
                    }
                }
                MobileForm.FormCheckDelegate {
                    id: doFullUpdate
                    checked: SettingsManager.doFullUpdate
                    text: i18n("Update existing episode data on refresh (slower)")
                    onToggled: {
                        SettingsManager.doFullUpdate = checked;
                        SettingsManager.save();
                    }
                }

                MobileForm.FormCheckDelegate {
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

                MobileForm.FormCheckDelegate {
                    id: autoDownload
                    checked: SettingsManager.autoDownload
                    text: i18n("Automatically download new episodes")

                    enabled: autoQueue.checked
                    onToggled: {
                        SettingsManager.autoDownload = checked;
                        SettingsManager.save();
                    }
                }

                MobileForm.FormDelegateSeparator { above: autoDownload; below: episodeBehavior }

                MobileForm.FormComboBoxDelegate {
                    id: episodeBehavior
                    text: i18n("Played episode behavior")
                    textRole: "text"
                    valueRole: "value"
                    model: [{"text": i18n("Do Not Delete"), "value": 0},
                            {"text": i18n("Delete Immediately"), "value": 1},
                            {"text": i18n("Delete at Next Startup"), "value": 2}]
                    Component.onCompleted: currentIndex = indexOfValue(SettingsManager.autoDeleteOnPlayed)
                    onActivated: {
                        SettingsManager.autoDeleteOnPlayed = currentValue;
                        SettingsManager.save();
                    }
                }

                MobileForm.FormDelegateSeparator { above: episodeBehavior; below: resetPositionOnPlayed }

                MobileForm.FormCheckDelegate {
                    id: resetPositionOnPlayed
                    checked: SettingsManager.resetPositionOnPlayed
                    text: i18n("Reset play position after an episode is played")
                    onToggled: {
                        SettingsManager.resetPositionOnPlayed = checked;
                        SettingsManager.save();
                    }
                }
            }
        }

        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("When adding new podcasts")
                }

                MobileForm.FormRadioDelegate {
                    checked: SettingsManager.markUnreadOnNewFeed === 0
                    text: i18n("Mark all episodes as played")
                    onToggled: {
                        SettingsManager.markUnreadOnNewFeed = 0;
                        SettingsManager.save();
                    }
                }


                MobileForm.FormRadioDelegate {
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

                MobileForm.FormRadioDelegate {
                    checked: SettingsManager.markUnreadOnNewFeed === 2
                    text: i18n("Mark all episodes as unplayed")
                    onToggled: {
                        SettingsManager.markUnreadOnNewFeed = 2;
                        SettingsManager.save();
                    }
                }
            }
        }

        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Article")
                }

                MobileForm.FormTextDelegate {
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

                MobileForm.FormDelegateSeparator { above: fontSize; below: useSystemFontCheckBox }

                MobileForm.FormCheckDelegate {
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
    }
}
