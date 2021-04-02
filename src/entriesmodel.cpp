/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QAbstractListModel>
#include <QVariant>
#include <QVector>

#include "database.h"
#include "datamanager.h"
#include "entriesmodel.h"
#include "fetcher.h"

EntriesModel::EntriesModel(Feed *feed)
    : QAbstractListModel(feed)
    , m_feed(feed)
{
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        if (m_feed->url() == url) {
            beginResetModel();
            // TODO: make sure to pop the entrylistpage if it's the active one
            endResetModel();
        }
    });
}

QVariant EntriesModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    return QVariant::fromValue(DataManager::instance().getEntry(m_feed, index.row()));
}

QHash<int, QByteArray> EntriesModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
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
