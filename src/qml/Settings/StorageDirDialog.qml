/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import Qt.labs.platform 1.1

import org.kde.kasts 1.0

FolderDialog {
    currentFolder: "file://" + StorageManager.storagePath
    options: FolderDialog.ShowDirsOnly
    onAccepted: {
        StorageManager.setStoragePath(folder);
    }
}
