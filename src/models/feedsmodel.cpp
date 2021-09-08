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
}

QHash<int, QByteArray> FeedsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "feed";
    return roleNames;
}

int FeedsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return DataManager::instance().feedCount();
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    return QVariant::fromValue(DataManager::instance().getFeed(index.row()));
}

void FeedsModel::removeFeed(int index)
{
    DataManager::instance().removeFeed(index);
}

void FeedsModel::refreshAll()
{
    //    for (auto &feed : m_feeds) {
    //        feed->refresh();
    //    }
}
