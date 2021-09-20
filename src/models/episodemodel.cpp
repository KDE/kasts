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

EpisodeModel::EpisodeModel(QObject *parent)
    : QAbstractListModel(parent)
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
    switch (role) {
    case EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_entryIds[index.row()]));
    case IdRole:
        return QVariant::fromValue(m_entryIds[index.row()]);
    case ReadRole:
        return QVariant::fromValue(m_read[index.row()]);
    case NewRole:
        return QVariant::fromValue(m_new[index.row()]);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> EpisodeModel::roleNames() const
{
    return {
        {EntryRole, "entry"},
        {IdRole, "id"},
        {ReadRole, "read"},
        {NewRole, "new"},
    };
}

int EpisodeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entryIds.count();
}

void EpisodeModel::updateInternalState()
{
    m_entryIds.clear();
    m_read.clear();
    m_new.clear();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, read, new FROM Entries ORDER BY updated DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_entryIds += query.value(QStringLiteral("id")).toString();
        m_read += query.value(QStringLiteral("read")).toBool();
        m_new += query.value(QStringLiteral("new")).toBool();
    }
}
