/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/feedsmodel.h"

#include <QModelIndex>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

#include "database.h"
#include "datamanager.h"
#include "datatypes.h"
#include "fetcher.h"
#include "objectslogging.h"
#include "utils/entryutils.h"

FeedsModel::FeedsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::feedAdded, this, [this](const qint64 feeduid) {
        updateFeed(feeduid);
        beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
        endInsertRows();
    });
    connect(&DataManager::instance(), &DataManager::feedRemoved, this, [this](const qint64 feeduid) {
        int idx = -1;
        for (int i = 0; i < m_feeds.count(); i++) {
            if (m_feeds[i].feeduid == feeduid) {
                idx = i;
            }
        }

        beginRemoveRows(QModelIndex(), idx, idx);
        m_feeds.remove(idx);
        endRemoveRows();
    });
    connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, &FeedsModel::triggerFeedUpdate);

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT "
                       "    *, "
                       "    COUNT(Entries.entryuid) AS entryCount, "
                       "    SUM(CASE WHEN NOT Entries.read THEN 1 ELSE 0 END) AS unreadCount, "
                       "    SUM(CASE WHEN Entries.new THEN 1 ELSE 0 END) AS newCount, "
                       "    SUM(CASE WHEN Entries.favorite THEN 1 ELSE 0 END) AS favoriteCount "
                       "FROM Feeds "
                       "    INNER JOIN Entries "
                       "        ON Entries.feeduid=Feeds.feeduid "
                       "        GROUP BY Feeds.feeduid;"));
    Database::instance().execute(query);
    while (query.next()) {
        DataTypes::FeedDetails feedDetails;
        feedDetails.feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
        feedDetails.name = query.value(QStringLiteral("name")).toString();
        feedDetails.url = query.value(QStringLiteral("url")).toString();
        feedDetails.image = query.value(QStringLiteral("image")).toString();
        feedDetails.link = query.value(QStringLiteral("link")).toString();
        feedDetails.description = query.value(QStringLiteral("description")).toString();
        feedDetails.subscribed = query.value(QStringLiteral("feeduid")).toLongLong();
        feedDetails.lastUpdated = query.value(QStringLiteral("lastUpdated")).toLongLong();
        feedDetails.entryCount = query.value(QStringLiteral("entryCount")).toLongLong();
        feedDetails.unreadEntryCount = query.value(QStringLiteral("unreadCount")).toLongLong();
        feedDetails.newEntryCount = query.value(QStringLiteral("newCount")).toLongLong();
        feedDetails.favoriteEntryCount = query.value(QStringLiteral("favoriteCount")).toLongLong();
        m_feeds += feedDetails;
    }
    query.finish();

    for (DataTypes::FeedDetails &feed : m_feeds) {
        QStringList authors;
        query.prepare(QStringLiteral("SELECT name FROM FeedAuthors WHERE feeduid=:feeduid"));
        query.bindValue(QStringLiteral(":feeduid"), feed.feeduid);
        Database::instance().execute(query);
        while (query.next()) {
            authors += query.value(QStringLiteral("name")).toString();
        }
        feed.authors = EntryUtils::combineAuthors(authors);
    }

    qCDebug(kastsObjects) << "FeedsModel constructed" << this;
}

FeedsModel::~FeedsModel()
{
    qCDebug(kastsObjects) << "FeedsModel destructed" << this;
}

QHash<int, QByteArray> FeedsModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {FeeduidRole, "feeduid"},
        {UrlRole, "url"},
        {ImageRole, "image"},
        {LinkRole, "link"},
        {DescriptionRole, "description"},
        {AuthorsRole, "authors"},
        {RefreshingRole, "refreshing"},
        {IsSubscribedRole, "isSubscribed"},
        {SubscribedRole, "subscribed"},
        {LastUpdatedRole, "lastUpdated"},
        {EntryCountRole, "entryCount"},
        {UnreadCountRole, "unreadCount"},
        {NewCountRole, "newCount"},
        {FavoriteCountRole, "favoriteCount"},
        {FeedRole, "feed"},
    };
}

int FeedsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_feeds.count();
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case TitleRole:
        return QVariant::fromValue(m_feeds[index.row()].name);
    case FeeduidRole:
        return QVariant::fromValue(m_feeds[index.row()].feeduid);
    case UrlRole:
        return QVariant::fromValue(m_feeds[index.row()].url);
    case ImageRole:
        return QVariant::fromValue(m_feeds[index.row()].image);
    case LinkRole:
        return QVariant::fromValue(m_feeds[index.row()].link);
    case DescriptionRole:
        return QVariant::fromValue(m_feeds[index.row()].description);
    case AuthorsRole:
        return QVariant::fromValue(m_feeds[index.row()].authors);
    case RefreshingRole:
        return QVariant::fromValue(m_feeds[index.row()].refreshing);
    case IsSubscribedRole:
        return QVariant::fromValue(true);
    case LastUpdatedRole:
        return QVariant::fromValue(m_feeds[index.row()].lastUpdated);
    case EntryCountRole:
        return QVariant::fromValue(m_feeds[index.row()].entryCount);
    case UnreadCountRole:
        return QVariant::fromValue(m_feeds[index.row()].unreadEntryCount);
    case NewCountRole:
        return QVariant::fromValue(m_feeds[index.row()].newEntryCount);
    case FavoriteCountRole:
        return QVariant::fromValue(m_feeds[index.row()].favoriteEntryCount);
    case FeedRole:
        return QVariant::fromValue(DataManager::instance().getFeed(m_feeds[index.row()].feeduid));
    default:
        return QVariant();
    }
}

void FeedsModel::triggerFeedUpdate(const qint64 feeduid)
{
    for (int i = 0; i < m_feeds.count(); i++) {
        if (m_feeds[i].feeduid == feeduid) {
            updateFeed(feeduid);
            Q_EMIT dataChanged(index(i, 0), index(i, 0));
            return;
        }
    }
}

void FeedsModel::updateFeed(const qint64 feeduid)
{
    // First find the index of the feed
    int idx = -1;
    for (int i = 0; i < m_feeds.count(); i++) {
        if (m_feeds[i].feeduid == feeduid) {
            idx = i;
        }
    }
    if (idx < 0) {
        DataTypes::FeedDetails feedDetails;
        m_feeds += feedDetails;
        idx = m_feeds.count() - 1;
    }

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT "
                       "    *, "
                       "    COUNT(Entries.entryuid) AS entryCount, "
                       "    SUM(CASE WHEN NOT Entries.read THEN 1 ELSE 0 END) AS unreadCount, "
                       "    SUM(CASE WHEN Entries.new THEN 1 ELSE 0 END) AS newCount, "
                       "    SUM(CASE WHEN Entries.favorite THEN 1 ELSE 0 END) AS favoriteCount "
                       "FROM Feeds "
                       "    INNER JOIN Entries "
                       "        ON Entries.feeduid=Feeds.feeduid "
                       "WHERE Feeds.feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feeds[idx].feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
        m_feeds[idx].name = query.value(QStringLiteral("name")).toString();
        m_feeds[idx].url = query.value(QStringLiteral("url")).toString();
        m_feeds[idx].image = query.value(QStringLiteral("image")).toString();
        m_feeds[idx].link = query.value(QStringLiteral("link")).toString();
        m_feeds[idx].description = query.value(QStringLiteral("description")).toString();
        m_feeds[idx].subscribed = query.value(QStringLiteral("feeduid")).toLongLong();
        m_feeds[idx].lastUpdated = query.value(QStringLiteral("lastUpdated")).toLongLong();
        m_feeds[idx].entryCount = query.value(QStringLiteral("entryCount")).toLongLong();
        m_feeds[idx].unreadEntryCount = query.value(QStringLiteral("unreadCount")).toLongLong();
        m_feeds[idx].newEntryCount = query.value(QStringLiteral("newCount")).toLongLong();
        m_feeds[idx].favoriteEntryCount = query.value(QStringLiteral("favoriteCount")).toLongLong();
    }
}
