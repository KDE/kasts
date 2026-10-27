/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.ki18n

import org.kde.kasts

Kirigami.PromptDialog {
    id: root

    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

    title: KI18n.i18n("Add New Podcast")
    preferredWidth: Kirigami.Units.gridUnit * 20

    // This item can be used to trigger the addition of a feed; it will open an
    // overlay with options in case the operation is not allowed by the settings
    ConnectionCheckAction {
        id: addFeed
        function action(): void {
            DataManager.addFeed(urlField.text.trim());
        }
    }

    onAccepted: {
        if (urlField.text.trim().length > 0) {
            addFeed.run();
        }
    }

    onOpened: {
        urlField.forceActiveFocus();
    }

    Component.onCompleted: {
        standardButton(Kirigami.Dialog.Ok).enabled = Qt.binding(function () {
            return urlField.acceptableInput && urlField.text.trim() != "";
        });
    }

    Kirigami.FormLayout {
        Controls.TextField {
            id: urlField
            implicitWidth: Kirigami.Units.gridUnit * 16
            Layout.fillWidth: true
            Kirigami.FormData.label: KI18n.i18n("Url:")
            placeholderText: "https://example.com/podcast-feed.rss"
            validator: RegularExpressionValidator {
                regularExpression: /\S+/
            }
            Keys.onReturnPressed: root.accept()
        }
    }
}
