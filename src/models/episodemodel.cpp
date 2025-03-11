/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodemodel.h"

#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "feed.h"

EpisodeModel::EpisodeModel(QObject *parent)
    : AbstractEpisodeModel(parent)
{
    // When feed is updated, the entire model needs to be reset because we
    // cannot know where the new entries will be inserted into the list (or that
    // maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        Q_UNUSED(url)
        beginResetModel();
        updateInternalState();
        endResetModel();
    });

    updateInternalState();
}

QVariant EpisodeModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(m_titles[index.row()]);
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_entryIds[index.row()]));
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_entryIds[index.row()]);
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(m_read[index.row()]);
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(m_new[index.row()]);
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(m_favorite[index.row()]);
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(m_contents[index.row()]);
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feedNames[index.row()]);
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(m_updated[index.row()]);
    default:
        return QVariant();
    }
}

int EpisodeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entryIds.count();
}

void EpisodeModel::updateInternalState()
{
    m_entryIds.clear();
    m_read.clear();
    m_new.clear();
    m_favorite.clear();
    m_titles.clear();
    m_contents.clear();
    m_feedNames.clear();
    m_updated.clear();

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, read, new, favorite, title, content, feed, updated FROM Entries ORDER BY updated DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
        m_read += query.value(QStringLiteral("read")).toBool();
        m_new += query.value(QStringLiteral("new")).toBool();
        m_favorite += query.value(QStringLiteral("favorite")).toBool();
        m_titles += query.value(QStringLiteral("title")).toString();
        m_contents += query.value(QStringLiteral("content")).toString();
        m_feedNames += DataManager::instance().getFeed(query.value(QStringLiteral("feed")).toString())->name();
        m_updated += query.value(QStringLiteral("updated")).toInt();
    }
}
