/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodemodel.h"

#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "entry.h"

EpisodeModel::EpisodeModel()
    : QAbstractListModel(nullptr)
{
    // When feed is updated, the entire model needs to be reset because we
    // cannot know where the new entries will be inserted into the list (or that
    // maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        Q_UNUSED(url)
        beginResetModel();
        updateInternalState();
        endResetModel();
    });

    updateInternalState();
}

QVariant EpisodeModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    return QVariant::fromValue(DataManager::instance().getEntry(m_entryIds[index.row()]));
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
    return m_entryIds.count();
}

void EpisodeModel::updateInternalState()
{
    m_entryIds.clear();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id FROM Entries ORDER BY updated DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
    }
}
