/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.Dialog {
    id: root

    required property ChapterModel model

    preferredWidth: Kirigami.Units.gridUnit * 30
    preferredHeight: Kirigami.Units.gridUnit * 25

    showCloseButton: true

    title: KI18n.i18nc("@title of dialog box showing a list of chapters", "Chapters")

    ListView {
        id: chapterList

        currentIndex: -1
        model: root.model
        delegate: ChapterListDelegate {}
    }
}
