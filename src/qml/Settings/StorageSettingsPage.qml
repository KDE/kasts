/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3

import org.kde.kirigami 2.12 as Kirigami

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Storage Settings")

    Kirigami.FormLayout {
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
            FileDialog {
                id: storagePathDialog
                title: i18n("Select Storage Path")
                selectFolder: true
                folder: "file://" + StorageManager.storagePath
                onAccepted: {
                    StorageManager.setStoragePath(fileUrl);
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
    }
}
