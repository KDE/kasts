/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: detailsPage

    property QtObject feed;

    title: i18nc("<Feed Name> - Details", "%1 - Details", feed.name)

    ColumnLayout {
        Kirigami.Icon {
            source: Fetcher.image(feed.image)
            height: 200
            width: height
        }
        Kirigami.Heading {
            text: feed.name
        }
        Kirigami.Heading {
            text: feed.description;
            level: 3
        }
        Controls.Label {
            text: i18nc("by <author(s)>", "by %1", feed.authors[0].name)
            visible: feed.authors.length !== 0
        }
        Controls.Label {
            text: "<a href='%1'>%1</a>".arg(feed.link)
            onLinkActivated: Qt.openUrlExternally(link)
        }
        Controls.Label {
            text: i18n("Subscribed since: %1", feed.subscribed.toLocaleString(Qt.locale(), Locale.ShortFormat))
        }
        Controls.Label {
            text: i18n("last updated: %1", feed.lastUpdated.toLocaleString(Qt.locale(), Locale.ShortFormat))
        }
        Controls.Label {
            text: i18n("%1 posts, %2 unread", feed.entryCount, feed.unreadEntryCount)
        }
    }
}
