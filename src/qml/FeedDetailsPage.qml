/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.12 as Kirigami

import org.kde.alligator 1.0

Kirigami.ScrollablePage {
    id: page

    property QtObject feed;

    title: i18nc("<Feed Name> - Details", "%1 - Details", feed.name)

    header: GenericHeader {
        id: headerImage

        image: feed.cachedImage
        title: feed.name
        subtitle: page.feed.authors.length === 0 ? "" : i18nc("by <author(s)>", "by") + " " + page.feed.authors[0].name
    }

    ColumnLayout {
        Kirigami.Heading {
            text: feed.description;
            level: 3
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18nc("by <author(s)>", "by %1", feed.authors[0].name)
            visible: feed.authors.length !== 0
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: "<a href='%1'>%1</a>".arg(feed.link)
            onLinkActivated: Qt.openUrlExternally(link)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("Subscribed since: %1", feed.subscribed.toLocaleString(Qt.locale(), Locale.ShortFormat))
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("last updated: %1", feed.lastUpdated.toLocaleString(Qt.locale(), Locale.ShortFormat))
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Controls.Label {
            text: i18n("%1 posts, %2 unread", feed.entryCount, feed.unreadEntryCount)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
