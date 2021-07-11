/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import Qt.labs.platform 1.1
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
            text: i18n("Search History")
        }

        Controls.Button {
            icon.name: "search"
            text: i18n("Clear search history")
            onClicked: SearchHistoryModel.deleteSearchHistory();
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
            text: i18n("Storage")
        }

        RowLayout {
            visible: Qt.platform.os !== "android" // not functional on android
            Kirigami.FormData.label: i18n("Storage path:")

            Layout.fillWidth: true
            Controls.TextField {
                Layout.fillWidth: true
                readOnly: true
                text: StorageManager.storagePath
                enabled: !defaultStoragePath.checked
            }
            Controls.Button {
                icon.name: "document-open-folder"
                text: i18n("Select folder...")
                enabled: !defaultStoragePath.checked
                onClicked: storagePathDialog.open()
            }
            FolderDialog {
                id: storagePathDialog
                title: i18n("Select Storage Path")
                currentFolder: "file://" + StorageManager.storagePath
                options: FolderDialog.ShowDirsOnly
                onAccepted: {
                    StorageManager.setStoragePath(folder);
                }
            }
        }

        Controls.CheckBox {
            id: defaultStoragePath
            visible: Qt.platform.os !== "android" // not functional on android
            checked: SettingsManager.storagePath == ""
            text: i18n("Use default path")
            onToggled: {
                if (checked) {
                    StorageManager.setStoragePath("");
                }
            }
        }

        Controls.Label {
            Kirigami.FormData.label: i18n("Podcast Downloads:")
            text: i18nc("Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedEnclosureDirSize)
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Image Cache:")
            Controls.Label {
                text: i18nc("Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedImageDirSize)
            }
            Controls.Button {
                icon.name: "edit-clear-all"
                text: i18n("Clear Cache")
                onClicked: StorageManager.clearImageCache();
            }
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
