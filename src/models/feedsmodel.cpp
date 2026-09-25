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
        int idx = m_feedOrder.indexOf(feeduid);
        if (idx > -1) {
            beginRemoveRows(QModelIndex(), idx, idx);
            m_feeds.remove(feeduid);
            m_feedOrder.remove(idx);
            endRemoveRows();
        }
    });
    // TODO: make updates more granular instead of just reloading everything
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, &FeedsModel::updateEntryCount);
    connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, &FeedsModel::updateUnreadCount);
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, &FeedsModel::updateNewCount);
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, &FeedsModel::updateFavoriteCount);
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const qint64 feeduid) {
        updateFeed(feeduid);
        qint64 idx = m_feedOrder.indexOf(feeduid);
        if (idx > -1) {
            Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
        }
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdateStatusChanged, this, [this](const qint64 feeduid, bool status) {
        m_feeds[feeduid].refreshing = status;
        int idx = m_feedOrder.indexOf(feeduid);
        if (idx > -1) {
            Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {FeedsModel::RefreshingRole});
        }
    });

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
        feedDetails.subscribed = query.value(QStringLiteral("subscribed")).toLongLong();
        feedDetails.lastUpdated = query.value(QStringLiteral("lastUpdated")).toLongLong();
        feedDetails.entryCount = query.value(QStringLiteral("entryCount")).toLongLong();
        feedDetails.unreadCount = query.value(QStringLiteral("unreadCount")).toLongLong();
        feedDetails.newCount = query.value(QStringLiteral("newCount")).toLongLong();
        feedDetails.favoriteCount = query.value(QStringLiteral("favoriteCount")).toLongLong();
        m_feeds[feedDetails.feeduid] = feedDetails;
        m_feedOrder += feedDetails.feeduid;
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
        {NameRole, "name"},
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
    };
}

int FeedsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_feedOrder.count();
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case NameRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].name);
    case FeeduidRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].feeduid);
    case UrlRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].url);
    case ImageRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].image);
    case LinkRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].link);
    case DescriptionRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].description);
    case AuthorsRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].authors);
    case RefreshingRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].refreshing);
    case IsSubscribedRole:
        return QVariant::fromValue(true);
    case SubscribedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_feeds[m_feedOrder[index.row()]].subscribed));
    case LastUpdatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_feeds[m_feedOrder[index.row()]].lastUpdated));
    case EntryCountRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].entryCount);
    case UnreadCountRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].unreadCount);
    case NewCountRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].newCount);
    case FavoriteCountRole:
        return QVariant::fromValue(m_feeds[m_feedOrder[index.row()]].favoriteCount);
    default:
        return QVariant();
    }
}

void FeedsModel::updateFeed(const qint64 feeduid)
{
    // Create a new entry if feed doesn't exist yet
    if (!m_feedOrder.contains(feeduid)) {
        DataTypes::FeedDetails feedDetails;
        m_feeds[feeduid] = feedDetails;
        m_feedOrder += feeduid;
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
        m_feeds[feeduid].feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
        m_feeds[feeduid].name = query.value(QStringLiteral("name")).toString();
        m_feeds[feeduid].url = query.value(QStringLiteral("url")).toString();
        m_feeds[feeduid].image = query.value(QStringLiteral("image")).toString();
        m_feeds[feeduid].link = query.value(QStringLiteral("link")).toString();
        m_feeds[feeduid].description = query.value(QStringLiteral("description")).toString();
        m_feeds[feeduid].subscribed = query.value(QStringLiteral("subscribed")).toLongLong();
        m_feeds[feeduid].lastUpdated = query.value(QStringLiteral("lastUpdated")).toLongLong();
        m_feeds[feeduid].entryCount = query.value(QStringLiteral("entryCount")).toLongLong();
        m_feeds[feeduid].unreadCount = query.value(QStringLiteral("unreadCount")).toLongLong();
        m_feeds[feeduid].newCount = query.value(QStringLiteral("newCount")).toLongLong();
        m_feeds[feeduid].favoriteCount = query.value(QStringLiteral("favoriteCount")).toLongLong();
        // TODO: do we need dirname here?
    }

    QStringList authors;
    query.prepare(QStringLiteral("SELECT name FROM FeedAuthors WHERE feeduid=:feeduid"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    while (query.next()) {
        authors += query.value(QStringLiteral("name")).toString();
    }
    m_feeds[feeduid].authors = EntryUtils::combineAuthors(authors);
}

void FeedsModel::updateEntryCount(const qint64 feeduid)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT(entryuid) AS entryCount FROM Entries WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feeds[feeduid].entryCount = query.value(QStringLiteral("entryCount")).toLongLong();
    }

    qint64 idx = m_feedOrder.indexOf(feeduid);
    if (idx > -1) {
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {FeedsModel::EntryCountRole});
        // These signals should trigger the updateNewCount and updateUnreadCount slots below (in addition to other objects listening in)
        Q_EMIT DataManager::instance().unreadEntryCountChanged(feeduid);
        Q_EMIT DataManager::instance().newEntryCountChanged(feeduid);
    }
}

void FeedsModel::updateNewCount(const qint64 feeduid)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT SUM(CASE WHEN new THEN 1 ELSE 0 END) AS newCount FROM Entries WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feeds[feeduid].newCount = query.value(QStringLiteral("newCount")).toLongLong();
    }

    qint64 idx = m_feedOrder.indexOf(feeduid);
    if (idx > -1) {
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {FeedsModel::NewCountRole});
    }
}

void FeedsModel::updateUnreadCount(const qint64 feeduid)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT SUM(CASE WHEN read THEN 0 ELSE 1 END) AS unreadCount FROM Entries WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feeds[feeduid].unreadCount = query.value(QStringLiteral("unreadCount")).toLongLong();
    }

    qint64 idx = m_feedOrder.indexOf(feeduid);
    if (idx > -1) {
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {FeedsModel::UnreadCountRole});
    }
}

void FeedsModel::updateFavoriteCount(const qint64 feeduid)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT SUM(CASE WHEN favorite THEN 1 ELSE 0 END) AS favoriteCount FROM Entries WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feeds[feeduid].newCount = query.value(QStringLiteral("favoriteCount")).toLongLong();
    }

    qint64 idx = m_feedOrder.indexOf(feeduid);
    if (idx > -1) {
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {FeedsModel::FavoriteCountRole});
    }
}
