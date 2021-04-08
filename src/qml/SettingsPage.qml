/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Settings")

    // TODO: Remove old alligator settings from the kcfg and the qml code

    Kirigami.FormLayout {

        Kirigami.Heading {
            Kirigami.FormData.isSection: true
            text: i18n("Queue Settings")
        }

        Controls.SpinBox {
            id: numberNewEpisodes

            Kirigami.FormData.label: i18n("# of episodes to label as new when adding a new subscription:")
            value: _settings.numberNewEpisodes

            onValueModified: _settings.numberNewEpisodes = value
        }

        Controls.CheckBox {
            id: autoDownload
            checked: _settings.autoDownload
            text: i18n("Automatically download new episodes")

            onToggled: _settings.autoDownload = checked
        }

        Controls.CheckBox {
            id: allowStreaming
            checked: _settings.allowStreaming
            text: i18n("Allow streaming of audio")

            onToggled: _settings.allowStreaming = checked
        }

    }
}
