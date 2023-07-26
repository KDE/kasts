/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesmodel.h"

#include <QString>

#include "datamanager.h"
#include "entry.h"
#include "feed.h"

EntriesModel::EntriesModel(Feed *feed)
    : AbstractEpisodeModel(feed)
    , m_feed(feed)
{
    // When feed is updated, the entire model needs to be reset because we
    // cannot know where the new entries will be inserted into the list (or that
    // maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        if (m_feed->url() == url) {
            beginResetModel();
            endResetModel();
        }
    });
}

QVariant EntriesModel::data(const QModelIndex &index, int role) const
{
    Entry *entry = DataManager::instance().getEntry(m_feed, index.row());
    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(entry->title());
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(entry);
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(DataManager::instance().getIdList(m_feed)[index.row()]);
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(entry->read());
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(entry->getNew());
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(entry->favorite());
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(entry->content());
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feed->name());
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(entry->updated());
    default:
        return QVariant();
    }
}

int EntriesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return DataManager::instance().entryCount(m_feed);
}

Feed *EntriesModel::feed() const
{
    return m_feed;
}

void EntriesModel::updateInternalState()
{
    // nothing to do; DataManager already has the updated data.
}
