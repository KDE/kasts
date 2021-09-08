/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/downloadmodel.h"
#include "models/downloadmodellogging.h"

#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"

DownloadModel::DownloadModel()
    : QAbstractListModel(nullptr)
{
    updateInternalState();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (index.row() < m_downloadingCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(m_downloadingIds[index.row()]));
    } else if (index.row() < m_downloadingCount + m_partiallyDownloadedCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(m_partiallyDownloadedIds[index.row() - m_downloadingCount]));
    } else if (index.row() < m_downloadingCount + m_partiallyDownloadedCount + m_downloadedCount) {
        return QVariant::fromValue(DataManager::instance().getEntry(m_downloadedIds[index.row() - m_downloadingCount - m_partiallyDownloadedCount]));
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
    updateInternalState();
    endResetModel();
}

void DownloadModel::updateInternalState()
{
    m_downloadingIds.clear();
    m_partiallyDownloadedIds.clear();
    m_downloadedIds.clear();

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.id = Entries.id WHERE downloaded=:downloaded ORDER BY updated DESC;"));

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloading));
    Database::instance().execute(query);
    while (query.next()) {
        m_downloadingIds += query.value(QStringLiteral("id")).toString();
    }

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::PartiallyDownloaded));
    Database::instance().execute(query);
    while (query.next()) {
        m_partiallyDownloadedIds += query.value(QStringLiteral("id")).toString();
    }

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloaded));
    Database::instance().execute(query);
    while (query.next()) {
        m_downloadedIds += query.value(QStringLiteral("id")).toString();
    }

    m_downloadingCount = m_downloadingIds.count();
    m_partiallyDownloadedCount = m_partiallyDownloadedIds.count();
    m_downloadedCount = m_downloadedIds.count();
}
