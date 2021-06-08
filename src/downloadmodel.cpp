/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "downloadmodel.h"

#include "datamanager.h"
#include "downloadmodellogging.h"
#include "episodemodel.h"

DownloadModel::DownloadModel()
    : QAbstractListModel(nullptr)
{
    // initialize item counters
    m_downloadingCount = DataManager::instance().entryCount(EpisodeModel::Downloading);
    m_partiallyDownloadedCount = DataManager::instance().entryCount(EpisodeModel::PartiallyDownloaded);
    m_downloadedCount = DataManager::instance().entryCount(EpisodeModel::Downloaded);
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (index.row() < m_downloadingCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(EpisodeModel::Downloading, index.row()));
    } else if (index.row() < m_downloadingCount + m_partiallyDownloadedCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(EpisodeModel::PartiallyDownloaded, index.row() - m_downloadingCount));
    } else if (index.row() < m_downloadingCount + m_partiallyDownloadedCount + m_downloadedCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(EpisodeModel::Downloaded, index.row() - m_downloadingCount - m_partiallyDownloadedCount));
    } else {
        qWarning() << "Trying to fetch DownloadModel item outside of valid range; this should never happen";
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_downloadingCount + m_partiallyDownloadedCount + m_downloadedCount;
}

void DownloadModel::monitorDownloadStatus()
{
    beginResetModel();

    m_downloadingCount = DataManager::instance().entryCount(EpisodeModel::Downloading);
    m_partiallyDownloadedCount = DataManager::instance().entryCount(EpisodeModel::PartiallyDownloaded);
    m_downloadedCount = DataManager::instance().entryCount(EpisodeModel::Downloaded);

    endResetModel();
}
