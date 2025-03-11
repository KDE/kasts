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
#include "enclosure.h"
#include "models/episodemodel.h"

DownloadModel::DownloadModel()
    : QAbstractListModel(nullptr)
{
    updateInternalState();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case EpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_entryIds[index.row()]));
    case EpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_entryIds[index.row()]);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    return {
        {EpisodeModel::Roles::EntryRole, "entry"},
        {EpisodeModel::Roles::IdRole, "id"},
        {EpisodeModel::Roles::ReadRole, "read"},
        {EpisodeModel::Roles::NewRole, "new"},
        {EpisodeModel::Roles::FavoriteRole, "favorite"},
    };
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entryIds.count();
}

void DownloadModel::monitorDownloadStatus()
{
    beginResetModel();
    updateInternalState();
    endResetModel();
}

void DownloadModel::updateInternalState()
{
    m_entryIds.clear();

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.id = Entries.id WHERE downloaded=:downloaded ORDER BY updated DESC;"));

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloading));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
    }

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::PartiallyDownloaded));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
    }

    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloaded));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
    }
}

// Hack to get a QItemSelection in QML
QItemSelection DownloadModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
