/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 * SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kirigamiaddons.labs.components as Addons

import org.kde.kasts
import org.kde.kasts.settings

FormCard.FormCardPage {
    id: root

    property int currentType: SettingsManager.proxyType
    property bool proxyConfigChanged: false

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18nc("@title Form header for settings related to network connections", "Network")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: doNetworkChecks
            checked: SettingsManager.checkNetworkStatus
            text: i18nc("@option:check", "Enable network connection checks")
            onToggled: {
                SettingsManager.checkNetworkStatus = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18nc("@title Form header for settings related to metered connections", "On metered connections")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormCheckDelegate {
            id: allowMeteredFeedUpdates
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredFeedUpdates
            text: i18nc("@option:check", "Allow podcast updates")
            onToggled: {
                SettingsManager.allowMeteredFeedUpdates = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredEpisodeDownloads
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredEpisodeDownloads
            text: i18nc("@option:check", "Allow episode downloads")
            onToggled: {
                SettingsManager.allowMeteredEpisodeDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredImageDownloads
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredImageDownloads
            text: i18nc("@option:check", "Allow image downloads")
            onToggled: {
                SettingsManager.allowMeteredImageDownloads = checked;
                SettingsManager.save();
            }
        }

        FormCard.FormCheckDelegate {
            id: allowMeteredStreaming
            enabled: SettingsManager.checkNetworkStatus
            checked: SettingsManager.allowMeteredStreaming
            text: i18nc("@option:check", "Allow streaming")
            onToggled: {
                SettingsManager.allowMeteredStreaming = checked;
                SettingsManager.save();
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related to network proxies", "Network Proxy")
    }

    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio Network proxy selection", "System Default")
            checked: currentType === 0
            enabled: !SettingsManager.isProxyTypeImmutable
            onToggled: {
                currentType = 0;
            }
        }

        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio Network proxy selection", "No Proxy")
            checked: currentType === 1
            enabled: !SettingsManager.isProxyTypeImmutable
            onToggled: {
                currentType = 1;
            }
        }

        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio Network proxy selection", "HTTP")
            checked: currentType === 2
            enabled: !SettingsManager.isProxyTypeImmutable
            onToggled: {
                currentType = 2;
            }
        }

        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio Network proxy selection", "Socks5")
            checked: currentType === 3
            enabled: !SettingsManager.isProxyTypeImmutable
            onToggled: {
                currentType = 3;
            }
        }

        FormCard.FormDelegateSeparator {
            visible: currentType > 1
        }

        FormCard.FormTextFieldDelegate {
            id: hostField
            visible: currentType > 1
            label: i18nc("@label:textbox Hostname for proxy config", "Host")
            text: SettingsManager.proxyHost
            inputMethodHints: Qt.ImhUrlCharactersOnly
            onEditingFinished: {
                proxyConfigChanged = true;
            }
        }

        FormCard.FormSpinBoxDelegate {
            id: portField
            visible: currentType > 1
            label: i18nc("@label:spinbox Port for proxy config", "Port")
            value: SettingsManager.proxyPort
            from: 0
            to: 65536
            textFromValue: (value, locale) => {
                return value; // it will add a thousands separator if we don't do this, not sure why
            }
            onValueChanged: {
                proxyConfigChanged = true;
            }
        }

        FormCard.FormTextFieldDelegate {
            id: userField
            visible: currentType > 1
            label: i18nc("@label:textbox Username for proxy config", "User")
            text: SettingsManager.proxyUser
            inputMethodHints: Qt.ImhUrlCharactersOnly
            onEditingFinished: {
                proxyConfigChanged = true;
            }
        }

        FormCard.FormTextFieldDelegate {
            id: passwordField
            visible: currentType > 1
            label: i18nc("@label:textbox Password for proxy config", "Password")
            text: SettingsManager.proxyPassword
            echoMode: TextInput.Password
            inputMethodHints: Qt.ImhUrlCharactersOnly
            onEditingFinished: {
                proxyConfigChanged = true;
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            trailing: Controls.Button {
                icon.name: "dialog-ok"
                text: i18nc("@action:button", "Apply")
                enabled: currentType !== SettingsManager.proxyType || proxyConfigChanged
                onClicked: {
                    SettingsManager.proxyType = currentType;
                    SettingsManager.proxyHost = hostField.text;
                    SettingsManager.proxyPort = portField.value;
                    SettingsManager.proxyUser = userField.text;
                    SettingsManager.proxyPassword = passwordField.text;
                    SettingsManager.save();
                    proxyConfigChanged = false;
                    Fetcher.setNetworkProxy();
                }
            }
        }
    }

    footer: Addons.Banner {
        Layout.fillWidth: true
        type: Kirigami.MessageType.Warning
        visible: currentType < 2 && Fetcher.isSystemProxyDefined()
        text:  i18nc("@info:status Warning message related to app proxy settings", "Your system level or app level proxy settings might be ignored by the audio backend when streaming audio. The settings should still be honored by all other network related actions, including downloading episodes.")
    }

    Component.onCompleted: {
        proxyConfigChanged = false;
    }
}
