/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QString>

#include "entriesmodel.h"
#include "entry.h"
#include "datamanager.h"

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
    if (role != 0)
        return QVariant();
    //qDebug() << "fetching item" << index.row();
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

Feed* EntriesModel::feed() const
{
    return m_feed;
}
