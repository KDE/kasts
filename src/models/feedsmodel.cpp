/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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

#include "database.h"
#include "datamanager.h"
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
    connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, [=](const QString &url) {
        for (int i = 0; i < rowCount(QModelIndex()); i++) {
            if (data(index(i, 0), UrlRole).toString() == url) {
                Q_EMIT dataChanged(index(i, 0), index(i, 0));
                return;
            }
        }
    });
}

QHash<int, QByteArray> FeedsModel::roleNames() const
{
    return {
        {FeedRole, "feed"},
        {UrlRole, "url"},
        {TitleRole, "title"},
        {UnreadCountRole, "unreadCount"},
    };
}

int FeedsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return DataManager::instance().feedCount();
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case FeedRole:
        return QVariant::fromValue(DataManager::instance().getFeed(index.row()));
    case UrlRole:
        return QVariant::fromValue(DataManager::instance().getFeed(index.row())->url());
    case TitleRole:
        return QVariant::fromValue(DataManager::instance().getFeed(index.row())->name());
    case UnreadCountRole:
        return QVariant::fromValue(DataManager::instance().getFeed(index.row())->unreadEntryCount());
    default:
        return QVariant();
    }
}
