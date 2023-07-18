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
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import org.kde.kasts 1.0

Kirigami.ScrollablePage {
    title: i18n("Storage Settings")

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
                        text: i18n("Select Folder…")
                        enabled: !defaultStoragePath.checked
                        onClicked: storagePathDialog.open()
                    }

                    StorageDirDialog {
                        id: storagePathDialog
                        title: i18n("Select Storage Path")
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
