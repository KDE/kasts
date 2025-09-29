/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/feedsmodel.h"

#include <QDebug>
#include <QModelIndex>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>
#include <qnamespace.h>

#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"

FeedsModel::FeedsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::feedAdded, this, [this]() {
        beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
        endInsertRows();
    });
    connect(&DataManager::instance(), &DataManager::feedRemoved, this, [this](const int &index) {
        beginRemoveRows(QModelIndex(), index, index);
        endRemoveRows();
    });
    connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, &FeedsModel::triggerFeedUpdate);
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, &FeedsModel::triggerFeedUpdate);
}

QHash<int, QByteArray> FeedsModel::roleNames() const
{
    return {
        {UrlRole, "url"},
        {FeedIdRole, "feedid"},
        {FeedRole, "feed"},
        {NameRole, "name"},
        {TitleRole, "title"},
        {UnreadCountRole, "unreadCount"},
        {NewCountRole, "newCount"},
        {FavoriteCountRole, "favoriteCount"},
    };
}

int FeedsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return DataManager::instance().feedCount();
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
    const auto feed = DataManager::instance().getFeedByIndex(index.row());
    if (!feed) {
        return {};
    }
    switch (role) {
    case UrlRole:
        return QVariant::fromValue(feed->url());
    case FeedIdRole:
        return QVariant::fromValue(feed->feedid());
    case FeedRole:
        return QVariant::fromValue(feed);
    case NameRole:
        return QVariant::fromValue(feed->name());
    case TitleRole:
        return QVariant::fromValue(feed->name());
    case UnreadCountRole:
        return QVariant::fromValue(feed->unreadEntryCount());
    case NewCountRole:
        return QVariant::fromValue(feed->newEntryCount());
    case FavoriteCountRole:
        return QVariant::fromValue(feed->favoriteEntryCount());
    default:
        return QVariant();
    }
}

void FeedsModel::triggerFeedUpdate(const int &feedid)
{
    for (int i = 0; i < rowCount(QModelIndex()); i++) {
        if (data(index(i, 0), FeedIdRole).toInt() == feedid) {
            Q_EMIT dataChanged(index(i, 0), index(i, 0));
            return;
        }
    }
}
