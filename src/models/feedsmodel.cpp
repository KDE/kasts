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
#include "feed.h"
#include "fetcher.h"

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
    query.prepare(QStringLiteral("SELECT * FROM Feeds;"));
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
        m_feeds += feedDetails;
    }
}

QHash<int, QByteArray> FeedsModel::roleNames() const
{
    return {
        {FeeduidRole, "feeduid"},
        {FeedRole, "feed"},
        {UrlRole, "url"},
        {TitleRole, "title"},
        {UnreadCountRole, "unreadCount"},
        {NewCountRole, "newCount"},
        {FavoriteCountRole, "favoriteCount"},
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
    case FeeduidRole:
        return QVariant::fromValue(m_feeds[index.row()].feeduid);
    case FeedRole:
        return QVariant::fromValue(DataManager::instance().getFeed(m_feeds[index.row()].feeduid));
    case Qt::DisplayRole:
    case UrlRole:
        return QVariant::fromValue(m_feeds[index.row()].url);
    case TitleRole:
        return QVariant::fromValue(m_feeds[index.row()].name);
    case UnreadCountRole:
        return QVariant::fromValue(DataManager::instance().getFeed(m_feeds[index.row()].feeduid)->unreadEntryCount());
    case NewCountRole:
        return QVariant::fromValue(DataManager::instance().getFeed(m_feeds[index.row()].feeduid)->newEntryCount());
    case FavoriteCountRole:
        return QVariant::fromValue(DataManager::instance().getFeed(m_feeds[index.row()].feeduid)->favoriteEntryCount());
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
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE feeduid=:feeduid;"));
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
    }
}
