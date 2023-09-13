/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick.Dialogs

import org.kde.kasts

FileDialog {
    selectFolder: true
    folder: "file://" + StorageManager.storagePath
    onAccepted: {
        StorageManager.setStoragePath(fileUrl);
    }
}
