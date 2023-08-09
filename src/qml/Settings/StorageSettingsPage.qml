/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard

import org.kde.kasts 1.0

FormCard.FormCardPage {
    id: root

    FormCard.FormHeader {
        title: i18n("Storage path")
        Layout.fillWidth: true
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormTextDelegate {
            id: storagePath
            visible: Qt.platform.os !== "android" // not functional on android
            text: i18n("Storage path")
            description: StorageManager.storagePath

            trailing: Controls.Button {
                Layout.leftMargin: Kirigami.Units.largeSpacing
                icon.name: "document-open-folder"
                text: i18n("Select folder…")
                enabled: !defaultStoragePath.checked
                onClicked: storagePathDialog.open()
            }


            StorageDirDialog {
                id: storagePathDialog
                title: i18n("Select Storage Path")
            }
        }

        FormCard.FormDelegateSeparator { above: storagePath; below: defaultStoragePath }

        FormCard.FormCheckDelegate {
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
    }

    FormCard.FormHeader {
        Layout.fillWidth: true
        title: i18n("Information")
    }

    FormCard.FormCard {
        Layout.fillWidth: true

        FormCard.FormTextDelegate {
            text: i18n("Podcast downloads")
            description: i18nc("Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedEnclosureDirSize)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("Image cache")
            description: i18nc("Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedImageDirSize)

            trailing: Controls.Button {
                icon.name: "edit-clear-all"
                text: i18n("Clear Cache")
                onClicked: StorageManager.clearImageCache();
            }
        }
    }
}
