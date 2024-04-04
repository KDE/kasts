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
import org.kde.kasts.settings

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related to app appearance", "Appearance")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormComboBoxDelegate {
            Layout.fillWidth: true
            id: colorTheme
            text: i18nc("@label:listbox", "Color theme")
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
            text: i18nc("@option:check", "Always show podcast titles in subscription view")
            checked: SettingsManager.alwaysShowFeedTitles
            onToggled: {
                SettingsManager.alwaysShowFeedTitles = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title Title header for settings related to the tray icon", "Tray icon")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: showTrayIcon
            visible: SystrayIcon.available
            enabled: SystrayIcon.available
            text: i18nc("@option:check", "Show icon in system tray")
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
            text: i18nc("@option:check", "Minimize to tray instead of closing")
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
            text: i18nc("@label:listbox Label for selecting the color of the tray icon", "Tray icon type")

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
        title: i18nc("@title Form header for settings related to text/fonts", "Text")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: useSystemFontCheckBox
            checked: SettingsManager.articleFontUseSystem
            text: i18nc("@option:check", "Use system default")

            onToggled: {
                SettingsManager.articleFontUseSystem = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormDelegateSeparator { below: fontSize; above: useSystemFontCheckBox }

        FormCard.FormTextDelegate {
            id: fontSize
            text: i18nc("@label:spinbox", "Font size")

            trailing: Controls.SpinBox {
                id: articleFontSizeSpinBox

                enabled: !useSystemFontCheckBox.checked
                value: SettingsManager.articleFontSize
                from: 6
                to: 20

                onValueModified: {
                    SettingsManager.articleFontSize = value;
                    SettingsManager.save();
                }
            }
        }
    }
}
