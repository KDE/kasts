/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kasts

Kirigami.Dialog {
    id: addSheet
    parent: applicationWindow().overlay
    showCloseButton: true
    standardButtons: Kirigami.Dialog.NoButton

    title: i18n("Add New Podcast")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 20

    ColumnLayout {
        Controls.Label {
            text: i18n("Url:")
        }
        Controls.TextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "https://example.com/podcast-feed.rss"
            // focus: addSheet.sheetOpen // disabled for now since it causes problem with virtual keyboard appearing at the same time as the overlay
            Keys.onReturnPressed: addFeedAction.triggered();
        }

        // This item can be used to trigger the addition of a feed; it will open an
        // overlay with options in case the operation is not allowed by the settings
        ConnectionCheckAction {
            id: addFeed
            function action() {
                DataManager.addFeed(urlField.text);
            }
        }
    }

    customFooterActions: Kirigami.Action {
        id: addFeedAction
        text: i18n("Add Podcast")
        enabled: urlField.text
        onTriggered: {
            addSheet.close();
            addFeed.run();
        }
    }
}
