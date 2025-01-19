/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import org.kde.kasts

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        title: i18nc("@title Form header for settings related to storage paths", "Storage path")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormTextDelegate {
            id: storagePath
            visible: Qt.platform.os !== "android" // not functional on android
            text: i18nc("@label showing path used for local storage", "Storage path")
            description: StorageManager.storagePath

            trailing: Controls.Button {
                Layout.leftMargin: Kirigami.Units.largeSpacing
                icon.name: "document-open-folder"
                text: i18nc("@action:button", "Select folder…")
                enabled: !defaultStoragePath.checked
                onClicked: storagePathDialog.open()
            }


            StorageDirDialog {
                id: storagePathDialog
                title: i18nc("@title of dialog box", "Select Storage Path")
            }
        }

        FormCard.FormDelegateSeparator { above: storagePath; below: defaultStoragePath }

        FormCard.FormCheckDelegate {
            id: defaultStoragePath
            visible: Qt.platform.os !== "android" // not functional on android
            checked: SettingsManager.storagePath == ""
            text: i18nc("@option:check", "Use default path")
            onToggled: {
                if (checked) {
                    StorageManager.setStoragePath("");
                }
            }
        }
    }

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18nc("@title Form header for section showing information about local storage", "Information")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormTextDelegate {
            text: i18nc("@label showing the storage space used by local podcast downloads", "Podcast downloads")
            description: i18nc("@label Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedEnclosureDirSize)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18nc("@label showing the storage space used by the image cache", "Image cache")
            description: i18nc("@label Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedImageDirSize)

            trailing: Controls.Button {
                icon.name: "edit-clear-all"
                text: i18nc("@action:button", "Clear Cache")
                onClicked: StorageManager.clearImageCache();
            }
        }
    }
}
