/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesmodel.h"

#include <QString>

#include "datamanager.h"

EntriesModel::EntriesModel(const qint64 feeduid, QObject *parent)
    : AbstractEpisodeModel(
          QStringLiteral("SELECT feeduid, name FROM Feeds WHERE feeduid=%1;").arg(feeduid),
          QStringLiteral("SELECT * FROM Entries JOIN Feeds ON Feeds.feeduid=Entries.feeduid WHERE Feeds.feeduid=%1 ORDER BY updated DESC;").arg(feeduid),
          QStringLiteral("SELECT * FROM Enclosures WHERE feeduid=%1;").arg(feeduid),
          parent) // TODO: probably needs another parent?
    , m_feeduid(feeduid)
{
    // When feed is updated, the entire model needs to be reset
    // because we cannot know where the new entries will be inserted into the
    // list (or that maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const qint64 feeduid) {
        if (m_feeds.contains(feeduid)) {
            beginResetModel();
            updateInternalState();
            endResetModel();
        }
    });
}
