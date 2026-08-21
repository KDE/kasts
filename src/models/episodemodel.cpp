/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodemodel.h"

#include "datamanager.h"

EpisodeModel::EpisodeModel(QObject *parent)
    : AbstractEpisodeModel(QStringLiteral("SELECT feeduid, name FROM Feeds;"),
                           QStringLiteral("SELECT * FROM Entries JOIN Feeds ON Feeds.feeduid=Entries.feeduid ORDER BY updated DESC;"),
                           QStringLiteral("SELECT * FROM Enclosures;"),
                           parent)
{
    // When feed is updated or removed, the entire model needs to be reset
    // because we cannot know where the new entries will be inserted into the
    // list (or that maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const qint64 feeduid) {
        if (m_feeds.contains(feeduid)) {
            beginResetModel();
            updateInternalState();
            endResetModel();
        }
    });
    connect(&DataManager::instance(), &DataManager::feedRemoved, this, [this](const qint64 feeduid) {
        if (m_feeds.contains(feeduid)) {
            beginResetModel();
            updateInternalState();
            endResetModel();
        }
    });
}
