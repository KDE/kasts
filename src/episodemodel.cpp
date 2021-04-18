/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "episodemodel.h"
#include "datamanager.h"


EpisodeModel::EpisodeModel()
    : QAbstractListModel(nullptr)
{
    // When feed is updated, the entire model needs to be reset because we
    // cannot know where the new entries will be inserted into the list (or that
    // maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        // we have to reset the entire model in case entries are removed or added
        // because we have no way of knowing where those entries will be added/removed
        beginResetModel();
        endResetModel();
    });
}

QVariant EpisodeModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    return QVariant::fromValue(DataManager::instance().getEntry(m_type, index.row()));
}

QHash<int, QByteArray> EpisodeModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int EpisodeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return DataManager::instance().entryCount(m_type);
}

EpisodeModel::Type EpisodeModel::type() const
{
    return m_type;
}

void EpisodeModel::setType(EpisodeModel::Type type)
{
    m_type = type;
    if (m_type == EpisodeModel::New) {
        connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, [this](const QString &url) {
            // we have to reset the entire model in case entries are removed or added
            // because we have no way of knowing where those entries will be added/removed
            beginResetModel();
            endResetModel();
        });
    } else if (m_type == EpisodeModel::Unread) {
        connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, [this](const QString &url) {
            // we have to reset the entire model in case entries are removed or added
            // because we have no way of knowing where those entries will be added/removed
            beginResetModel();
            endResetModel();
        });
    }
    else if (m_type == EpisodeModel::Downloaded) {  // TODO: this needs to be removed !!!!!!
        connect(&DataManager::instance(), &DataManager::downloadCountChanged, this, [this](const QString &url) {
            // we have to reset the entire model in case entries are removed or added
            // because we have no way of knowing where those entries will be added/removed
            beginResetModel();
            endResetModel();
        });
    }
}
