/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesmodel.h"

#include <QString>

#include "datamanager.h"
#include "entry.h"
#include "feed.h"
#include "models/episodemodel.h"

EntriesModel::EntriesModel(Feed *feed)
    : QAbstractListModel(feed)
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
    switch (role) {
    case EpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_feed, index.row()));
    case EpisodeModel::Roles::IdRole:
        return QVariant::fromValue(DataManager::instance().getIdList(m_feed)[index.row()]);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> EntriesModel::roleNames() const
{
    return {
        {EpisodeModel::Roles::EntryRole, "entry"},
        {EpisodeModel::Roles::IdRole, "id"},
        {EpisodeModel::Roles::ReadRole, "read"},
        {EpisodeModel::Roles::NewRole, "new"},
    };
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

// Hack to get a QItemSelection in QML
QItemSelection EntriesModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
