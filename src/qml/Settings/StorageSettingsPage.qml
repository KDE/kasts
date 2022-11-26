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
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    id: page
    title: i18n("Storage Settings")

    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    ColumnLayout {
        spacing: 0
        width: page.width

        MobileForm.FormCard {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Storage path")
                }

                MobileForm.FormTextDelegate {
                    id: storagePath
                    visible: Qt.platform.os !== "android" // not functional on android
                    text: i18n("Storage path")
                    description: StorageManager.storagePath

                    trailing: Controls.Button {
                        Layout.leftMargin: Kirigami.Units.largeSpacing
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

                MobileForm.FormDelegateSeparator { above: storagePath; below: defaultStoragePath }

                MobileForm.FormCheckDelegate {
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
        }

        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: i18n("Information")
                }

                MobileForm.FormTextDelegate {
                    text: i18n("Podcast downloads")
                    description: i18nc("Using <amount of bytes> of disk space", "Using %1 of disk space", StorageManager.formattedEnclosureDirSize)
                }

                MobileForm.FormDelegateSeparator {}

                MobileForm.FormTextDelegate {
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
    }
}
